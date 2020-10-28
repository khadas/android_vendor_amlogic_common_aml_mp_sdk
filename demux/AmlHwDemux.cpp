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
#include <sstream>
#include <set>
#include <utils/Looper.h>
#include "AmlHwDemux.h"
#include <utils/AmlMpHandle.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <sys/ioctl.h>
extern "C" {
#include <linux/dvb/dmx.h>
}

#define MLOG(fmt, ...) ALOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

namespace aml_mp {
using android::ABuffer;
using android::wp;

////////////////////////////////////////////////////////////////////////////////
struct AmlHwDemux::Filter : public AmlMpHandle
{
    Filter(Aml_MP_Demux_SectionFilterCb cb, void* userData, int id);
    ~Filter();

    void notifyListener(const sp<ABuffer>& data, int version);
    void setOwner(const sp<AmlHwDemux::Channel>& channel);
    bool hasOwner() const;
    int id() const {
        return mId;
    }

    wp<AmlHwDemux::Channel> getOwner() const {
        return mChannel;
    }

private:
    Aml_MP_Demux_SectionFilterCb mCb;
    void* pUserData;
    const int mId;
    int mVersion;
    wp<AmlHwDemux::Channel> mChannel;

private:
    Filter(const Filter&);
    Filter& operator= (const Filter&);
};

struct AmlHwDemux::Channel : public AmlMpHandle
{
    Channel(int pid, int channelFd);
    ~Channel();
    bool attachFilter(const sp<Filter>& filter);
    bool detachFilter(const sp<Filter>& filter);
    bool hasFilter() const;
    void onData(const sp<ABuffer>& data);
    int pid() const {
        return mPid;
    }

    int channelFd() const {
        return mFd;
    }

    bool enabled() const;
    void setEnable(bool enable);

private:
    int mPid = AML_MP_INVALID_PID;

    std::atomic_bool mEnabled {true};

    mutable std::mutex mLock;
    std::set<sp<Filter>> mFilters;

    int mFd = -1;

    Channel(const Channel&);
    Channel& operator= (const Channel&);
};

struct AmlHwDemux::HwSectionCallback : android::LooperCallback
{
    HwSectionCallback() {

    }

    ~HwSectionCallback() {

    }

    int handleEvent(int fd, int events, void* data)
    {
        sp<ABuffer> buffer = new ABuffer(1024);
        if (events & Looper::EVENT_INPUT) {
            int len = ::read(fd, buffer->base(), buffer->size());
            if (len < 0) {
                ALOGE("read failed, %s", strerror(errno));
                return 0;
            }

            buffer->setRange(0, len);
        }

        sp<Channel> channel = static_cast<Channel*>(data);
        channel->onData(buffer);

        return 1;
    }
};

////////////////////////////////////////////////////////////////////////////////
AmlHwDemux::AmlHwDemux()
: mHwSectionCallback(new HwSectionCallback)
{
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

    return 0;
}

int AmlHwDemux::close()
{
    MLOG();

    return 0;
}

int AmlHwDemux::start()
{
    mLooper = new Looper(Looper::PREPARE_ALLOW_NON_CALLBACKS);
    if (mLooper == nullptr) {
        ALOGE("create looper failed!");
        return -1;
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

void* AmlHwDemux::createChannel(int pid)
{
    sp<Channel> channel;

    {
        std::lock_guard<std::mutex> _l(mLock);
        if (mStopped) {
            ALOGE("can't create channel for pid:%d, mStopped:%d", pid, mStopped.load(std::memory_order_relaxed));
            return NULL;
        }

        auto it = mChannels.find(pid);
        if (it != mChannels.end()) {
            ALOGV("return exist channel:%p", channel.get());
            channel = it->second;
            goto exit;
        }

        int channelFd;
        if (openHardwareChannel(pid, &channelFd) < 0) {
            ALOGE("openHardwareChannel failed!");
            return nullptr;
        }

        channel = new Channel(pid, channelFd);
        auto ret = mChannels.emplace(pid, channel);
        if (ret.second) {
            ALOGI("create channel success!");
        }

        startPollHardwareChannel(channel.get());
    }

exit:
    channel->incStrong(this);
    return aml_handle_cast(channel);
}

int AmlHwDemux::destroyChannel(void* _channel)
{
    sp<Channel> channel = aml_handle_cast<Channel>(_channel);
    if (channel == nullptr) {
        return -1;
    }

    int pid = channel->pid();
    MLOG("channel:%d", pid);

    if (channel->hasFilter()) {
        ALOGI("channel:%d has filter, don't do destroy!", channel->pid());
        return -1;
    }

    std::lock_guard<std::mutex> _l(mLock);
    auto it = mChannels.find(pid);
    if (it == mChannels.end()) {
        return 0;
    }

    stopPollHardwareChannel(channel.get());
    closeHardwareChannel(channel->channelFd());

    mChannels.erase(it);

    channel->decStrong(this);
    return 0;
}

int AmlHwDemux::openChannel(void* _channel)
{
    sp<Channel> channel = aml_handle_cast<Channel>(_channel);
    ALOGI("channel:%p, _channel:%p", channel.get(), _channel);
    if (channel == nullptr) {
        return -1;
    }

    ALOGV("enable channel:%d", channel->pid());

    return 0;
}

int AmlHwDemux::closeChannel(void* _channel)
{
    sp<Channel> channel = aml_handle_cast<Channel>(_channel);
    if (channel == nullptr) {
        return -1;
    }

    ALOGV("disable channel:%d", channel->pid());

    return 0;
}

void* AmlHwDemux::createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData)
{
    sp<Filter> filter = new Filter(cb, userData, mFilterId++);

    filter->incStrong(this);

    return aml_handle_cast(filter);
}

int AmlHwDemux::destroyFilter(void* _filter)
{
    sp<Filter> filter = aml_handle_cast<Filter>(_filter);
    if (filter == nullptr) {
        return -1;
    }

    if (filter->hasOwner()) {
        ALOGW("filter has not been detached before!!!");
        sp<Channel> channel = filter->getOwner().promote();
        if (channel != nullptr) {
            channel->detachFilter(filter);
        }
    }

    filter->decStrong(this);
    return 0;
}

int AmlHwDemux::attachFilter(void* _filter, void* _channel)
{
    sp<Filter> filter = aml_handle_cast<Filter>(_filter);
    sp<Channel> channel = aml_handle_cast<Channel>(_channel);

    if (filter == nullptr || channel == nullptr) {
        return -1;
    }

    channel->attachFilter(filter);

    return 0;
}

int AmlHwDemux::detachFilter(void* _filter, void* _channel)
{
    sp<Filter> filter = aml_handle_cast<Filter>(_filter);
    sp<Channel> channel = aml_handle_cast<Channel>(_channel);

    if (filter == nullptr || channel == nullptr) {
        return -1;
    }

    if (!channel->hasFilter()) {
        ALOGW("channel has no filter!!!");
        return -1;
    }

    channel->detachFilter(filter);

    return 0;
}

int AmlHwDemux::openHardwareChannel(int pid, int* pFd)
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
        return -1;
    }

    int buffersize = 32 * 1024;
    ret = ioctl(fd, DMX_SET_BUFFER_SIZE, buffersize);
    if (ret < 0) {
        MLOG("set buffer size failed!");
        return -1;
    }

    ret = ioctl(fd, DMX_START);
    if (ret < 0) {
        MLOG("dmx start failed!");
        return -1;
    }

    *pFd = fd;
    return 0;
}

int AmlHwDemux::closeHardwareChannel(int fd)
{
    if (fd < 0) {
        MLOG("mFd is invalid");
        return -1;
    }

    int ret = ioctl(fd, DMX_STOP);
    if (ret < 0) {
        MLOG("dmx stop failed!");
        return -1;
    }

    ::close(fd);

    return 0;
}

void AmlHwDemux::startPollHardwareChannel(Channel* channel)
{
    int ret = mLooper->addFd(
            channel->channelFd(),
            Looper::POLL_CALLBACK,
            Looper::EVENT_ERROR|Looper::EVENT_INPUT,
            mHwSectionCallback,
            channel);

    if (ret <= 0) {
        ALOGE("addFd failed! fd:%d", channel->channelFd());
    }
}

void AmlHwDemux::stopPollHardwareChannel(Channel* channel)
{
    int ret = mLooper->removeFd(channel->channelFd());
    if (ret <= 0) {
        ALOGE("removeFd failed! fd:%d", channel->channelFd());
    }
}

///////////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////////
AmlHwDemux::Filter::Filter(Aml_MP_Demux_SectionFilterCb cb, void* userData, int id)
: mCb(cb)
, pUserData(userData)
, mId(id)
, mVersion(-1)
{
    MLOG("ctor filter:%d", mId);
}

AmlHwDemux::Filter::~Filter()
{
    MLOG("dtor filter:%d", mId);
}

void AmlHwDemux::Filter::notifyListener(const sp<ABuffer>& data, int version)
{
    if (mCb == nullptr) {
        ALOGW("user cb is NULL!");
        return;
    }

    if (mVersion >= 0 && version >= 0 && mVersion == version) {
        return;
    }

    mVersion = version;
    if (mVersion >= 0) {
        ALOGI("notify filter:%d, version:%d", mId, mVersion);
    }

    mCb(data->size(), data->base(), pUserData);
}

void AmlHwDemux::Filter::setOwner(const sp<AmlHwDemux::Channel>& channel)
{
    mChannel = channel;
}

bool AmlHwDemux::Filter::hasOwner() const
{
    return mChannel != nullptr;
}

////////////////////////////////////////////////////////////////////////////////
AmlHwDemux::Channel::Channel(int pid, int channelFd)
: mPid(pid)
, mFd(channelFd)
{
    MLOG("ctor channel, pid:%d, channelFd:%d", mPid, channelFd);
}

AmlHwDemux::Channel::~Channel()
{
    MLOG("dtor channel, pid:%d", mPid);
}

bool AmlHwDemux::Channel::attachFilter(const sp<Filter>& filter)
{
    {
        std::lock_guard<std::mutex> _l(mLock);
        mFilters.insert(filter);
        ALOGV("attach filter:%d to channel:%d, totoal filters num:%d",
            filter->id(), mPid, mFilters.size());
    }

    filter->setOwner(this);

    return true;
}

bool AmlHwDemux::Channel::detachFilter(const sp<Filter>& filter)
{
    std::lock_guard<std::mutex> _l(mLock);

    if (mFilters.find(filter) != mFilters.end()) {
        filter->setOwner(nullptr);
        mFilters.erase(filter);
        ALOGV("detach filter:%d from channel:%d, total filters num:%d",
            filter->id(), mPid, mFilters.size());
    }

    return true;
}

bool AmlHwDemux::Channel::hasFilter() const
{
    std::lock_guard<std::mutex> _l(mLock);
    return !mFilters.empty();
}

void AmlHwDemux::Channel::onData(const sp<ABuffer>& data)
{
    std::set<sp<Filter>> filters;

    {
        std::lock_guard<std::mutex> _l(mLock);
        if (!enabled() || mFilters.empty()) {
            return;
        }

        filters = mFilters;
    }

    int version = data->data()[5]>>1 & 0x1F;

    for (auto& f : filters) {
        f->notifyListener(data, version);
    }
}

bool AmlHwDemux::Channel::enabled() const
{
    return mEnabled.load(std::memory_order_relaxed);
}

void AmlHwDemux::Channel::setEnable(bool enable)
{
    mEnabled.store(enable, std::memory_order_relaxed);
}


}
