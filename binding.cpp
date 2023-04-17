#include <napi.h>
#include <soundio/soundio.h>
#include <sstream>
#include <iostream>
#include <random>
#include <stack>
#include <mutex>

using namespace Napi;

static SoundIo* soundio;

struct OutputDeviceStruct {
    Napi::ObjectReference jsRef;
};

// Buffer size measured in frames
constexpr size_t BUFFER_SIZE = 512;

class AudioOutputDevice : public Napi::ObjectWrap<AudioOutputDevice> {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
        AudioOutputDevice(const Napi::CallbackInfo& info);
        ~AudioOutputDevice();
        static Napi::Value New(const Napi::CallbackInfo& info);
        Napi::ThreadSafeFunction jsCallback;
        bool callbackSet;

        bool popQueue(uint8_t* &out, size_t &size);

    private:
        SoundIoOutStream* outStream;
        SoundIoDevice* device;
        bool isClosed;
        std::stack<uint8_t*> blockQueue;
        std::mutex queueMutex;

        Napi::Value Close(const Napi::CallbackInfo& info);
        // Napi::Value SetCallback(const Napi::CallbackInfo& info);
        // Napi::Value GetCallback(const Napi::CallbackInfo& info);

        Napi::Value GetSampleRate(const Napi::CallbackInfo& info);
        Napi::Value GetFormat(const Napi::CallbackInfo& info);
        Napi::Value GetChannelCount(const Napi::CallbackInfo& info);

        Napi::Value Queue(const Napi::CallbackInfo& info);
        Napi::Value GetSamplesPerBlock(const Napi::CallbackInfo& info);
        Napi::Value GetQueueSize(const Napi::CallbackInfo& info);

};

static void writeCallback(SoundIoOutStream* outStream, int frameCountMin, int frameCountMax) {
    const SoundIoChannelLayout* layout = &outStream->layout;
    struct SoundIoChannelArea* areas;
    int framesLeft = frameCountMin;
    int err;

    size_t frameSize = soundio_get_bytes_per_frame(outStream->format, layout->channel_count);

    AudioOutputDevice* data = (AudioOutputDevice*)outStream->userdata;

    while (framesLeft > 0) {
        int frameCount = framesLeft;

        if ((err = soundio_outstream_begin_write(outStream, &areas, &frameCount))) {
            printf("err soundio_outstream_begin_write: %s\n", soundio_strerror(err));
            return;
        }

        if (!frameCount) break;

        size_t offset = 0;
        
        uint8_t* block;
        size_t blockSize;

        if (data->popQueue(block, blockSize)) {
            memcpy(areas[0].ptr + offset, block, blockSize);
            offset += blockSize;
        } else {
            printf("queue empty!\n");
        }

        // if buffer isn't completely filled, set remaining samples to 0
        if (offset < frameCount * frameSize) {
            memset(areas[0].ptr + offset, 0, frameCount * frameSize - offset);
        }

        if ((err = soundio_outstream_end_write(outStream))) {
            printf("err soundio_outstream_end_write: %s\n", soundio_strerror(err));
            return;
        }

        framesLeft -= frameCount;
    }
}

Napi::Object AudioOutputDevice::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "AudioOutputDevice", {
        InstanceMethod<&AudioOutputDevice::Close>("close", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),

        InstanceMethod<&AudioOutputDevice::GetSampleRate>("getSampleRate", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceMethod<&AudioOutputDevice::GetFormat>("getFormat", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceMethod<&AudioOutputDevice::GetChannelCount>("getChannelCount", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        
        InstanceMethod<&AudioOutputDevice::Queue>("queue", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceMethod<&AudioOutputDevice::GetSamplesPerBlock>("getSamplesPerBlock", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceMethod<&AudioOutputDevice::GetQueueSize>("getQueueSize", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    exports.Set("AudioOutputDevice", func);

    env.SetInstanceData<Napi::FunctionReference>(constructor);

    return exports;
}

AudioOutputDevice::AudioOutputDevice(const Napi::CallbackInfo& info) : Napi::ObjectWrap<AudioOutputDevice>(info) {
    Napi::Env env = info.Env();

    int sampleRate = 48000;
    SoundIoFormat format = SoundIoFormatFloat32NE;
    int numChannels = 2;

    if (info[0].IsObject()) {
        Napi::Object options = info[0].As<Napi::Object>();
        
        if (options.Has("sampleRate")) sampleRate = options.Get("sampleRate").As<Napi::Number>().Int32Value();
        if (options.Has("channels")) numChannels = options.Get("channels").As<Napi::Number>().Int32Value();
        if (options.Has("format")) format = static_cast<SoundIoFormat>(options.Get("format").As<Napi::Number>().Int32Value());
    }
    
    int defaultOutDeviceIndex = soundio_default_output_device_index(soundio);
    if (defaultOutDeviceIndex < 0) {
        Napi::Error::New(env, "error on soundio_default_output_device_index").ThrowAsJavaScriptException();
        return;
    }

    device = soundio_get_output_device(soundio, defaultOutDeviceIndex);
    if (!device) {
        Napi::Error::New(info.Env(), "error on soundio_get_output_device").ThrowAsJavaScriptException();
        return;
    }

    const SoundIoChannelLayout* layout = soundio_channel_layout_get_default(numChannels);
    
    outStream = soundio_outstream_create(device);
    outStream->sample_rate = sampleRate;
    outStream->layout = soundio_device_supports_layout(device, layout) ? *layout : device->layouts[0];
    outStream->format = format;
    outStream->userdata = this;
    outStream->write_callback = writeCallback;

    int err;
    if ((err = soundio_outstream_open(outStream))) {
        std::stringstream msg;
        msg << "unable to open device: " << soundio_strerror(err);
        Napi::Error::New(info.Env(), msg.str()).ThrowAsJavaScriptException();
        return;
    }

    if (outStream->layout_error) {
        std::stringstream msg;
        msg << "unable to set channel layout: " << soundio_strerror(outStream->layout_error);
        Napi::Error::New(info.Env(), msg.str()).ThrowAsJavaScriptException();
        return;
    }
    
    if ((err = soundio_outstream_start(outStream))) {
        std::stringstream msg;
        msg << "unable to start device: " << soundio_strerror(err);
        Napi::Error::New(info.Env(), msg.str()).ThrowAsJavaScriptException();
        return;
    }

    isClosed = false;
}

AudioOutputDevice::~AudioOutputDevice() {
    if (!isClosed) {
        soundio_outstream_destroy(outStream);
        soundio_device_unref(device);
    }
}

bool AudioOutputDevice::popQueue(uint8_t* &out, size_t &size) {
    size_t bytesPerFrame = soundio_get_bytes_per_frame(outStream->format, outStream->layout.channel_count);
    size = bytesPerFrame * BUFFER_SIZE;

    queueMutex.lock();

    if (blockQueue.size() > 0) {
        out = blockQueue.top();
        blockQueue.pop();
        queueMutex.unlock();

        return true;
    } else {
        queueMutex.unlock();
        return false;
    }
}

Napi::Value AudioOutputDevice::Close(const Napi::CallbackInfo& info) {
    if (!isClosed) {
        soundio_outstream_destroy(outStream);
        soundio_device_unref(device);
        isClosed = true;
    }

    return info.Env().Undefined();
}

/*
Napi::Value AudioOutputDevice::SetCallback(const Napi::CallbackInfo& info) {
    // Napi::Env env = info.Env();
    // Napi::Value callback = info[0];

    // if (callback.IsFunction()) {
    //     jsCallback = Napi::ThreadSafeFunction::New(env, callback.As<Napi::Function>(), "Callback", 0, 1);
    //     callbackSet = true;
    //     return env.Null();
    // } else {
    //     Napi::TypeError::New(env, "AudioOutputDevice.setCallback: Argument 1 is not a function.").ThrowAsJavaScriptException();
    //     callbackSet = false;
    //     return env.Null();
    // }
    return info.Env().Undefined();
}
*/

Napi::Value AudioOutputDevice::Queue(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    size_t bytesPerFrame = soundio_get_bytes_per_frame(outStream->format, outStream->layout.channel_count);

    if (bytesPerFrame <= 0) {
        Napi::Error::New(env, "AudioOutputDevice.queue: invalid format").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info[0].IsArrayBuffer()) {
        Napi::ArrayBuffer buf = info[0].As<ArrayBuffer>();

        if (buf.ByteLength() != bytesPerFrame * BUFFER_SIZE) {
            Napi::RangeError::New(env, "AudioOutputDevice.queue: ArrayBuffer.byteLength != samplesPerBlock * bytesPerFrame").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        // copy ArrayBuffer source to a new block
        uint8_t* block = new uint8_t[bytesPerFrame * BUFFER_SIZE];
        memcpy(block, buf.Data(), buf.ByteLength());

        // todo: ...maybe should make this async?
        queueMutex.lock();
        blockQueue.push(block);
        queueMutex.unlock();
    } else {
        Napi::TypeError::New(env, "AudioOutputDevice.queue: Argument 1 is not a Buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    return env.Undefined();
}

Napi::Value AudioOutputDevice::GetSamplesPerBlock(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), BUFFER_SIZE);
}

Napi::Value AudioOutputDevice::GetQueueSize(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), blockQueue.size());
}

/*
Napi::Value AudioOutputDevice::GetCallback(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (callbackSet) return jsCallback.
    if (jsCallback.()) return env.Undefined();
    else return jsCallback.Value();
}
*/

Napi::Value AudioOutputDevice::GetSampleRate(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), outStream->sample_rate);
}

Napi::Value AudioOutputDevice::GetFormat(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), outStream->format);
}

Napi::Value AudioOutputDevice::GetChannelCount(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), outStream->layout.channel_count);
}











void Close(const Napi::CallbackInfo& info) {
    soundio_destroy(soundio);
}

Napi::Object Init(Env env, Object exports) {
    int err;

    soundio = soundio_create();

    if (!soundio) {
        throw Napi::Error::New(env, "error: soundio_create()");
    }

    if ((err = soundio_connect(soundio))) {
        std::stringstream errMessage;
        errMessage << "error connecting: " << soundio_strerror(err);
        throw Napi::Error::New(env, errMessage.str());
    }

    soundio_flush_events(soundio);

    exports.Set("close", Function::New(env, Close));

    // formats for number types and both endians
    exports.Set("FORMAT_INVALID", Napi::Number::New(env, SoundIoFormatInvalid));
    exports.Set("FORMAT_S8", Napi::Number::New(env, SoundIoFormatS8));              ///< Signed 8 bit
    exports.Set("FORMAT_U8", Napi::Number::New(env, SoundIoFormatU8));              ///< Unsigned 8 bit
    exports.Set("FORMAT_S16_LE", Napi::Number::New(env, SoundIoFormatS16LE));       ///< Signed 16 bit Little Endian
    exports.Set("FORMAT_S16_BE", Napi::Number::New(env, SoundIoFormatS16BE));       ///< Signed 16 bit Big Endian
    exports.Set("FORMAT_U16_LE", Napi::Number::New(env, SoundIoFormatU16LE));       ///< Unsigned 16 bit Little Endian
    exports.Set("FORMAT_U16_BE", Napi::Number::New(env, SoundIoFormatU16BE));       ///< Unsigned 16 bit Big Endian
    exports.Set("FORMAT_S24_LE", Napi::Number::New(env, SoundIoFormatS24LE));       ///< Signed 24 bit Little Endian using low three bytes in 32-bit word
    exports.Set("FORMAT_S24_BE", Napi::Number::New(env, SoundIoFormatS24BE));       ///< Signed 24 bit Big Endian using low three bytes in 32-bit word
    exports.Set("FORMAT_U24_LE", Napi::Number::New(env, SoundIoFormatU24LE));       ///< Unsigned 24 bit Little Endian using low three bytes in 32-bit word
    exports.Set("FORMAT_U24_BE", Napi::Number::New(env, SoundIoFormatU24BE));       ///< Unsigned 24 bit Big Endian using low three bytes in 32-bit word
    exports.Set("FORMAT_S32_LE", Napi::Number::New(env, SoundIoFormatS32LE));       ///< Signed 32 bit Little Endian
    exports.Set("FORMAT_S32_BE", Napi::Number::New(env, SoundIoFormatS32BE));       ///< Signed 32 bit Big Endian
    exports.Set("FORMAT_U32_LE", Napi::Number::New(env, SoundIoFormatU32LE));       ///< Unsigned 32 bit Little Endian
    exports.Set("FORMAT_U32_BE", Napi::Number::New(env, SoundIoFormatU32BE));       ///< Unsigned 32 bit Big Endian
    exports.Set("FORMAT_F32_LE", Napi::Number::New(env, SoundIoFormatFloat32LE));   ///< Float 32 bit Little Endian, Range -1.0 to 1.0
    exports.Set("FORMAT_F32_BE", Napi::Number::New(env, SoundIoFormatFloat32BE));   ///< Float 32 bit Big Endian, Range -1.0 to 1.0
    exports.Set("FORMAT_F64_LE", Napi::Number::New(env, SoundIoFormatFloat64LE));   ///< Float 64 bit Little Endian, Range -1.0 to 1.0
    exports.Set("FORMAT_F64_BE", Napi::Number::New(env, SoundIoFormatFloat64BE));   ///< Float 64 bit Big Endian, Range -1.0 to 1.0
    
    // formats for native endianness
    exports.Set("FORMAT_S16_NE", Napi::Number::New(env, SoundIoFormatS16NE));
    exports.Set("FORMAT_U16_NE", Napi::Number::New(env, SoundIoFormatU16NE));
    exports.Set("FORMAT_S24_NE", Napi::Number::New(env, SoundIoFormatS24NE));
    exports.Set("FORMAT_U24_NE", Napi::Number::New(env, SoundIoFormatU24NE));
    exports.Set("FORMAT_S32_NE", Napi::Number::New(env, SoundIoFormatS32NE));
    exports.Set("FORMAT_U32_NE", Napi::Number::New(env, SoundIoFormatU32NE));
    exports.Set("FORMAT_F32_NE", Napi::Number::New(env, SoundIoFormatFloat32NE));
    exports.Set("FORMAT_F64_NE", Napi::Number::New(env, SoundIoFormatFloat64NE));

    // formats for foreign endianness
    exports.Set("FORMAT_S16_FE", Napi::Number::New(env, SoundIoFormatS16FE));
    exports.Set("FORMAT_U16_FE", Napi::Number::New(env, SoundIoFormatU16FE));
    exports.Set("FORMAT_S24_FE", Napi::Number::New(env, SoundIoFormatS24FE));
    exports.Set("FORMAT_U24_FE", Napi::Number::New(env, SoundIoFormatU24FE));
    exports.Set("FORMAT_S32_FE", Napi::Number::New(env, SoundIoFormatS32FE));
    exports.Set("FORMAT_U32_FE", Napi::Number::New(env, SoundIoFormatU32FE));
    exports.Set("FORMAT_F32_FE", Napi::Number::New(env, SoundIoFormatFloat32FE));
    exports.Set("FORMAT_F64_FE", Napi::Number::New(env, SoundIoFormatFloat64FE));

    AudioOutputDevice::Init(env, exports);
    
    return exports;
}

NODE_API_MODULE(addon, Init);