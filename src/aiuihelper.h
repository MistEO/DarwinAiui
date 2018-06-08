#pragma once

#include "jsoncpp/json/json.h"
#include <aiui/AIUI.h>
#include <pthread.h>
#include <string>
#include <unistd.h>

#include "aiuilistener.h"

class AiuiHelper {
public:
    //IAIUI参数
    const char* AiuiDir = "./AIUI/";
    const char* ConfigPath = "./AIUI/config/aiui.cfg";
    const char* LogDir = "./AIUI/log";
    //IAT参数
    const char* SessionBeginParams = "sub = iat, domain = iat, language = zh_cn, "
                                     "accent = mandarin, sample_rate = 16000, "
                                     "result_type = plain, result_encoding = utf8";
    const char* LoginParams = "appid = 591acb84, work_dir = .";
    const uint BufferSize = 4096;
    //TTS参数
    const char* TtsSessionBeginParams = "voice_name = xiaoyan, text_encoding = utf8, sample_rate = 16000, speed = 50, volume = 50, pitch = 50, rdn = 2";
    const char* WavFilename = "tts.wav";

    ~AiuiHelper();

    static AiuiHelper& ins();

    void start(const std::string & ip = "127.0.0.1", const std::string & user = "robotis");

    int resource_socket_fd = -1;

    //Don't manual call those function
    void _on_speech_begin();
    void _on_speech_end(int reason);
    //该函数可能会被调用多次
    void _on_speech_result(const char* result, char is_last);

private:
    enum UiState {
        State_Idle,
        State_Working,
        State_Speech,
        State_Write,
        State_Exec,
        State_Finished
    };
    AiuiHelper();
    friend class AiuiListener;
    void _create_agent();
    void _wake_up();
    void _wait_for_working() const;
    void _destroy();
    void _speech_transform(int record_sec = 5);
    void _file_transform(const std::string& filename);
    void _wait_for_finished() const;
    void _write_text(const std::string& text);
    void _result_parse(const VA::Json::Value& data) const;
    void _text_to_speech(const std::string& text) const;
    bool _order_match(const std::string& text) const;
    void _download_and_play_audio(const std::string& url) const;
    void _request_audio(const std::string& filename) const;
    std::string _request_mic() const;

    static AiuiHelper unique_instance;

    aiui::IAIUIAgent* _agent = nullptr;
    AiuiListener _listener;
    char* _speech_result = nullptr;
    uint _speech_buffersize = BufferSize;
    mutable UiState _state = State_Idle;
    std::string ip;
    std::string user;
};
