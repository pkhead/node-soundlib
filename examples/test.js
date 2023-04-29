const audiolib = require("../index");

let outputDevice = new audiolib.AudioOutputDevice();
const FREQ = 440;

setInterval(() => {
    let time = 0;

    while (outputDevice.numFreeFrames > 256) {
        let arr = new Float32Array(256 * outputDevice.channelCount);

        for (let i = 0; i < arr.length; i += 2) {
            let val = 0.1 * Math.sin(time);
            arr[i] = val;
            arr[i + 1] = val;

            time += (FREQ * 2 * Math.PI) / outputDevice.sampleRate;
        }
        
        outputDevice.queue(arr.buffer);
        
    }
}, 100);