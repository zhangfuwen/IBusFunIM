//
// Created by zhangfuwen on 23-9-15.
//

#include "AudioRecorder.h"
#include <iostream>
#include <alsa/asoundlib.h>


//int main() {
//    // Create an instance of the AudioRecorder class
//    AudioRecorder recorder("default", 44100, 2, 1024);
//
//    // Start recording audio
//    recorder.start_recording();
//
//    return 0;
//}
void AudioRecorder::start_recording() {
    if (!handle) {
        std::cerr << "Audio device is not available." << std::endl;
        return;
    }
    printf("handle:%d\n", handle);
    printf("frames %d\n", frames);

    float buffer[frames];
    int err;
    while (true) {
        if ((err = snd_pcm_readi(handle, buffer, frames/2)) != (int)frames/2) {
            std::cerr << "Read error: " << snd_strerror(err) << std::endl;
        }
        if (on_frame) {
            on_frame(buffer, frames);
        }

        // Process the recorded audio data here...
        // For example, you can write the data to a file or do real-time analysis.

        // To stop recording, you can add a condition or trigger an event.
    }
}
AudioRecorder::~AudioRecorder() {
    if (handle) {
        snd_pcm_close(handle);
    }
}
AudioRecorder::AudioRecorder(
    const std::string &device,
    unsigned int rate,
    unsigned int channels,
    unsigned int bufferFrames)
    :
    handle(nullptr), deviceName(device), sampleRate(rate), frames(bufferFrames) {
    int err;
    if ((err = snd_pcm_open(&handle, deviceName.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        std::cerr << "Cannot open audio device " << deviceName << ": " << snd_strerror(err) << std::endl;
        handle = nullptr;
        return;
    }

    if ((err = snd_pcm_set_params(handle, SND_PCM_FORMAT_FLOAT, SND_PCM_ACCESS_RW_INTERLEAVED,
                                  channels, sampleRate, 1, 500000)) < 0) {
        std::cerr << "Cannot set audio parameters: " << snd_strerror(err) << std::endl;
        snd_pcm_close(handle);
        handle = nullptr;
        return;
    }
}
void AudioRecorder::setOnFrame(const std::function<void(float *, int)> &onFrame) { on_frame = onFrame; }
