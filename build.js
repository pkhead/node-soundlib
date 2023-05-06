const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const CMAKE_OPTIONS = "-DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=off -DBUILD_EXAMPLE_PROGRAMS=off -DBUILD_TESTS=off";

if (!fs.existsSync("libsoundio/build")) {
    fs.mkdirSync("libsoundio/build");
}
process.chdir("libsoundio/build");

if (process.platform === "win32") {
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

    const cmake = findExe("cmake", "CMAKE");
    const make = findExe("nmake", "MAKE");

    if (cmake && make) {
        execSync(`${cmake} ${CMAKE_OPTIONS} -G "NMake Makefiles" ..`);
        execSync(`${make}`);
    }
} else {
    if (!fs.existsSync("cmake")) throw new Error("CMake not found in $PATH");
    if (!fs.existsSync("make")) throw new Error("make not found in $PATH");
    execSync(`cmake ${CMAKE_OPTIONS} ..`);
    execSync("make");
}
