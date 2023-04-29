const audiolib = require("../index");

let outputDevice = new audiolib.AudioOutputDevice({
    float: true,
    bitDepth: 32
});

let FREQ = 440;
let time = 0;

const BUF_SIZE = 512;
const NUM_RANGE = 1;
let buf = new Float32Array(BUF_SIZE * outputDevice.channelCount);

process.stdin.on("data", (chunk) => {
    let msg = chunk.toString().trim();

    if (msg === "a") {
        FREQ /= 2;
    } else if (msg === "s") {
        FREQ *= 2;
    }
});

setInterval(() => {
    while (outputDevice.numQueuedFrames < BUF_SIZE * 5) {
        let max = -Infinity; 
        for (let i = 0; i < buf.length; i += 2) {
            let val = 0.1 * Math.sin(time);
            buf[i] = val * NUM_RANGE;
            buf[i + 1] = val * NUM_RANGE;
            time += (FREQ * 2 * Math.PI) / outputDevice.sampleRate;
        }
        
        outputDevice.queue(buf.buffer);
    }
}, 10);