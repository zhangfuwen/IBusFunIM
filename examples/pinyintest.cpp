//
// Created by zhangfuwen on 2022/1/17.
//

#include <cstdio>
#include <cstring>
#include <glib.h>
#include "pinyinime.h"



int main()
{
    ime_pinyin::char16 buffer[40];
    bool ret = ime_pinyin::im_open_decoder("/usr/local/share/dict_pinyin.dat", "/home/zhangfuwen/pinyin.dat");
    if(!ret) {
        printf("failed to open decoder\n");
        return -1;
    }
    int numCandidates = ime_pinyin::im_search("nihao", strlen("nihao"));
    printf("num candidates %d\n", numCandidates);
    for(int j =0; j< numCandidates; j++) {
        auto cand_size = ime_pinyin::im_get_candidate(j, buffer, 40);
        char bu[]="你好";
        glong items_read;
        glong items_written;
        GError *error;
        gunichar * utf32_str = g_utf16_to_ucs4(buffer, (glong)cand_size, &items_read, &items_written, &error);
        printf("utf32(%d):%s\n", items_written, utf32_str);
        char * utf8_str = g_utf16_to_utf8(buffer, (glong)cand_size, &items_read, &items_written, &error);
        printf("utf8(%d):%s\n", items_written, utf8_str);
    }


    return 0;
}