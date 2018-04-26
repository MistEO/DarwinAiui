/*

@file

@brief 閿熸枻鎷烽敓鏂ゆ嫹褰曢敓鏂ゆ嫹娑岃閿熺獤璁规嫹閿熺祫SC閿熸帴鍙ｅ嚖鎷疯涓€閿熸枻鎷稭IC褰曢敓鏂ゆ嫹璇嗛敓鏂ゆ嫹閿熶茎锝忔嫹閿燂拷



@author		taozhang9

@date		2016/05/27

*/

enum sr_audsrc

{

    SR_MIC, /* write data from mic */

    SR_USER /* write data from user by calling API */

};

//#define DEFAULT_INPUT_DEVID     (-1)

#define E_SR_NOACTIVEDEVICE 1

#define E_SR_NOMEM 2

#define E_SR_INVAL 3

#define E_SR_RECORDFAIL 4

#define E_SR_ALREADY 5

class AiuiHelper;
#define END_REASON_VAD_DETECT 0 /* detected speech done  */

struct speech_rec {

    enum sr_audsrc aud_src; /* from mic or manual  stream write */

    AiuiHelper* notif = nullptr;

    const char* session_id;

    int ep_stat;

    int rec_stat;

    int audio_status;

    struct recorder* recorder;

    volatile int state;

    char* session_begin_params;
};

#ifdef __cplusplus

extern "C" {

#endif

/* must init before start . is aud_src is SR_MIC, the default capture device

 * will be used. see sr_init_ex */

int sr_init(struct speech_rec* sr, const char* session_begin_params, enum sr_audsrc aud_src, AiuiHelper* notifier);

int sr_start_listening(struct speech_rec* sr);

int sr_stop_listening(struct speech_rec* sr);

/* only used for the manual write way. */

int sr_write_audio_data(struct speech_rec* sr, char* data, unsigned int len);

/* must call uninit after you don't use it */

void sr_uninit(struct speech_rec* sr);

#ifdef __cplusplus

} /* extern "C" */

#endif /* C++ */
