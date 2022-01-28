//
// Created by zhangfuwen on 2022/1/27.
//

#ifndef AUDIO_IME_IBUS_FUN_ENGINE_H
#define AUDIO_IME_IBUS_FUN_ENGINE_H

#include  <ibus.h>

#define IBUS_TYPE_FUN_ENGINE \
    (ibus_fun_engine_get_type())

GType ibus_fun_engine_get_type(void);

#endif // AUDIO_IME_FUNENGINE_H
