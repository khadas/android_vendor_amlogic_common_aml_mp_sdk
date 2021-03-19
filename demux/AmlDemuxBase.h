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
#include <utils/AmlMpRefBase.h>
#include <mutex>
#include <map>

namespace aml_mp {
struct AmlMpBuffer;

///////////////////////////////////////////////////////////////////////////////
typedef int (*Aml_MP_Demux_SectionFilterCb)(int pid, size_t size, const uint8_t* data, void* userData);

class AmlDemuxBase : public AmlMpRefBase
{
public:
    typedef void* CHANNEL;
    typedef void* FILTER;

    static sptr<AmlDemuxBase> create(bool isHardwareDemux);
    virtual ~AmlDemuxBase();

    virtual int open(bool isHardwareSource, Aml_MP_DemuxId demuxId = AML_MP_DEMUX_ID_DEFAULT) = 0;
    virtual int close() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int flush() = 0;
    virtual int feedTs(const uint8_t* buffer, size_t size) {
        (void)buffer;
        return size;
    }

    CHANNEL createChannel(int pid, bool checkCRC = true);
    int destroyChannel(CHANNEL channel);
    int openChannel(CHANNEL channel);
    int closeChannel(CHANNEL channel);
    FILTER createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData);
    int destroyFilter(FILTER filter);
    int attachFilter(FILTER filter, CHANNEL channel);
    int detachFilter(FILTER filter, CHANNEL channel);

    struct ITsParser : virtual public AmlMpRefBase {
        using SectionCallback = void(int pid, const sptr<AmlMpBuffer>& data, int version);

        explicit ITsParser(const std::function<SectionCallback>& cb);
        virtual ~ITsParser() =default;
        virtual int feedTs(const uint8_t* buffer, size_t size) = 0;
        virtual void reset() = 0;
        virtual int addPSISection(int pid, bool checkCRC = true) = 0;
        virtual int getPSISectionData(int pid) = 0;
        virtual void removePSISection(int pid) = 0;

    protected:
        std::function<SectionCallback> mSectionCallback;

    private:
        ITsParser(const ITsParser&) = delete;
        ITsParser& operator=(const ITsParser&) = delete;
    };

protected:
    struct Channel;
    struct Filter;

    AmlDemuxBase();
    virtual int addPSISection(int pid, bool checkCRC = true) = 0;
    virtual int removePSISection(int pid) = 0;
    virtual bool isStopped() const = 0;

    void notifyData(int pid, const sptr<AmlMpBuffer>& data, int version);

    std::atomic<uint32_t> mFilterId{0};
    std::mutex mLock;
    std::map<int, Channel*> mChannels;

private:
    AmlDemuxBase(const AmlDemuxBase&) = delete;
    AmlDemuxBase& operator= (const AmlDemuxBase&) = delete;
};

}





#endif
