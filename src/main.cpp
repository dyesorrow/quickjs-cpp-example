
extern "C" {
#include "quickjs/quickjs.h"
#include "quickjs/quickjs-libc.h"
}

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <malloc.h>

#include <string>
#include <iostream>
#include <fstream>

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static JSValue eval_buf(JSContext *ctx, const char *buf, int buf_len, const char *filename, int eval_flags = 0) {
    JSValue val;
    int ret;
    if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        /* for the modules, we compile then run to be able to set import.meta */
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(val)) {
            js_module_set_import_meta(ctx, val, 1, 1);
            val = JS_EvalFunction(ctx, val);
        }
    } else {
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    }
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }
    // JS_FreeValue(ctx, val);
    return val;
}

int main(int argc, char **argv) {
    //==============================================================================
    // 初始化
    //==============================================================================

    JSRuntime *rt;
    JSContext *ctx;

    rt = JS_NewRuntime();
    if (!rt) {
        fprintf(stderr, "cannot allocate JS runtime\n");
        exit(2);
    }

    auto JS_NewCustomContext = [](JSRuntime *rt) -> JSContext * {
        JSContext *ctx = JS_NewContextRaw(rt);
        if (!ctx) {
            return NULL;
        }

        JS_AddIntrinsicBaseObjects(ctx);
        JS_AddIntrinsicDate(ctx);
        JS_AddIntrinsicEval(ctx);
        JS_AddIntrinsicStringNormalize(ctx);
        JS_AddIntrinsicRegExp(ctx);
        JS_AddIntrinsicJSON(ctx);
        JS_AddIntrinsicProxy(ctx);
        JS_AddIntrinsicMapSet(ctx);
        JS_AddIntrinsicTypedArrays(ctx);
        JS_AddIntrinsicPromise(ctx);
        JS_AddIntrinsicBigInt(ctx);
        JS_AddIntrinsicBigFloat(ctx);
        JS_AddIntrinsicBigDecimal(ctx);
        JS_AddIntrinsicOperators(ctx);
        JS_EnableBignumExt(ctx, 1);

        js_init_module_std(ctx, "std");
        js_init_module_os(ctx, "os");


        // 全局模块导入
        const char *str =
            "import * as std from 'std';\n"
            "import * as os from 'os';\n"
            "globalThis.std = std;\n"
            "globalThis.os = os;\n";
        eval_buf(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);

        return ctx;
    };

    js_std_set_worker_new_context_func(JS_NewCustomContext);
    js_std_init_handlers(rt);
    ctx = JS_NewCustomContext(rt);
    if (!ctx) {
        fprintf(stderr, "cannot allocate JS context\n");
        exit(2);
    }
    js_std_add_helpers(ctx, argc, argv);

    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
    JS_SetHostPromiseRejectionTracker(rt, js_std_promise_rejection_tracker, NULL);

    //==============================================================================
    // 添加 c 模块
    //==============================================================================
    std::string module_name = "file";
    static const JSCFunctionListEntry funcs[] = {
        JS_CFUNC_DEF("read", 4, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {
            JSValue obj;
            printf("call c here");
            sleep(1000);
            return obj;
        }),
    };
    JSModuleDef *m = JS_NewCModule(ctx, module_name.c_str(), [](JSContext *ctx, JSModuleDef *m) -> int {
        return JS_SetModuleExportList(ctx, m, funcs, countof(funcs));
    });
    JS_AddModuleExportList(ctx, m, funcs, countof(funcs));

    //==============================================================================
    // 执行文件
    //==============================================================================
    std::string filename = "test.js";
    std::ifstream in(filename, std::ios::in | std::ios::binary | std::ios::ate);
    long size = in.tellg();
    char *buff = new char[size + 1];
    in.seekg(0, std::ios::beg);
    in.read(buff, size);
    buff[size] = 0;
    JSValue v = eval_buf(ctx, buff, size, filename.c_str(), JS_EVAL_TYPE_GLOBAL|JS_EVAL_TYPE_MODULE);
    js_std_loop(ctx);


    //==============================================================================
    // 执行结束
    //==============================================================================
    // 输出内存占用
    // JSMemoryUsage stats;
    // JS_ComputeMemoryUsage(rt, &stats);
    // JS_DumpMemoryUsage(stdout, &stats, rt);

    // 释放内存
    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}
