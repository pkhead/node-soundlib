const audiolib = require("../index");

let outputDevice = new audiolib.AudioOutputDevice();
console.log(outputDevice);

setInterval(() => {
    console.log("1 sec");
}, 1000);