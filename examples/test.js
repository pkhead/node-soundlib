const audiolib = require("../index");

let outputDevice = new audiolib.AudioOutputDevice();
console.log(outputDevice.samplesPerBlock);

setInterval(() => {
    console.log("1 sec");

    console.log(outputDevice.queueSize);
    
    while (outputDevice.queueSize < 16) {
        let arr = new Float32Array(outputDevice.samplesPerBlock * outputDevice.channelCount);

        for (let i = 0; i < arr.length; i += 2) {
            let val = 0.1 * (Math.random() * 2 - 1);
            arr[i] = val;
            arr[i + 1] = val;
        }

        outputDevice.queue(arr.buffer);
    }
}, 1000 / 16);