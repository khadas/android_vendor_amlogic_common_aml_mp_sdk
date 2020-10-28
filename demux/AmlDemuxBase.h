/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_DEMUX_BASE_H_
#define _AML_MP_DEMUX_BASE_H_

#include <Aml_MP/Common.h>
#include <utils/RefBase.h>

namespace aml_mp {
using android::RefBase;
using android::sp;

///////////////////////////////////////////////////////////////////////////////
typedef int (*Aml_MP_Demux_SectionFilterCb)(size_t size, const uint8_t* data, void* userData);

class AmlDemuxBase : public RefBase
{
public:
    static sp<AmlDemuxBase> create(bool isHardwareDemux);
    virtual ~AmlDemuxBase();

    virtual int open(bool isHardwareSource, Aml_MP_DemuxId demuxId = AML_MP_DEMUX_ID_DEFAULT) = 0;
    virtual int close() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int flush() = 0;
    virtual void* createChannel(int pid) = 0;
    virtual int destroyChannel(void* channel) = 0;
    virtual int openChannel(void* channel) = 0;
    virtual int closeChannel(void* channel) = 0;
    virtual void* createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData) = 0;
    virtual int destroyFilter(void* filter) = 0;
    virtual int attachFilter(void* filter, void* channel) = 0;
    virtual int detachFilter(void* filter, void* channel) = 0;
    virtual int feedTs(const uint8_t* buffer, size_t size) {
        (void)buffer;
        return size;
    }

protected:
    AmlDemuxBase();

private:
    AmlDemuxBase(const AmlDemuxBase&) = delete;
    AmlDemuxBase& operator= (const AmlDemuxBase&) = delete;
};

}





#endif
