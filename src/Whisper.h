//
// Created by zhangfuwen on 23-9-15.
//

#ifndef IBUS_FUN_WHISPER_H
#define IBUS_FUN_WHISPER_H

#include <string>
#include "whisper.h"

class Whisper {
public:
    std::string recognize(float *buf, int len);
};

#endif // IBUS_FUN_WHISPER_H
