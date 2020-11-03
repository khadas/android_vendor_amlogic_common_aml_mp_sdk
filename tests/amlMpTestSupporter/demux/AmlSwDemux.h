/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_SW_DEMUX_H_
#define _AML_MP_SW_DEMUX_H_

#include "AmlDemuxBase.h"

namespace android {
struct ALooper;
struct AMessage;
struct ABuffer;
template <class T> struct AHandlerReflector;
}

namespace aml_mp {
class SwTsParser;

class AmlSwDemux : public AmlDemuxBase
{
public:
    AmlSwDemux();
    ~AmlSwDemux();
    virtual int open(bool isHardwareSource, Aml_MP_DemuxId demuxId) override;
    virtual int close() override;
    virtual int start() override;
    virtual int stop() override;
    virtual int flush() override;
    virtual int feedTs(const uint8_t* buffer, size_t size) override;

private:
    friend struct android::AHandlerReflector<AmlSwDemux>;
    enum {
        kWhatFeedData = 'fdta',
        kWhatFlush    = 'flsh',
        kWhatAddPid   = 'apid',
        kWhatRemovePid = 'rpid',
        kWhatDumpInfo = 'dmpI',
    };

    virtual int addPSISection(int pid) override;
    virtual int removePSISection(int pid) override;
    virtual bool isStopped() const override;

    void onMessageReceived(const sp<android::AMessage>& msg);

    void onFeedData(const sp<android::ABuffer>& data);
    int resync(const sp<android::ABuffer>& buffer);
    void onFlush();
    void onAddFilterPid(int pid);
    void onRemoveFilterPid(int pid);

    sp<android::ALooper> mLooper;
    sp<android::AHandlerReflector<AmlSwDemux>> mHandler;

    std::atomic<int32_t> mBufferGeneration{0};
    std::atomic<bool> mStopped{false};
    std::atomic<int64_t> mOutBufferCount {0};

    std::mutex mLock;
    sp<SwTsParser> mTsParser;
    sp<android::ABuffer> mRemainingBytesBuffer;

private:
    AmlSwDemux(const AmlSwDemux&) = delete;
    AmlSwDemux& operator= (const AmlSwDemux&) = delete;
};
}


#endif
