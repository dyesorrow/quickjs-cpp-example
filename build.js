const https = require("http");
const fs = require("fs");
const path = require("path");
const spawn = require('child_process').spawn;
const util = require('util');
const { exit } = require("process");
// promisify() 在所有情况下都会假定 original 是一个以回调作为其最后参数的函数
// callback 第一个参数为 error

const config = {};

const exec = util.promisify((cmd, args, options, callback) => {
    options.stdio = options.stdio || ['pipe', process.stdout, 'pipe']
    const childprocess = spawn(cmd, args, options);
    childprocess.on("exit", (data) => {
        callback(data);
    });
});

async function build() {
    // const cmd = "ccache";
    // const args = ["g++"];

    const cmd = "g++";
    const args = [];

    args.push("-o");
    args.push(config.output);

    for (let flag of config.flags) {
        args.push(flag);
    }
    for (let inc of config.includes) {
        args.push("-I");
        args.push(inc);
    }
    for (let link of config.linkdirs) {
        args.push("-L");
        args.push(link);
    }
    for (let file of config.files) {
        args.push(file);
    }
    // link 只能放到最后
    for (let link of config.links) {
        args.push("-l" + link);
    }
    // 输出命令
    let cmd_string = cmd + " ";
    for (let e of args) {
        cmd_string += e + " ";
    }

    console.log("\033[1;36m" + cmd_string + "\033[0m");
    try {
        await exec(cmd, args, {});
    } catch (error) {
        console.log("\033[1;31m" + 'build error' + "\033[0m");
        exit(-1);
    }
};



////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

config.includes = [
    "include",
]

config.linkdirs = [
]

config.links = [
]

config.flags = [
    "-static",
    "-Wno-write-strings",
    "-std=c++14",
    "-g",
    "-finput-charset=UTF-8",
    "-fexec-charset=GBK",
    // "-mwindows", // 不显示控制台
]

config.files = [
    "src/**.cpp",
    "include/quickjs/*.o",
]

config.output = "build/app.exe";

async function run() {
    const time_start = Date.now();

    try {
        await exec("mkdir",["build"], {
        });
    } catch (error) {
    }
    const libs = [
        // "clean",
        ".obj",
        "bjson.o",
        "libbf.o",
        "cutils.o",
        "libunicode.o",
        "libregexp.o",
        "quickjs.o",
        "quickjs-libc.o",
    ]
    for(let it of libs){
        await exec("make",[it], {
            cwd:"include/quickjs"
        });
    }
    await build()
    const time_end = Date.now();
    console.log("\033[1;32m" + "build done,  time used: " + (time_end - time_start) + "ms !" + "\033[0m");
}
run(); 
