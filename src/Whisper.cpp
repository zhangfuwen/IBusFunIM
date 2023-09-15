//
// Created by zhangfuwen on 23-9-15.
//

#include "Whisper.h"
std::string Whisper::recognize(float *buf, int len) {
    auto ctx = whisper_init_from_file("/usr/share/ibus-table/data/ggml-medium.bin");

    auto wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    if (whisper_full(ctx, wparams, buf, len) != 0) {
        fprintf(stderr, "failed to process audio\n");
        return "7";
    }

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        printf("%s", text);
    }

    whisper_free(ctx);
    return "";
}
