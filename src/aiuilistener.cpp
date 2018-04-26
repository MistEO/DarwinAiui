#include "aiuilistener.h"

#include "jsoncpp/json/json.h"
#include <iostream>

#include "aiuihelper.h"

using namespace std;
using namespace aiui;

AIUIEXPORT void AiuiListener::onEvent(const aiui::IAIUIEvent& event) const
{
    switch (event.getEventType()) {
    case AIUIConstant::EVENT_STATE: {

        this->_aiui_state = event.getArg1();
        switch (_aiui_state) {
        case AIUIConstant::STATE_IDLE: {
            cout << "EVENT_STATE:"
                 << "IDLE" << endl;
        } break;

        case AIUIConstant::STATE_READY: {
            cout << "EVENT_STATE:"
                 << "READY" << endl;
            sleep(1); //Ready之后立即WakeUp可能会出现问题，原因未知
            AiuiHelper::ins()._wake_up();
        } break;

        case AIUIConstant::STATE_WORKING: {
            cout << "EVENT_STATE:"
                 << "WORKING" << endl;
        } break;
        }
    } break;

    case AIUIConstant::EVENT_WAKEUP: {
        cout << "EVENT_WAKEUP:" << event.getInfo() << endl;
    } break;

    case AIUIConstant::EVENT_SLEEP: {
        cout << "EVENT_SLEEP:arg1=" << event.getArg1() << endl;
    } break;

    case AIUIConstant::EVENT_VAD: {
        switch (event.getArg1()) {
        case AIUIConstant::VAD_BOS: {
            cout << "EVENT_VAD:"
                 << "BOS" << endl;
        } break;

        case AIUIConstant::VAD_EOS: {
            cout << "EVENT_VAD:"
                 << "EOS" << endl;
        } break;

        case AIUIConstant::VAD_VOL: {
            //cout << "EVENT_VAD:" << "VOL" << endl;
        } break;
        }
    } break;

    case AIUIConstant::EVENT_RESULT: {
        using namespace VA;
        Json::Value bizParamJson;
        Json::Reader reader;

        if (!reader.parse(event.getInfo(), bizParamJson, false)) {
            cout << "parse error!" << endl
                 << event.getInfo() << endl;
            break;
        }
        Json::Value data = (bizParamJson["data"])[0];
        Json::Value params = data["params"];
        Json::Value content = (data["content"])[0];
        string sub = params["sub"].asString();
        cout << "EVENT_RESULT:" << sub << endl;

        if (sub == "nlp") {
            Json::Value empty;
            Json::Value contentId = content.get("cnt_id", empty);

            if (contentId.empty()) {
                cout << "Content Id is empty" << endl;
                break;
            }

            string cnt_id = contentId.asString();
            Buffer* buffer = event.getData()->getBinary(cnt_id.c_str());
            string resultStr;

            if (NULL != buffer) {
                resultStr = string((char*)buffer->data());

                // cout << resultStr << endl;
                Json::Value skill_data;
                if (!reader.parse(resultStr, skill_data, false)) {
                    cout << "parse error!" << endl
                         << resultStr << endl;
                    break;
                }
                AiuiHelper::ins()._result_parse(skill_data);

                // std::cout << "Answer text:" << skill_data["intent"]["answer"]["text"] << std::endl;
            }
        }
    } break;

    case AIUIConstant::EVENT_ERROR: {
        cout << "EVENT_ERROR:" << event.getArg1() << endl;
    } break;
    }
}

int AiuiListener::get_state() const
{
    return _aiui_state;
}