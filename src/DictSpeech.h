//
// Created by zhangfuwen on 2022/1/22.
//

#ifndef AUDIO_IME_DICTSPEECH_H
#define AUDIO_IME_DICTSPEECH_H

#include "RuntimeOptions.h"
#include <csignal>
#include <ibus.h>
#include <nlsEvent.h>
#include <string>
#include <thread>

#define BUFSIZE 1024
#define SAMPLE_RATE 16000
#define DEFAULT_STRING_LEN 128

class SpeechListener {
public:
    virtual void OnCompleted(std::string text) = 0;
    virtual void OnFailed() = 0;
    virtual void OnPartialResult(std::string text) = 0;
    virtual void IBusUpdateIndicator(long) = 0;
    virtual ~SpeechListener() = default;
    ;
};
class DictSpeech {
public:
    enum Status { IDLE, RECODING, WAITING };
    explicit DictSpeech(SpeechListener *listener, SpeechRecognizerOptions *opts)
        : m_speechListerner(listener), m_opts(opts) {}
    void Start() {
        m_recording = true;
        auto ret = RecognitionPrepareAndStartRecording();
        if (ret < 0) {
            m_recording = false;
            m_speechListerner->OnFailed();
        }
    }
    void Stop() { m_recording = false; }

    Status GetStatus() const {
        if (m_recording) {
            return RECODING;
        }
        if (m_waiting) {
            return WAITING;
        }
        return IDLE;
    }

private:
#define BUFSIZE 1024
#define FRAME_100MS 3200
#define SAMPLE_RATE 16000
#define DEFAULT_STRING_LEN 128
    volatile bool m_recording = false; // currently m_recording user voice
    volatile long recordingTime = 0;   // can only record 60 seconds;
    volatile bool m_waiting = false;   // m_waiting for converted text from internet
    SpeechListener *m_speechListerner;
    int frame_size = FRAME_100MS;
    int encoder_type = ENCODER_NONE;
    std::string g_token;
    long g_expireTime = 0;
    SpeechRecognizerOptions *m_opts = nullptr;

    /**
     * 全局维护一个服务鉴权token和其对应的有效期时间戳，
     * 每次调用服务之前，首先判断token是否已经过期，
     * 如果已经过期，则根据AccessKey ID和AccessKey Secret重新生成一个token，
     * 并更新这个全局的token和其有效期时间戳。
     *
     * 注意：不要每次调用服务之前都重新生成新token，
     * 只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
     */
    // 自定义线程参数

    struct ParamStruct {
        char token[DEFAULT_STRING_LEN];
        char appkey[DEFAULT_STRING_LEN];
        char url[DEFAULT_STRING_LEN];
    };

    // 自定义事件回调参数
    struct ParamCallBack {
    public:
        explicit ParamCallBack(ParamStruct *param) {
            tParam = param;
            pthread_mutex_init(&mtxWord, nullptr);
            pthread_cond_init(&cvWord, nullptr);
        };

        ~ParamCallBack() {
            tParam = nullptr;
            pthread_mutex_destroy(&mtxWord);
            pthread_cond_destroy(&cvWord);
        };

        unsigned long userId{}; // 这里用线程号
        char userInfo[8]{};

        pthread_mutex_t mtxWord{};
        pthread_cond_t cvWord{};

        ParamStruct *tParam;
    };
    int RecognitionPrepareAndStartRecording();
    int NetGenerateToken(const std::string &akId, const std::string &akSecret, std::string *token, long *expireTime);
    void OnRecognitionStarted(AlibabaNls::NlsEvent *cbEvent, void *cbParam);
    void OnRecognitionResultChanged(AlibabaNls::NlsEvent *cbEvent, [[maybe_unused]] void *cbParam);
    void OnRecognitionCompleted(AlibabaNls::NlsEvent *cbEvent, void *cbParam);
    void OnRecognitionTaskFailed(AlibabaNls::NlsEvent *cbEvent, void *cbParam);
    void OnRecognitionChannelClosed(AlibabaNls::NlsEvent *cbEvent, void *cbParam);
    int RecognitionRecordAndRequest(ParamStruct *tst);
};
#endif // AUDIO_IME_DICTSPEECH_H
