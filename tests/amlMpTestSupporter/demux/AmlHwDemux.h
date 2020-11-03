/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_HW_DEMUX_H_
#define _AML_HW_DEMUX_H_

#include "AmlDemuxBase.h"
#include <thread>
#include <map>
#include <set>
#include <utils/Looper.h>

namespace android {
class Looper;
struct ABuffer;
}

namespace aml_mp {
using android::sp;
using android::Looper;
class HwTsParser;

class AmlHwDemux : public AmlDemuxBase
{
public:
    AmlHwDemux();
    ~AmlHwDemux();
    int open(bool isHardwareSource, Aml_MP_DemuxId demuxId) override;
    int close() override;
    int start() override;
    int stop() override;
    int flush() override;

private:
    void threadLoop();
    int addPSISection(int pid) override;
    int removePSISection(int pid) override;
    bool isStopped() const override;

    Aml_MP_DemuxId mDemuxId = AML_MP_DEMUX_ID_DEFAULT;
    std::string mDemuxName;
    std::thread mThread;
    sp<Looper> mLooper;
    sp<HwTsParser> mTsParser;

    std::atomic<bool> mStopped;

private:
    AmlHwDemux(const AmlHwDemux&) = delete;
    AmlHwDemux& operator= (const AmlHwDemux&) = delete;
};

}


#endif
