export interface AudioOutputDeviceOptions {
    endianness?: string,
    float?: boolean,
    bitDepth?: number,
    signed?: boolean,
    sampleRate?: number,
    channels?: number,
}

export class AudioOutputDevice {
    constructor(options?: AudioOutputDeviceOptions)
    close(): void
    queue(buffer: ArrayBuffer): void

    get sampleRate(): number
    get channelCount(): number
    get numFreeFrames(): number
    get numQueuedFrames(): number
    get bitDepth(): number
    get format(): number
    get isClosed(): boolean
}