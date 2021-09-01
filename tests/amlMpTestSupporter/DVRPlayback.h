/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "demux/AmlTsParser.h"
#include "source/Source.h"
#include "TestModule.h"
#ifdef ANDROID
#include <system/window.h>
#endif
#include <Aml_MP/Aml_MP.h>
#include <utils/RefBase.h>

namespace aml_mp {

// for dvr (encrypt) playback
class DVRPlayback : public TestModule, public ISourceReceiver
{
public:
    DVRPlayback(const std::string& url, bool cryptoMode, Aml_MP_DemuxId demuxId);
    ~DVRPlayback();
    #ifdef ANDROID
    void setANativeWindow(const android::sp<ANativeWindow>& window);
    #endif
    void registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    int start();
    int stop();
    void signalQuit();

protected:
    virtual const Command* getCommandTable() const override;
    virtual void* getCommandHandle() const override;

private:
    std::string stripUrlIfNeeded(const std::string& url) const;
    int initDVRDecryptPlayback(Aml_MP_DVRPlayerDecryptParams& decryptParams);
    int uninitDVRDecryptPlayback();

private:
    const std::string mUrl;
    const bool mCryptoMode;
    Aml_MP_DemuxId mDemuxId;
    AML_MP_DVRPLAYER mPlayer = AML_MP_INVALID_HANDLE;
    Aml_MP_PlayerEventCallback mEventCallback = nullptr;
    void* mUserData = nullptr;

    AML_MP_CASSESSION mCasSession = nullptr;
    AML_MP_SECMEM mSecMem = nullptr;
    bool mDvrReplayInited = false;

private:
    DVRPlayback(const DVRPlayback&) = delete;
    DVRPlayback& operator=(const DVRPlayback&) = delete;
};
}
