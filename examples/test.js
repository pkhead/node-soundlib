const audiolib = require("../index");

let outputDevice = new audiolib.AudioOutputDevice();
const FREQ = 440;
let time = 0;

const BUF_SIZE = 512;
let buf = new Float32Array(512 * outputDevice.channelCount);

setInterval(() => {
    while (outputDevice.numFreeFrames > BUF_SIZE) {
        for (let i = 0; i < buf.length; i += 2) {
            let val = 0.1 * Math.sin(time);
            buf[i] = val;
            buf[i + 1] = val;

            time += (FREQ * 2 * Math.PI) / outputDevice.sampleRate;
        }
        
        outputDevice.queue(buf.buffer);
    }
}, 100);