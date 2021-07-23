/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MEDIAPLAYER_BASE_H_
#define _AML_MEDIAPLAYER_BASE_H_

#include <Aml_MP/Aml_MediaPlayer.h>

namespace aml_mp {
class AmlMediaPlayerBase;

/////////////////////
//not release
//Aml_MediaPlayer.h
typedef struct {

} Aml_MP_MediaPlayerCreateParams;

//common
//#define AML_MP_DRMPLAYER_EVENT_PLAYBACK_STOP_END (0x500222)

//Aml_MediaPlayer.h
typedef Aml_MP_AVSyncSource Aml_MP_MediaPlayerAVSyncSource;

//Aml_MediaPlayer.h
typedef enum {
    //set/get
    AML_MP_MEDIAPLAYER_PARAMETER_SET_BASE  = 0x6000,
    AML_MP_MEDIAPLAYER_PARAMETER_LOOPING,                           //setLooping(int* )

    //get only
    AML_MP_MEDIAPLAYER_PARAMETER_GET_BASE = 0x7000,
    AML_MP_MEDIAPLAYER_PARAMETER_DURTION,                           //getDuration(int* )
    AML_MP_MEDIAPLAYER_PARAMETER_CURRENT_POSITION,                  //getCurrentPosition(int* )
} Aml_MP_MediaPlayerParameterKey;

//Aml_MediaPlayer.h
typedef enum {
    SEEK_PREVIOUS_SYNC,
    SEEK_NEXT_SYNC,
    SEEK_CLOSEST_SYNC,
    SEEK_CLOSEST,
    SEEK_FRAME_INDEX,
} Aml_MP_MediaPlayerSeekMode;
//not release
/////////////////////

struct AmlMediaPlayer_Ops{
    AmlMediaPlayer_Ops();
    ~AmlMediaPlayer_Ops();
    AmlMediaPlayerBase *(*createAmlMediaPlayerFunc)(Aml_MP_MediaPlayerCreateParams* createParams, int instanceId){NULL};

    int initAmlMediaPlayerLib(Aml_MP_MediaPlayerCreateParams* createParams);
    int releaseAmlMediaPlayerLib();

    const char* mName = LOG_TAG;

private:
    bool    isInit{false};
    void*   libHandle{NULL};
};

class AmlMediaPlayerBase {
public:
    static AmlMediaPlayerBase* create(Aml_MP_MediaPlayerCreateParams* createParams, int instanceId);
    explicit AmlMediaPlayerBase(Aml_MP_MediaPlayerCreateParams* createParams, int instanceId);
    virtual ~AmlMediaPlayerBase();

    int registerEventCallBack(Aml_MP_MediaPlayerEventCallback cb, void* userData);

    int setSubtitleWindow(int x, int y, int width, int height);
    int showSubtitle();
    int hideSubtitle();

    virtual int setDataSource(const char *uri) = 0;
    virtual int setANativeWindow(ANativeWindow* nativeWindow) = 0;
    virtual int setVideoWindow(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
    virtual int prepare() = 0;
    virtual int prepareAsync() = 0;
    virtual int start() = 0;
    virtual int seekTo(int msec, Aml_MP_MediaPlayerSeekMode mode) = 0;
    virtual int pause() = 0;
    virtual int resume() = 0;
    virtual int stop() = 0;
    virtual int reset() = 0;
    virtual int release() = 0;
    virtual int getCurrentPosition(int* msec) = 0;
    virtual int getDuration(int* msec) = 0;
    virtual int setVolume(float volume) = 0;
    virtual int getVolume(float* volume) = 0;
    virtual int showVideo() = 0;
    virtual int hideVideo() = 0;
    virtual int invoke(Aml_MP_MediaPlayerInvokeRequest* request, Aml_MP_MediaPlayerInvokeReply* reply) = 0;
    virtual int setAVSyncSource(Aml_MP_AVSyncSource syncSource) = 0;
    virtual int setParameter(Aml_MP_MediaPlayerParameterKey key, void* parameter) = 0;
    virtual int getParameter(Aml_MP_MediaPlayerParameterKey key, void* parameter) = 0;
    virtual bool isPlaying() = 0;
    virtual int setPlaybackRate(float rate) = 0;

protected:
    void notifyListener(Aml_MP_MediaPlayerEventType eventType, int64_t param);

private:
    char mName[50];

    Aml_MP_MediaPlayerEventCallback mEventCb;
    void* mUserData;

    AmlMediaPlayerBase(const AmlMediaPlayerBase&) = delete;
    AmlMediaPlayerBase& operator= (const AmlMediaPlayerBase&) = delete;
};

}

#endif
