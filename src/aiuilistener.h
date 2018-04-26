#pragma once

#include <aiui/AIUI.h>

class AiuiListener : public aiui::AIUIListener {
public:
    AIUIEXPORT void onEvent(const aiui::IAIUIEvent& event) const;
    int get_state() const;

private:
    mutable int _aiui_state = aiui::AIUIConstant::STATE_IDLE;
};