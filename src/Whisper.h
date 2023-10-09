//
// Created by zhangfuwen on 23-9-15.
//

#ifndef IBUS_FUN_WHISPER_H
#define IBUS_FUN_WHISPER_H

#include <string>
#include "whisper.h"

class Whisper {
public:
    Whisper() {
        ctx = whisper_init_from_file("/usr/share/ibus-table/data/ggml-medium.bin");
    }
public:
    std::string recognize(float *buf, int len);
    whisper_context *ctx = nullptr;
};

#endif // IBUS_FUN_WHISPER_H
