//
// Created by zhangfuwen on 23-9-15.
//

#include "Whisper.h"
#include "AudioRecorder.h"

int main()
{
    AudioRecorder recorder("default", 44100, 2, 1024);
    Whisper whisper;
    std::vector<float> buf;
    recorder.setOnFrame([&](auto data, auto len) {
        printf("len %d\n", len);
        for(int i = 0; i< len; i++) {
            buf.push_back(data[i]);
        }
        if(buf.size() < 1024*1024) {
            return;
        }
//        std::copy_n(data, len, buf.end());
        printf("buf len %d\n", buf.size());
//        auto ret = whisper.recognize(buf.data(), buf.size()/16);
        std::vector<float> temp;
        temp.resize(buf.size());
        std::copy(buf.begin(), buf.end(), temp.begin());
        auto ret = whisper.recognize(temp.data(), temp.size());
        printf("ret :%s\n", ret.c_str());
        buf.clear();
    });
    recorder.start_recording();

}