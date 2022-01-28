//
// Created by zhangfuwen on 2022/1/25.
//

#ifndef AUDIO_IME_COMMON_H
#define AUDIO_IME_COMMON_H
#include <ibus.h>
#include <glib/gi18n.h>
#include <locale.h>

struct CandidateAttr {
    bool _isPinyin;
    explicit CandidateAttr(bool isPinyin = false) :_isPinyin(isPinyin) {}
};


#define CONF_SECTION "engine/ibus_fun"
#define CONF_NAME_ID "access_id"  // no captal letter allowed
#define CONF_NAME_SECRET "access_secret" // no captal letter allowed
#define CONF_NAME_WUBI "wubi_table"
#define CONF_NAME_PINYIN "pinyin_enable"
#define CONF_NAME_SPEECH "speech_enable"
#define CONF_NAME_ORIENTATION "orientation"

#define GETTEXT_PACKAGE "messages"

static const char* get_ibus_fun_user_data_dir(char *path) {
    const char* home = getenv("HOME");
    strcpy(path, home);
    strcat(path, "/.config/ibus/fun");
    return path;
}

#endif // AUDIO_IME_COMMON_H
