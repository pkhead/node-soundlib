const { exec, execSync } = require("child_process");
const fs = require("fs");

if (!fs.existsSync("build")) fs.mkdirSync("build");
if (!fs.existsSync("build/Release")) fs.mkdirSync("build/Release");

const AUDIOLIB_PATH = `bin/${process.platform}_${process.arch}_napi${process.versions.napi}.node`;
const LIBSOUNDIO_PATH =
    process.platform === "win32" ?
    `bin/soundio-${process.platform}_${process.arch}.dll` :
    `bin/libsoundio-${process.platform}_${process.arch}.so.2`; 

// if a precompiled binary exists
if (fs.existsSync(AUDIOLIB_PATH)) {
    fs.copyFileSync(AUDIOLIB_PATH, "build/Release/audiolib.node");

    if (process.platform === "win32") {
        fs.copyFileSync(LIBSOUNDIO_PATH, "build/Release/soundio.dll");
    } else {
        fs.copyFileSync(LIBSOUNDIO_PATH, "build/Release/libsoundio.so.2");
    }
} else {
    console.log("No precompiled binaries found, attempting to compile");
    
    const proc = exec("node build.js && node-gyp rebuild", (err) => {
        if (err) return;

        fs.copyFileSync("build/Release/audiolib.node", AUDIOLIB_PATH);
        
        if (process.platform === "win32") {
            fs.copyFileSync("build/Release/soundio.dll", LIBSOUNDIO_PATH);
        } else {
            fs.renameSync("build/Release/libsoundio.so.2.0.0", "build/Release/libsoundio.so.2");
            fs.copyFileSync("build/Release/libsoundio.so.2", LIBSOUNDIO_PATH);
        }
    });

    proc.stdout.pipe(process.stdout);
    proc.stderr.pipe(process.stderr);
}