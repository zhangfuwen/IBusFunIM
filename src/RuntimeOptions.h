//
// Created by zhangfuwen on 2022/1/27.
//

#ifndef AUDIO_IME_RUNTIMEOPTIONS_H
#define AUDIO_IME_RUNTIMEOPTIONS_H

#include <string>
#include <ibus.h>

class SpeechRecognizerOptions {
public:
    std::string speechAkId;
    std::string speechSecret;
};

class RuntimeOptions : public SpeechRecognizerOptions {
public:
    static RuntimeOptions *get() {
        if(instance == nullptr) {
            instance = new RuntimeOptions();
        }
        return instance;
    }
    std::string wubi_table;
    bool pinyin = true;
    bool speech = true;

    bool capsOn = false;
    bool speechOn = false;

    volatile bool dictFastEnabled = true;
    volatile bool dictFastPendingReload = false;

    IBusOrientation lookupTableOrientation = IBUS_ORIENTATION_SYSTEM;

    static RuntimeOptions *instance;
};

#endif // AUDIO_IME_RUNTIMEOPTIONS_H
