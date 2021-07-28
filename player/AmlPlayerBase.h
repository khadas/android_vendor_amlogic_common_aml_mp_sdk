/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_PLAYER_BASE_H_
#define _AML_PLAYER_BASE_H_

#include <utils/AmlMpRefBase.h>
#include <Aml_MP/Aml_MP.h>
#ifdef HAVE_SUBTITLE
#include <SubtitleNativeAPI.h>
#endif

struct ANativeWindow;

namespace aml_mp {
//using android::sp;
//using android::RefBase;
//using android::NativeHandle;

class AmlPlayerBase : public AmlMpRefBase {
public:
    static sptr<AmlPlayerBase> create(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    virtual ~AmlPlayerBase();

    int registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);

    int setSubtitleParams(const Aml_MP_SubtitleParams* params);
    int switchSubtitleTrack(const Aml_MP_SubtitleParams* params);
    int showSubtitle();
    int hideSubtitle();
    int startSubtitleDecoding();
    int stopSubtitleDecoding();
    int setSubtitleWindow(int x, int y, int width, int height);

    virtual int initCheck() const = 0;
    virtual int setVideoParams(const Aml_MP_VideoParams* params) = 0;
    virtual int setAudioParams(const Aml_MP_AudioParams* params) = 0;
    virtual int setANativeWindow(ANativeWindow* nativeWindow) = 0;
    virtual int start();
    virtual int stop();
    virtual int pause() = 0;
    virtual int resume() = 0;
    virtual int flush() = 0;
    virtual int setPlaybackRate(float rate) = 0;
    virtual int switchAudioTrack(const Aml_MP_AudioParams* params) = 0;
    virtual int writeData(const uint8_t* buffer, size_t size) = 0;
    virtual int writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts) = 0;
    virtual int getCurrentPts(Aml_MP_StreamType type, int64_t* pts) = 0;
    virtual int getBufferStat(Aml_MP_BufferStat* bufferStat) = 0;
    virtual int setVideoWindow(int x, int y, int width, int height) = 0;
    virtual int setVolume(float volume) = 0;
    virtual int getVolume(float* volume) = 0;
    virtual int showVideo() = 0;
    virtual int hideVideo() = 0;
    virtual int setParameter(Aml_MP_PlayerParameterKey key, void* parameter) = 0;
    virtual int getParameter(Aml_MP_PlayerParameterKey key, void* parameter) = 0;

    virtual int setAVSyncSource(Aml_MP_AVSyncSource syncSource) = 0;
    virtual int setPcrPid(int pid) = 0;

    virtual int startVideoDecoding() = 0;
    virtual int stopVideoDecoding() = 0;

    virtual int startAudioDecoding() = 0;
    virtual int stopAudioDecoding() = 0;
    virtual int startADDecoding() {return 0;}
    virtual int stopADDecoding() {return 0;}

    virtual int setADParams(const Aml_MP_AudioParams* params, bool enableMix) = 0;

#ifdef HAVE_SUBTITLE
    //Subtitle Call back function
    static void AmlMPSubtitleDataCb(const char * data, int size, AmlSubDataType type,
                                                int x, int y, int width, int height, int videoWidth,
                                                int videoHeight, int showing);
    static void AmlMPSubtitleAvailCb(int avail);
    static void AmlMPSubtitleDimensionCb(int width, int height);
    static void AmlMPSubtitleAfdEventCb(int event);
    static void AmlMPSubtitleChannelUpdateCb(int event, int id);
    static void AmlMPSubtitleLanguageCb(std::string lang);
    static void AmlMPSubtitleInfoCb(int what, int extra);

    static wptr<AmlPlayerBase> sSubtitleCbHandle;
#endif

protected:
    int mSubWindowX;
    int mSubWindowY;
    int mSubWindowWidth;
    int mSubWindowHeight;

    explicit AmlPlayerBase(int instanceId);
    void notifyListener(Aml_MP_PlayerEventType eventType, int64_t param = 0);

private:
    char mName[50];
    const int mInstanceId;

    Aml_MP_PlayerEventCallback mEventCb;
    void* mUserData;


#ifdef HAVE_SUBTITLE
    static bool constructAmlSubtitleParam(AmlSubtitleParam* amlSubParam, Aml_MP_SubtitleParams* params);
    AmlSubtitleHnd mSubtitleHandle = nullptr;

    //Subtitle info
    Aml_MP_SubtitleData mSubtitleData;
    Aml_MP_SubtitleDimension mSubtitleDimension;
    char mSubtitleIso639Code[4];
#endif
    Aml_MP_SubtitleParams mSubtitleParams;

    AmlPlayerBase(const AmlPlayerBase&) = delete;
    AmlPlayerBase& operator= (const AmlPlayerBase&) = delete;
};

}

#endif
