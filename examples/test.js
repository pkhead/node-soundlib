const audiolib = require("../index");

let outputDevice = new audiolib.AudioOutputDevice();

setInterval(() => {
    while (outputDevice.numFreeSamples > 256 * outputDevice.channelCount) {
        let arr = new Float32Array(256 * outputDevice.channelCount);

        for (let i = 0; i < arr.length; i += 2) {
            let val = 0.1 * (Math.random() * 2 - 1);
            arr[i] = val;
            arr[i + 1] = val;
        }

        outputDevice.queue(arr.buffer);
    }
}, 16);