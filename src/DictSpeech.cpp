//
// Created by zhangfuwen on 2022/1/22.
//

#include "DictSpeech.h"
#include "common_log.h"
//#include "nlsClient.h"
//#include "nlsEvent.h"
//#include "nlsToken.h"
//#include "speechRecognizerRequest.h"
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <functional>
#include <pthread.h>
#include <pulse/error.h>
#include <pulse/simple.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

using namespace std;

std::string audio_text;

//using SpeechCallbackType = std::function<void(AlibabaNls::NlsEvent *ev, void *cbParam)>;
//static SpeechCallbackType fnOnRecognitionStarted;
//static SpeechCallbackType fnOnRecognitionTaskFailed;
//static SpeechCallbackType fnOnRecognitionChannelClosed;
//static SpeechCallbackType fnOnRecognitionResultChanged;
//static SpeechCallbackType fnOnRecognitionCompleted;

/**
 * 根据AccessKey ID和AccessKey Secret重新生成一个token，并获取其有效期时间戳
// */
//int DictSpeech::NetGenerateToken(const string &akId, const string &akSecret, string *token, long *expireTime) {
//    if (akId.empty() || akSecret.empty()) {
//        FUN_ERROR("akId(%lu) or akSecret(%lu) is empty", akId.size(), akSecret.size());
//        return -1;
//    }
//    AlibabaNlsCommon::NlsToken nlsTokenRequest;
//    nlsTokenRequest.setAccessKeyId(akId);
//    nlsTokenRequest.setKeySecret(akSecret);
//    //  nlsTokenRequest.setDomain("nls-meta-vpc-pre.aliyuncs.com");
//
//    if (-1 == nlsTokenRequest.applyNlsToken()) {
//        FUN_INFO("Failed:%s", nlsTokenRequest.getErrorMsg());
//        return -1;
//    }
//
//    *token = nlsTokenRequest.getToken();
//    *expireTime = nlsTokenRequest.getExpireTime();
//
//    return 0;
//}

/**
 * @brief 调用start(), 成功与云端建立连接, sdk内部线程上报started事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为nullptr, 可以根据需求自定义参数
 * @return
// */
//void DictSpeech::OnRecognitionStarted(AlibabaNls::NlsEvent *cbEvent, void *cbParam) {
//    if (cbParam) {
//        auto *tmpParam = (ParamCallBack *)cbParam;
//        FUN_INFO("OnRecognitionStarted userId:%lu, %s", tmpParam->userId,
//                 tmpParam->userInfo); // 仅表示自定义参数示例
//        //通知发送线程start()成功, 可以继续发送数据
//        pthread_mutex_lock(&(tmpParam->mtxWord));
//        pthread_cond_signal(&(tmpParam->cvWord));
//        pthread_mutex_unlock(&(tmpParam->mtxWord));
//    }
//
//    FUN_INFO("OnRecognitionStarted: status code:%d, task id:%s",
//             cbEvent->getStatusCode(), // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
//             cbEvent->getTaskId()); // 当前任务的task id，方便定位问题，建议输出
//}

/**
 * @brief 设置允许返回中间结果参数, sdk在接收到云端返回到中间结果时,
 *        sdk内部线程上报ResultChanged事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为nullptr, 可以根据需求自定义参数
 * @return
 */
//
//void DictSpeech::OnRecognitionResultChanged(AlibabaNls::NlsEvent *cbEvent, [[maybe_unused]] void *cbParam) {
//    FUN_DEBUG("result changed");
//    m_speechListerner->OnPartialResult(cbEvent->getResult());
//}

/**
 * @brief sdk在接收到云端返回识别结束消息时, sdk内部线程上报Completed事件
 * @note 上报Completed事件之后, SDK内部会关闭识别连接通道.
 *       此时调用sendAudio会返回-1, 请停止发送.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为nullptr, 可以根据需求自定义参数
 * @return
// */
//void DictSpeech::OnRecognitionCompleted(AlibabaNls::NlsEvent *cbEvent, void *cbParam) {
//
//    if (cbParam) {
//        auto *tmpParam = (ParamCallBack *)cbParam;
//        if (!tmpParam->tParam)
//            return;
//        FUN_DEBUG("OnRecognitionCompleted: userId %lu, %s", tmpParam->userId,
//                 tmpParam->userInfo); // 仅表示自定义参数示例
//    }
//
//    FUN_DEBUG("OnRecognitionCompleted: status code:%d, task id:%s, result:%s",
//             cbEvent->getStatusCode(), // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
//             cbEvent->getTaskId(),  // 当前任务的task id，方便定位问题，建议输出
//             cbEvent->getResult()); // 获取中间识别结果
//
//    audio_text = cbEvent->getResult();
//
//    FUN_DEBUG("OnRecognitionCompleted: All response:%s",
//             cbEvent->getAllResponse()); // 获取服务端返回的全部信息
//
//    m_waiting = false;
//    m_recording = false;
//    m_speechListerner->OnCompleted(audio_text);
//}

/**
 * @brief 识别过程发生异常时, sdk内部线程上报TaskFailed事件
 * @note 上报TaskFailed事件之后, SDK内部会关闭识别连接通道.
 *       此时调用sendAudio会返回-1, 请停止发送.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为nullptr, 可以根据需求自定义参数
 * @return
 */
//
//void DictSpeech::OnRecognitionTaskFailed(AlibabaNls::NlsEvent *cbEvent, void *cbParam) {
//    if (cbParam) {
//        auto *tmpParam = (ParamCallBack *)cbParam;
//        FUN_INFO("taskFailed userId:%lu, %s", tmpParam->userId,
//                 tmpParam->userInfo); // 仅表示自定义参数示例
//    }
//
//    FUN_INFO("OnRecognitionTaskFailed: status code:%d, task id:%s, error message:%s",
//             cbEvent->getStatusCode(), // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
//             cbEvent->getTaskId(), // 当前任务的task id，方便定位问题，建议输出
//             cbEvent->getErrorMessage());
//
//    FUN_INFO("OnRecognitionTaskFailed: All response:%s",
//             cbEvent->getAllResponse()); // 获取服务端返回的全部信息
//    m_waiting = false;
//    m_speechListerner->OnFailed();
//}

/**
 * @brief 识别结束或发生异常时，会关闭连接通道,
 *        sdk内部线程上报ChannelCloseed事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为nullptr, 可以根据需求自定义参数
 * @return
 */
//
//void DictSpeech::OnRecognitionChannelClosed(AlibabaNls::NlsEvent *cbEvent, void *cbParam) {
//    FUN_INFO("OnRecognitionChannelClosed: All response:%s",
//             cbEvent->getAllResponse()); // 获取服务端返回的全部信息
//    if (cbParam) {
//        auto *tmpParam = (ParamCallBack *)cbParam;
//        FUN_INFO("OnRecognitionChannelClosed CbParam:%lu, %s", tmpParam->userId,
//                 tmpParam->userInfo); // 仅表示自定义参数示例
//
//        //通知发送线程, 最终识别结果已经返回, 可以调用stop()
//        pthread_mutex_lock(&(tmpParam->mtxWord));
//        pthread_cond_signal(&(tmpParam->cvWord));
//        pthread_mutex_unlock(&(tmpParam->mtxWord));
//    }
//}
//
//int DictSpeech::RecognitionRecordAndRequest(ParamStruct *tst) {
//
//    // 0: 从自定义线程参数中获取token, 配置文件等参数.
//    if (tst == nullptr) {
//        FUN_ERROR("arg is not valid.");
//        return -1;
//    }
//
//    //初始化自定义回调参数, 以下两变量仅作为示例表示参数传递,
//    //在demo中不起任何作用
//    //回调参数在堆中分配之后, SDK在销毁requesr对象时会一并销毁, 外界无需在释放
//    auto cbParam = new (nothrow) ParamCallBack(tst);
//    cbParam->userId = pthread_self();
//    strcpy(cbParam->userInfo, "User.");
//
//    /*
//     * 1: 创建一句话识别SpeechRecognizerRequest对象
//     */
//    AlibabaNls::SpeechRecognizerRequest *request = AlibabaNls::NlsClient::getInstance()->createRecognizerRequest();
//    if (request == nullptr) {
//        FUN_ERROR("createRecognizerRequest failed.");
//        return -3;
//    }
//    //        std::function<void(AlibabaNls::NlsEvent *ev, void *cbParam)> fn
//    fnOnRecognitionStarted = bind(&DictSpeech::OnRecognitionStarted, this, placeholders::_1, placeholders::_2);
//    fnOnRecognitionTaskFailed =
//        bind(&DictSpeech::OnRecognitionTaskFailed, this, placeholders::_1, placeholders::_2);
//    fnOnRecognitionChannelClosed =
//        bind(&DictSpeech::OnRecognitionChannelClosed, this, placeholders::_1, placeholders::_2);
//    fnOnRecognitionResultChanged =
//        bind(&DictSpeech::OnRecognitionResultChanged, this, placeholders::_1, placeholders::_2);
//    fnOnRecognitionCompleted =
//        bind(&DictSpeech::OnRecognitionCompleted, this, placeholders::_1, placeholders::_2);
//
//    // 设置start()成功回调函数
//    request->setOnRecognitionStarted([](AlibabaNls::NlsEvent *ev, void *par) { fnOnRecognitionStarted(ev, par); },
//                                     cbParam);
//    // 设置异常识别回调函数
//    request->setOnTaskFailed([](AlibabaNls::NlsEvent *ev, void *par) { fnOnRecognitionTaskFailed(ev, par); }, cbParam);
//    // 设置识别通道关闭回调函数
//    request->setOnChannelClosed([](AlibabaNls::NlsEvent *ev, void *par) { fnOnRecognitionChannelClosed(ev, par); },
//                                cbParam);
//    // 设置中间结果回调函数
//    request->setOnRecognitionResultChanged(
//        [](AlibabaNls::NlsEvent *ev, void *par) { fnOnRecognitionResultChanged(ev, par); }, cbParam);
//    // 设置识别结束回调函数
//    request->setOnRecognitionCompleted([](AlibabaNls::NlsEvent *ev, void *par) { fnOnRecognitionCompleted(ev, par); },
//                                       cbParam);
//
//    // 设置AppKey, 必填参数, 请参照官网申请
//    if (strlen(tst->appkey) > 0) {
//        request->setAppKey(tst->appkey);
//        FUN_DEBUG("setAppKey:%s", tst->appkey);
//    }
//    // 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm
//    request->setFormat("pcm");
//    // 设置音频数据采样率, 可选参数, 目前支持16000, 8000. 默认是16000
//    request->setSampleRate(SAMPLE_RATE);
//    // 设置是否返回中间识别结果, 可选参数. 默认false
//    request->setIntermediateResult(true);
//    // 设置是否在后处理中添加标点, 可选参数. 默认false
//    request->setPunctuationPrediction(true);
//    // 设置是否在后处理中执行ITN, 可选参数. 默认false
//    request->setInverseTextNormalization(true);
//
//    //是否启动语音检测, 可选, 默认是False
//    // request->setEnableVoiceDetection(true);
//    //允许的最大开始静音, 可选, 单位是毫秒,
//    //超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
//    //注意: 需要先设置enable_voice_detection为true
//    // request->setMaxStartSilence(800);
//    //允许的最大结束静音, 可选, 单位是毫秒,
//    //超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
//    //注意: 需要先设置enable_voice_detection为true
//    // request->setMaxEndSilence(800);
//    // request->setCustomizationId("TestId_123"); //定制模型id, 可选.
//    // request->setVocabularyId("TestId_456"); //定制泛热词id, 可选.
//
//    // 设置账号校验token, 必填参数
//    if (strlen(tst->token) > 0) {
//        request->setToken(tst->token);
//        FUN_DEBUG("setToken:%s", tst->token);
//    }
//    if (strlen(tst->url) > 0) {
//        FUN_INFO("setUrl:%s", tst->url);
//        request->setUrl(tst->url);
//    }
//
//    FUN_INFO("begin sendAudio. ");
//
//    /*
//     * 2: start()为异步操作。成功返回started事件。失败返回TaskFailed事件。
//     */
//    FUN_INFO("start ->");
//    struct timespec outtime {};
//    struct timeval now {};
//    int ret = request->start();
//    if (ret < 0) {
//        FUN_ERROR("start failed(%d)", ret);
//        AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);
//        return -4;
//    } else {
//        //等待started事件返回, 在发送
//        FUN_INFO("wait started callback.");
//        gettimeofday(&now, nullptr);
//        outtime.tv_sec = now.tv_sec + 10;
//        outtime.tv_nsec = now.tv_usec * 1000;
//        pthread_mutex_lock(&(cbParam->mtxWord));
//        pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
//        pthread_mutex_unlock(&(cbParam->mtxWord));
//    }
//
//    static const pa_sample_spec ss = {.format = PA_SAMPLE_S16LE, .rate = 16000, .channels = 1};
//    int error;
//
//    FUN_DEBUG("pa_simple_new");
//    pa_simple *s =
//        pa_simple_new(nullptr, "ibus-fun", PA_STREAM_RECORD, nullptr, "record", &ss, nullptr, nullptr, &error);
//    /* Create the m_recording stream */
//    if (!s) {
//        FUN_INFO("pa_simple_new() failed: %s", pa_strerror(error));
//        return -5;
//    }
//
//    struct timeval x {};
//    gettimeofday(&x, nullptr);
//
//    FUN_DEBUG("m_recording %d", m_recording);
//    while (m_recording) {
//        struct timeval y {};
//        gettimeofday(&y, nullptr);
//        auto newRecordingTime = y.tv_sec - x.tv_sec;
//        if (newRecordingTime - recordingTime >= 1) {
//            m_speechListerner->IBusUpdateIndicator(recordingTime);
//        }
//        recordingTime = newRecordingTime;
//        if (recordingTime > 59) {
//            break;
//        }
//        uint8_t buf[BUFSIZE];
//        /* Record some data ... */
//        FUN_DEBUG("reading %d", m_recording);
//        if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
//            FUN_INFO("pa_simple_read() failed: %s", pa_strerror(error));
//            break;
//        }
//        uint8_t data[frame_size];
//        memset(data, 0, frame_size);
//
//        /*
//         * 3: 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败,
//         * 需要停止发送; 返回0 为成功. notice : 返回值非成功发送字节数.
//         *    若希望用省流量的opus格式上传音频数据, 则第三参数传入ENCODER_OPU
//         *    ENCODER_OPU/ENCODER_OPUS模式时,nlen必须为640
//         */
//        FUN_DEBUG("send audio %d", m_recording);
//        ret = request->sendAudio(buf, sizeof(buf), (ENCODER_TYPE)encoder_type);
//        if (ret < 0) {
//            // 发送失败, 退出循环数据发送
//            FUN_ERROR("send data fail(%d)", ret);
//            break;
//        }
//    } // while
//    m_waiting = true;
//    recordingTime = 0;
//
//    /*
//     * 6: 通知云端数据发送结束.
//     * stop()为异步操作.失败返回TaskFailed事件
//     */
//    // stop()后会收到所有回调，若想立即停止则调用cancel()
//    ret = request->stop();
//    FUN_INFO("stop done");
//
//    /*
//     * 6: 通知SDK释放request.
//     */
//    if (ret == 0) {
//        FUN_INFO("wait closed callback.");
//        gettimeofday(&now, nullptr);
//        outtime.tv_sec = now.tv_sec + 10;
//        outtime.tv_nsec = now.tv_usec * 1000;
//        // 等待closed事件后再进行释放, 否则会出现崩溃
//        pthread_mutex_lock(&(cbParam->mtxWord));
//        pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
//        pthread_mutex_unlock(&(cbParam->mtxWord));
//        FUN_INFO("wait closed callback done.");
//    } else {
//        FUN_INFO("stop ret is %d", ret);
//    }
//    AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);
//    return 0;
//}
//
//int DictSpeech::RecognitionPrepareAndStartRecording() {
//    FUN_DEBUG("starting");
//    int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig("log-recognizer", AlibabaNls::LogDebug, 400,
//                                                                 50); //"log-recognizer"
//    if (-1 == ret) {
//        FUN_ERROR("set log failed.");
//        return -1;
//    }
//
//    // 启动工作线程, 在创建请求和启动前必须调用此函数
//    // 入参为负时, 启动当前系统中可用的核数
//    AlibabaNls::NlsClient::getInstance()->startWorkThread();
//
//    ParamStruct tst{};
//    memset(tst.appkey, 0, DEFAULT_STRING_LEN);
//    string appkey = "Y0ueIZ5N4OkyfpUW";
//    string token = "1a9838b31cd5425b80f3f7677697c252";
//    time_t curTime = time(nullptr);
//    if (g_expireTime == 0 || g_expireTime - curTime < 10) {
//        FUN_DEBUG("generating new token %lu %lu", g_expireTime, curTime);
//        if (-1 == NetGenerateToken(m_opts->speechAkId, m_opts->speechSecret, &g_token, &g_expireTime)) {
//            FUN_ERROR("failed to gen token");
//            return -1;
//        }
//    }
//    memset(tst.token, 0, DEFAULT_STRING_LEN);
//    memcpy(tst.token, g_token.c_str(), g_token.length());
//    memcpy(tst.appkey, appkey.data(), appkey.size());
//    tst.appkey[appkey.size()] = '\0';
//
//    memset(tst.url, 0, DEFAULT_STRING_LEN);
//
//    ret = RecognitionRecordAndRequest(&tst);
//    AlibabaNls::NlsClient::getInstance()->releaseInstance();
//    return ret;
//}