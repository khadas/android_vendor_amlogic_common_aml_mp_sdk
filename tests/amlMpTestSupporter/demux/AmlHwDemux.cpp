/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlHwDemux"
#include <utils/Log.h>
#include <Aml_MP/Aml_MP.h>
#include "AmlHwDemux.h"
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpBuffer.h>
#include <utils/AmlMpEventLooper.h>
#include <sys/ioctl.h>
#include <sstream>
extern "C" {
#include <linux/dvb/dmx.h>
}

#define MLOG(fmt, ...) ALOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

namespace aml_mp {
class HwTsParser : public AmlDemuxBase::ITsParser, public LooperCallback
{
public:
    HwTsParser(const std::function<SectionCallback>& cb, const std::string& name);
    ~HwTsParser();
    int feedTs(const uint8_t* buffer, size_t size);
    void reset();
    int addPSISection(int pid);
    int getPSISectionData(int pid);
    void removePSISection(int pid);

private:
    virtual int handleEvent(int fd, int events, void* data);

    std::string mDemuxName;

    std::mutex mLock;
    std::map<int, int> mChannelFds; //pid, fd

private:
    HwTsParser(const HwTsParser&) = delete;
    HwTsParser& operator=(const HwTsParser&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
AmlHwDemux::AmlHwDemux()
{
    MLOG();
}

AmlHwDemux::~AmlHwDemux()
{
    close();
}

int AmlHwDemux::open(bool isHardwareSource, Aml_MP_DemuxId demuxId)
{
    mDemuxId = demuxId;

    Aml_MP_DemuxSource demuxSource = AML_MP_DEMUX_SOURCE_DMA0;
    if (isHardwareSource) {
        switch (mDemuxId) {
        case AML_MP_HW_DEMUX_ID_0:
            demuxSource = AML_MP_DEMUX_SOURCE_TS0;
            break;

        case AML_MP_HW_DEMUX_ID_1:
            demuxSource = AML_MP_DEMUX_SOURCE_TS1;
            break;

        case AML_MP_HW_DEMUX_ID_2:
            demuxSource = AML_MP_DEMUX_SOURCE_TS2;
            break;

        default:
            break;
        }
    }

    ALOGI("demuxSource:%d", demuxSource);
    Aml_MP_SetDemuxSource(mDemuxId, demuxSource);

    std::stringstream s;
    s << "/dev/dvb0.demux" << mDemuxId;
    mDemuxName = s.str();
    ALOGI("mDemuxName:%s", mDemuxName.c_str());

    if (mTsParser == nullptr) {
        mTsParser = new HwTsParser([this](int pid, const sptr<AmlMpBuffer>& data, int version) {
            return notifyData(pid, data, version);
        }, mDemuxName);
    }

    return 0;
}

int AmlHwDemux::close()
{
    MLOG();

    mTsParser.clear();

    return 0;
}

int AmlHwDemux::start()
{
    if (mLooper == nullptr) {
        mLooper = new Looper(Looper::PREPARE_ALLOW_NON_CALLBACKS);
        if (mLooper == nullptr) {
            ALOGE("create looper failed!");
            return -1;
        }
    }

    mThread = std::thread([this] {
        threadLoop();
    });

    return 0;
}

int AmlHwDemux::stop()
{
    {
        std::lock_guard<std::mutex> _l(mLock);
        mStopped = true;
    }

    if (mLooper) {
        mLooper->wake();
    }

    ALOGI("join...");
    if (mThread.joinable()) {
        mThread.join();
    }

    ALOGI("stopped!");
    return 0;
}

int AmlHwDemux::flush()
{
    MLOG();

    return 0;
}

int AmlHwDemux::addPSISection(int pid)
{
    int channelFd = mTsParser->addPSISection(pid);

    int ret = mLooper->addFd(
            channelFd,
            Looper::POLL_CALLBACK,
            Looper::EVENT_ERROR|Looper::EVENT_INPUT,
            mTsParser,
            (void*)pid);

    if (ret <= 0) {
        ALOGE("addFd failed! fd:%d", channelFd);
    }

    return ret;
}

int AmlHwDemux::removePSISection(int pid)
{
    int channelFd = mTsParser->getPSISectionData(pid);
    int ret = mLooper->removeFd(channelFd);
    if (ret <= 0) {
        ALOGE("removeFd failed! fd:%d", channelFd);
    }

    mTsParser->removePSISection(pid);

    return ret;
}

bool AmlHwDemux::isStopped() const
{
    return mStopped.load(std::memory_order_relaxed);
}

void AmlHwDemux::threadLoop()
{
    int ret = 0;

    if (mLooper == nullptr) {
        ALOGE("looper is NULL!");
        return;
    }
    Looper::setForThread(mLooper);

    for (;;) {
        ret = mLooper->pollOnce(10 * 1000);
        if (ret == Looper::POLL_ERROR) {
            ALOGE("error!");
            break;
        }

        bool stopped = mStopped.load(std::memory_order_relaxed);
        if (stopped) {
            ALOGE("request stop...");
            break;
        }
    }

    ALOGI("AmlHwDemux threadLoop exited!");
}

///////////////////////////////////////////////////////////////////////////////
HwTsParser::HwTsParser(const std::function<SectionCallback>& cb, const std::string& name)
: ITsParser(cb)
, mDemuxName(name)
{
    MLOG();
}

HwTsParser::~HwTsParser()
{
    MLOG();
}

int HwTsParser::feedTs(const uint8_t* buffer, size_t size)
{
    (void)buffer;
    (void)size;

    return 0;
}

int HwTsParser::addPSISection(int pid)
{
    int fd = -1;
    fd = ::open(mDemuxName.c_str(), O_RDWR);
    if (fd < 0) {
        MLOG("open %s failed! %s", mDemuxName.c_str(), strerror(errno));
        return -1;
    }

    struct dmx_sct_filter_params filter_param;
    memset(&filter_param, 0, sizeof(filter_param));
    filter_param.pid = pid;
    filter_param.flags = DMX_CHECK_CRC;

    int ret = ioctl(fd, DMX_SET_FILTER, &filter_param);
    if (ret < 0) {
        MLOG("set filter failed!");
        ::close(fd);
        return -1;
    }

    int buffersize = 32 * 1024;
    ret = ioctl(fd, DMX_SET_BUFFER_SIZE, buffersize);
    if (ret < 0) {
        MLOG("set buffer size failed!");
        ::close(fd);
        return -1;
    }

    ret = ioctl(fd, DMX_START);
    if (ret < 0) {
        MLOG("dmx start failed!");
        ::close(fd);
        return -1;
    }

    {
        std::unique_lock<std::mutex> _l(mLock);
        mChannelFds.emplace(pid, fd);
    }

    return fd;
}

int HwTsParser::getPSISectionData(int pid)
{
    std::unique_lock<std::mutex> _l(mLock);
    auto it = mChannelFds.find(pid);
    return it != mChannelFds.end() ? it->second : -1;
}

void HwTsParser::removePSISection(int pid)
{
    int fd = -1;
    {
        std::unique_lock<std::mutex> _l(mLock);
        auto it = mChannelFds.find(pid);
        if (it != mChannelFds.end()) {
            fd = it->second;
        }
    }

    if (fd < 0) {
        MLOG("mFd is invalid");
        return;
    }

    int ret = ioctl(fd, DMX_STOP);
    if (ret < 0) {
        MLOG("dmx stop failed!");
        return;
    }

    ::close(fd);
}

void HwTsParser::reset()
{

}

int HwTsParser::handleEvent(int fd, int events, void* data)
{
    sptr<AmlMpBuffer> buffer = new AmlMpBuffer(1024);
    if (events & Looper::EVENT_INPUT) {
        int len = ::read(fd, buffer->base(), buffer->size());
        if (len < 0) {
            ALOGE("read failed, %s", strerror(errno));
            return 0;
        }

        buffer->setRange(0, len);
    }

    int version = buffer->data()[5]>>1 & 0x1F;
    int pid = (int)data;

    if (mSectionCallback) {
        mSectionCallback(pid, buffer, version);
    }

    return 1;
}

}
