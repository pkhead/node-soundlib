const fs = require("fs");

if (fs.existsSync("build/Release/libsoundio.so.2.0.0")) {
    fs.renameSync("build/Release/libsoundio.so.2.0.0", "build/Release/libsoundio.so.2")
}
