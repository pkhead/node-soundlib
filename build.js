const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const CMAKE_OPTIONS = "-DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=off -DBUILD_EXAMPLE_PROGRAMS=off -DBUILD_TESTS=off";

function findExe(name) {
    const envPath = process.env.PATH || "";
    const envExt = process.env.PATHEXT || "";
    const pathDirs = envPath
        .replace(/["]+/g, "")
        .split(path.delimiter)
        .filter(Boolean)

    const extensions = envExt.split(";");
    const candidates = pathDirs.flatMap((d) =>
        extensions.map((ext) => path.join(d, name + ext)));

    for (let filePath of candidates) {
        if (fs.existsSync(filePath) && fs.statSync(filePath).isFile()) return `"${filePath}"`;
    }

    console.error(`${name} not found in PATH`);
    return null;
}

if (!fs.existsSync("libsoundio/build")) {
    fs.mkdirSync("libsoundio/build");
}
process.chdir("libsoundio/build");

if (process.platform === "win32") {
    const cmake = findExe("cmake");
    const make = findExe("nmake");

    if (cmake && make) {
        execSync(`${cmake} ${CMAKE_OPTIONS} -G "NMake Makefiles" ..`);
        execSync(`${make}`);
    }
} else {
    const cmake = findExe("cmake");
    const make = findExe("make");

    if (cmake && make) {
        execSync(`cmake ${CMAKE_OPTIONS} ..`);
        execSync("make");
    }
}
