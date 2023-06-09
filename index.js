const bindings = require("./build/Release/audiolib.node");

let openDevices = [];

function cleanup() {
    for (let device of openDevices) {
        device._handle.close();
    }
    
    bindings.close();
}

process.on("exit", cleanup);

exports.AudioOutputDevice = class {
    constructor(options) {
        let format = "FORMAT_F32_NE";

        if (options) {
            let endian = "NE";

            if (options.endianness !== undefined) {
                if (options.endianness === "little") endian = "LE";
                else if (options.endianness === "big") endian = "BE";
                else if (options.endianness === "native") endian = "NE";
                else if (options.endianness === "foreign") endian = "FE";
                else {
                    throw new Error(`invalid value for options.endianness, expected "little", "big", "native", or "foreign"`);
                }
            }

            if (options.float) {
                if (options.bitDepth === undefined) options.bitDepth = 32;

                switch (options.bitDepth) {
                    case 32:
                        format = `FORMAT_F32_${endian}`;
                        break;

                    case 64:
                        format = `FORMAT_F64_${endian}`;
                        break;

                    default:
                        throw new Error(`invalid bitDepth for float, expected 32 or 64`);
                }
            } else {
                let sign = "S";
                if (options.signed !== undefined) sign = options.signed ? "S" : "U";
                if (options.bitDepth === undefined) options.bitDepth = 16;

                // 8-bit values have no depth
                if (options.bitDepth === 8) {
                    format = `FORMAT_${sign}8`;
                } else {
                    if (options.bitDepth !== 16 && options.bitDepth !== 24 && options.bitDepth !== 32 && options.bitDepth !== 64) {
                        throw new Error(`invalid bitDepth, expected 8, 16, 24, 32, or 64`);
                    }

                    format = `FORMAT_${sign}${options.bitDepth}_${endian}`;
                }
            }
        }

        var openOptions = {
            sampleRate: options === undefined || options.sampleRate === undefined ? 48000 : options.sampleRate,
            channels: options === undefined || options.channelCount === undefined ? 2 : options.channelCount,
            format: bindings[format]
        };

        this._handle = new bindings.AudioOutputDevice(openOptions);
        this._isClosed = false;
        openDevices.push(this);
    }

    close() {
        if (this._isClosed) throw new Error("OutputDevice already closed");
        this._handle.close();
        this._isClosed = true;
        
        let idx = openDevices.indexOf(this);
        if (idx >= 0) openDevices.splice(idx, 1);
    }

    queue(buffer) {
        return this._handle.queue(buffer);
    }

    get isClosed() {
        return this._isClosed;
    }

    get sampleRate() {
        return this._handle.getSampleRate();
    }

    get channelCount() {
        return this._handle.getChannelCount();
    }
    
    get numFreeFrames() {
        return this._handle.getBytesFree() / (this._handle.getSampleSize() * this._handle.getChannelCount());
    }

    get numQueuedFrames() {
        let bytesQueued = this._handle.getCapacity() - this._handle.getBytesFree();
        return Math.floor(bytesQueued / (this._handle.getSampleSize() * this._handle.getChannelCount()));
    }

    get bitDepth() {
        return this._handle.getSampleSize() * 8;
    }
    
    /*
    get format() {
        throw new Error("todo: get AudioOutputDevice(wrapper).format");
        let format_id = this._handle.getFormat();
        if (format_id == bindings.FORMAT_INVALID) return null;

        let format_str;

        for (let i in bindings) {
            if (i.slice(0, 7) === "FORMAT_") {
                if (bindings[i] === format_id) {
                    format_str = i;
                    break;
                }
            }
        }

        if (!format_str) return null;
        let result = {};

        let sign = format_str.slice(7, 8);

        if (sign === "F") {

        }
    }*/
}