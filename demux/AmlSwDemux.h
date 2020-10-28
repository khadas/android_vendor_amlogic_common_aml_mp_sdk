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

namespace aml_mp {
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
    virtual void* createChannel(int pid) override;
    virtual int destroyChannel(void* channel) override;
    virtual int openChannel(void* channel) override;
    virtual int closeChannel(void* channel) override;
    virtual void* createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData) override;
    virtual int destroyFilter(void* filter) override;
    virtual int attachFilter(void* filter, void* channel) override;
    virtual int detachFilter(void* filter, void* channel) override;
    virtual int feedTs(const uint8_t* buffer, size_t size) override;

private:
    void* mDemuxHandle = nullptr;
    std::atomic<uint32_t> mFilterId;

    AmlSwDemux(const AmlSwDemux&) = delete;
    AmlSwDemux& operator= (const AmlSwDemux&) = delete;
};
}


#endif
