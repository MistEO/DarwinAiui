#include "aiuihelper.h"

#include <malloc.h>
#include <regex>
#include <string>
#include <sys/socket.h>

#include "fileutil.h"
#include "jsoncpp/json/json.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "qisr.h"
#include "requestmessage.h"
#include "responsemessage.h"
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

void AiuiHelper::start(const std::string& ip, const std::string& user)
{
    // _speech_transform();
    std::string filename = _request_mic();
    this->ip = ip;
    this->user = user;
    std::string cpmic_cmd = "scp " + user + "@" + ip + ":" + filename + " " + filename;
    //std::cout << cpmic_cmd << std::endl;
    system(cpmic_cmd.c_str());
    _file_transform(filename);
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

void AiuiHelper::_file_transform(const std::string& filename)
{
    int errcode = 0;
    FILE* f_pcm = NULL;
    char* p_pcm = NULL;
    unsigned long pcm_count = 0;
    unsigned long pcm_size = 0;
    unsigned long read_size = 0;
    int FRAME_LEN = 640;
    struct speech_rec iat;

    if (filename.empty()) {
        return;
    }

    f_pcm = fopen(filename.c_str(), "rb");
    if (NULL == f_pcm) {
        printf("\nopen [%s] failed! \n", filename.c_str());
        fclose(f_pcm);
        f_pcm = NULL;
        return;
    }

    fseek(f_pcm, 0, SEEK_END);
    pcm_size = ftell(f_pcm);
    fseek(f_pcm, 0, SEEK_SET);

    p_pcm = (char*)malloc(pcm_size);
    if (NULL == p_pcm) {
        printf("\nout of memory! \n");
        free(p_pcm);
        p_pcm = NULL;
        sr_stop_listening(&iat);
        sr_uninit(&iat);
        return;
    }

    read_size = fread((void*)p_pcm, 1, pcm_size, f_pcm);
    if (read_size != pcm_size) {
        printf("\nread [%s] error!\n", filename.c_str());
        sr_stop_listening(&iat);
        sr_uninit(&iat);
        return;
    }

    errcode = sr_init(&iat, SessionBeginParams, SR_USER, this);
    if (errcode) {
        printf("speech recognizer init failed : %d\n", errcode);
        sr_stop_listening(&iat);
        sr_uninit(&iat);
        return;
    }

    errcode = sr_start_listening(&iat);
    if (errcode) {
        printf("\nsr_start_listening failed! error code:%d\n", errcode);
        sr_stop_listening(&iat);
        sr_uninit(&iat);
        return;
    }

    while (1) {
        unsigned int len = 10 * FRAME_LEN; /* 200ms audio */
        int ret = 0;

        if (pcm_size < 2 * len)
            len = pcm_size;
        if (len <= 0)
            break;

        ret = sr_write_audio_data(&iat, &p_pcm[pcm_count], len);

        if (0 != ret) {
            printf("\nwrite audio data failed! error code:%d\n", ret);

            sr_stop_listening(&iat);
            sr_uninit(&iat);
            return;
        }

        pcm_count += (long)len;
        pcm_size -= (long)len;
    }

    errcode = sr_stop_listening(&iat);
    if (errcode) {
        printf("\nsr_stop_listening failed! error code:%d \n", errcode);

        sr_stop_listening(&iat);
        sr_uninit(&iat);
        return;
    }
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
    bool ret = false;
    if (text.find("别说了") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        request_message.uri = "/stop_audio";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }
    static int x = 0;
    if (text.find("前进") != std::string::npos || text.find("向前走") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        request_message.uri = "/motor/walk_start";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }
    if (text.find("走快点") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        x += 5;
        request_message.uri = "/motor/walk?x=" + std::to_string(x);
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }
    if (text.find("停下来") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        x = 0;
        request_message.uri = "/motor/walk_stop";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }
    if (text.find("自我介绍") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        request_message.uri = "/motor/action/41?audio=/"+user+"/Data/mp3/Introduction.mp3";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }
    if (text.find("站起来") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        request_message.uri = "/motor/action/1";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }
    if (text.find("坐下") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        request_message.uri = "/motor/action/15";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }    
    if (text.find("跳个舞") != std::string::npos || text.find("街舞") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        request_message.uri = "/motor/action/17";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }
    if (text.find("爬起来") != std::string::npos) {
        RequestMessage request_message;
        request_message.method = "GET";
        request_message.uri = "/motor/fall_up";
        send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
        _state = State_Finished;
        ret = true;
    }
    if (ret) {
        char recv_buf[4096] = "";
        recv(resource_socket_fd, recv_buf, sizeof(recv_buf), 0);
    }
    return ret;
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
    std::string cptts_cmd = "scp " + filename + " " + user +"@" + ip + ":/tmp/" + filename;
    system(cptts_cmd.c_str());
    RequestMessage request_message;
    request_message.method = "GET";
    // char cwd[1024];
    // getcwd(cwd, 1024);
    request_message.uri = "/audio?filepath=/tmp/" + filename;
    // std::string play_cmd = "mplayer " + std::string() + cwd + "/" + filename;
    // system(play_cmd.c_str());
    send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
    
    request_message.uri = "/motor/action/6";
    send(resource_socket_fd, request_message.to_string().data(), request_message.to_string().length(), 0);
    
    char recv_buf[4096] = "";
    recv(resource_socket_fd, recv_buf, sizeof(recv_buf), 0);
    recv(resource_socket_fd, recv_buf, sizeof(recv_buf), 0);
}

std::string AiuiHelper::_request_mic() const
{
    std::cout << "Request Mic" << std::endl;
    RequestMessage request;
    request.method = "GET";
    request.uri = "/mic?time=5";
    send(resource_socket_fd, request.to_string().data(), request.to_string().length(), 0);

    char recv_buf[4096] = "";
    recv(resource_socket_fd, recv_buf, sizeof(recv_buf), 0);
    ResponseMessage response(recv_buf);
    return response.get_data();
}
