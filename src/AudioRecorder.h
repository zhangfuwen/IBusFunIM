//
// Created by zhangfuwen on 23-9-15.
//

#ifndef IBUS_FUN_AUDIORECORDER_H
#define IBUS_FUN_AUDIORECORDER_H

#include <iostream>
#include <functional>
#include <alsa/asoundlib.h>

class AudioRecorder {
private:
    snd_pcm_t *handle;
    std::string deviceName;
    unsigned int sampleRate;
    snd_pcm_uframes_t frames;
    std::function<void(float*, int)> on_frame = nullptr;

public:
    void setOnFrame(const std::function<void(float *, int)> &onFrame);

public:
    AudioRecorder(const std::string& device, unsigned int rate, unsigned int channels, unsigned int bufferFrames);

    ~AudioRecorder();

    void start_recording();
};
#endif // IBUS_FUN_AUDIORECORDER_H
