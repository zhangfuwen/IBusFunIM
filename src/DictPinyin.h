//
// Created by zhangfuwen on 2022/1/22.
//

#ifndef AUDIO_IME_DICTPINYIN_H
#define AUDIO_IME_DICTPINYIN_H

#include "pinyinime.h"
#include <string>
using namespace std;
extern volatile bool g_pinyin_table;
namespace pinyin {

class DictPinyin {
  public:
      DictPinyin() ;
    unsigned int Search(const basic_string<char> &input);
    basic_string<wchar_t> GetCandidate(int index);
    ~DictPinyin();
};

} // namespace pinyin

#endif // AUDIO_IME_DICTPINYIN_H
