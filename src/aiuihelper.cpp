#include "aiuihelper.h"

#include <regex>
#include <string>
#include <sys/socket.h>

#include "fileutil.h"
#include "jsoncpp/json/json.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "qisr.h"
#include "requestmessage.h"
#include "speech_recognizer.h"
#include "tts_sample.h"

using namespace std;
using namespace VA;
using namespace aiui;

AiuiHelper AiuiHelper::unique_instance;

AiuiHelper& AiuiHelper::ins()
{
    return unique_instance;
}

void AiuiHelper::start()
{
    _speech_transform();
    _wait_for_finished();
}

AiuiHelper::AiuiHelper()
{
    AIUISetting::setAIUIDir(AiuiDir);
    AIUISetting::initLogger(LogDir);
    MSPLogin(NULL, NULL, LoginParams);
    _create_agent();
    _wait_for_working();
}

AiuiHelper::~AiuiHelper()
{
    MSPLogout();
    _destroy();
}

void AiuiHelper::_create_agent()
{
    string appid = "591acb84";
    Json::Value paramJson;
    Json::Value appidJson;

    appidJson["appid"] = appid;

    string fileParam = FileUtil::readFileAsString(ConfigPath);
    Json::Reader reader;
    if (reader.parse(fileParam, paramJson, false)) {
        paramJson["login"] = appidJson;
        Json::FastWriter writer;
        string paramStr = writer.write(paramJson);
        _agent = IAIUIAgent::createAgent(paramStr.c_str(), &_listener);
    } else {
        cout << ConfigPath << " has something wrong!" << endl;
    }
}

void AiuiHelper::_wake_up()
{
    if (_agent) {
        IAIUIMessage* wakeupMsg = IAIUIMessage::create(AIUIConstant::CMD_WAKEUP);
        _agent->sendMessage(wakeupMsg);
        wakeupMsg->destroy();
    }
}

void AiuiHelper::_wait_for_working() const
{
    while (_listener.get_state() != AIUIConstant::STATE_WORKING) {
        usleep(10 * 1000);
    }
    sleep(1);
    _state = State_Working;
}

void AiuiHelper::_destroy()
{
    if (_agent) {
        _agent->destroy();
        _agent = nullptr;
    }
    _state = State_Idle;
}

/* demo recognize the audio from microphone */
void AiuiHelper::_speech_transform(int record_sec)
{
    _state = State_Speech;
    int errcode;
    struct speech_rec iat;

    errcode = sr_init(&iat, SessionBeginParams, SR_MIC, this);
    if (errcode) {
        printf("speech recognizer init failed\n");
        return;
    }

    errcode = sr_start_listening(&iat);
    if (errcode) {
        printf("start listen failed %d\n", errcode);
    }

    /* 'record_sec' seconds recording */
    sleep(record_sec);

    errcode = sr_stop_listening(&iat);
    if (errcode) {
        printf("stop listening failed %d\n", errcode);
    }

    sr_uninit(&iat);
}

void AiuiHelper::_wait_for_finished() const
{
    while (_state != State_Finished) {
        usleep(10 * 1000);
    }
}

void AiuiHelper::_on_speech_begin()
{
    if (_speech_result) {
        free(_speech_result);
    }
    _speech_result = (char*)malloc(BufferSize);
    _speech_buffersize = BufferSize;
    memset(_speech_result, 0, _speech_buffersize);
    printf("Start Listening...\n");
}

void AiuiHelper::_on_speech_end(int reason)
{
    if (reason == END_REASON_VAD_DETECT)
        printf("\nSpeaking done \n");
    else
        printf("\nRecognizer error %d\n", reason);

    if (!_order_match(_speech_result))
        _write_text(_speech_result);
}

void AiuiHelper::_on_speech_result(const char* result, char is_last)
{
    if (result) {
        size_t left = _speech_buffersize - 1 - strlen(_speech_result);
        size_t size = strlen(result);
        if (left < size) {
            _speech_result = (char*)realloc(_speech_result, _speech_buffersize + BufferSize);
            if (_speech_result)
                _speech_buffersize += BufferSize;
            else {
                printf("mem alloc failed\n");
                return;
            }
        }
        strncat(_speech_result, result, size);
        printf("\rResult: [ %s ]", _speech_result);
        if (is_last)
            putchar('\n');
    }
}

bool AiuiHelper::_order_match(const std::string& text) const
{
    if (text.find("别说了") != -1) {
        RequestMessage request_message;
        request_message.request_type = "GET";
        request_message.resource_type = "/stop_audio";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        return true;
    }
    return false;
}

void AiuiHelper::_write_text(const std::string& text)
{
    if (text.empty()) {
        _state = State_Finished;
        return;
    }
    _state = State_Write;
    if (_agent) {
        // std::cout << "call write" << std::endl;
        // textData内存会在Message在内部处理完后自动release掉
        Buffer* textData = Buffer::alloc(text.length());
        text.copy((char*)textData->data(), text.length());

        IAIUIMessage* writeMsg = IAIUIMessage::create(AIUIConstant::CMD_WRITE,
            0, 0, "data_type=text", textData);

        _agent->sendMessage(writeMsg);
        writeMsg->destroy();
    }
}

void AiuiHelper::_result_parse(const VA::Json::Value& data) const
{
    _state = State_Exec;
    std::cout << data << std::endl;
    std::string service = data["intent"]["service"].asString();

    if (service == "openQA") {
        _text_to_speech(data["intent"]["answer"]["text"].asString());
    } else if (service == "joke") {
        _text_to_speech(data["intent"]["data"]["result"][0]["content"].asString());
    } else if (service == "news") {
        _download_and_play_audio(
            data["intent"]["data"]["result"][0]["url"].asString());
    } else if (service == "story") {
        _download_and_play_audio(
            data["intent"]["data"]["result"][0]["playUrl"].asString());
    } else if (service == "crossTalk") {
        _download_and_play_audio(
            data["intent"]["data"]["result"][0]["url"].asString());
    } else {
        if (!data["intent"]["answer"]["text"].isNull()) {
            _text_to_speech(data["intent"]["answer"]["text"].asString());
        } else {
            _text_to_speech("啊哦~~这个问题太难了，换个问题吧！");
        }
    }
    _state = State_Finished;
}

void AiuiHelper::_text_to_speech(const std::string& text) const
{
    int ret = MSP_SUCCESS;
    std::cout << text << std::endl;
    ret = text_to_speech(text.c_str(), WavFilename, TtsSessionBeginParams);
    if (MSP_SUCCESS != ret) {
        printf("text_to_speech failed, error code: %d.\n", ret);
    }
    _request_audio(WavFilename);
    // play_sound(WavFilename, "/dev/snd/controlC2");
    // system((std::string("play ") + WavFilename + " > /dev/null 2>&1").c_str());
}

void AiuiHelper::_download_and_play_audio(const std::string& url) const
{
    _text_to_speech("让我想一想，请稍等一下~");
    system(std::string("wget -q '" + url + "' -O net_audio").c_str());
    _request_audio("net_audio");
    // if (format == ".m4a")
    // {
    //     // commond play not support m4a file
    //     system("avconv -i net_audio.m4a net_audio.mp3 > /dev/null 2>&1");
    // }
    // system("play net_audio.mp3 > /dev/null 2>&1");
}

void AiuiHelper::_request_audio(const std::string& filename) const
{
    RequestMessage request_message;
    request_message.request_type = "POST";
    request_message.resource_type = "/audio";
    char cwd[1024];
    getcwd(cwd, 1024);
    request_message.header_map["FilePath"] = std::string() + cwd + "/" + filename;
    send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
}