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
    virtual AML_MP_HANDLE createChannel(int pid) override;
    virtual int destroyChannel(AML_MP_HANDLE channel) override;
    virtual int openChannel(AML_MP_HANDLE channel) override;
    virtual int closeChannel(AML_MP_HANDLE channel) override;
    virtual AML_MP_HANDLE createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData) override;
    virtual int destroyFilter(AML_MP_HANDLE filter) override;
    virtual int attachFilter(AML_MP_HANDLE filter, AML_MP_HANDLE channel) override;
    virtual int detachFilter(AML_MP_HANDLE filter, AML_MP_HANDLE channel) override;
    virtual int feedTs(const uint8_t* buffer, size_t size) override;

private:
    AML_MP_HANDLE mDemuxHandle = nullptr;
    std::atomic<uint32_t> mFilterId;

    AmlSwDemux(const AmlSwDemux&) = delete;
    AmlSwDemux& operator= (const AmlSwDemux&) = delete;
};
}


#endif
