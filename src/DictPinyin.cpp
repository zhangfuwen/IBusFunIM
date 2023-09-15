//
// Created by zhangfuwen on 2022/1/22.
//

#include <functional>
#include "Engine.h"
//#include "speechRecognizerRequest.h"
//#include "nlsToken.h"
//#include "nlsEvent.h"
//#include "nlsClient.h"
#include "common_log.h"
#include <vector>
#include <utility>
#include <unordered_map>
#include <thread>
#include <string>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pthread.h>
#include <pinyinime.h>
#include <map>
#include <iostream>
#include <ibus.h>
#include <glib-object.h>
#include <glib.h>
#include <fstream>
#include <fcntl.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <cctype>
#include "DictPinyin.h"
volatile bool g_pinyin_table = true;
pinyin::DictPinyin::DictPinyin() {
    bool ret = ime_pinyin::im_open_decoder("/usr/share/ibus-table/data/dict_pinyin.dat", (get_ibus_fun_user_data_dir() + "/pinyin.dat").c_str());
}
unsigned int pinyin::DictPinyin::Search(const basic_string<char> &input) {
    auto numCandidates = ime_pinyin::im_search(input.c_str(), input.size());
    return numCandidates;
}
basic_string<wchar_t> pinyin::DictPinyin::GetCandidate(int index) {
    unsigned short buffer[240];
    auto ret = ime_pinyin::im_get_candidate(index, buffer, 240);
    if (ret == nullptr) {
        return {};
    }

    return {(wchar_t *)buffer, 240};
}
pinyin::DictPinyin::~DictPinyin() {
    ime_pinyin::im_close_decoder();
}
