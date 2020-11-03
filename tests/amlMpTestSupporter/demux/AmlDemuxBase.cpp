/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlDemuxBase"
#include <utils/Log.h>
#include "AmlDemuxBase.h"
#include "AmlHwDemux.h"
#include "AmlSwDemux.h"
#include <utils/AmlMpHandle.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <sstream>
#include <set>

#define MLOG(fmt, ...) ALOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

namespace aml_mp {
using android::ABuffer;
using android::sp;
using android::wp;

////////////////////////////////////////////////////////////////////////////////
struct AmlDemuxBase::Filter : public AmlMpHandle
{
    Filter(Aml_MP_Demux_SectionFilterCb cb, void* userData, int id);
    ~Filter();

    void notifyListener(const sp<ABuffer>& data, int version);
    void setOwner(const sp<AmlDemuxBase::Channel>& channel);
    bool hasOwner() const;
    int id() const {
        return mId;
    }

    wp<AmlDemuxBase::Channel> getOwner() const {
        return mChannel;
    }

private:
    Aml_MP_Demux_SectionFilterCb mCb;
    void* pUserData;
    const int mId;
    int mVersion;
    wp<AmlDemuxBase::Channel> mChannel;

private:
    Filter(const Filter&);
    Filter& operator= (const Filter&);
};

struct AmlDemuxBase::Channel : public AmlMpHandle
{
    Channel(int pid);
    ~Channel();
    bool attachFilter(const sp<Filter>& filter);
    bool detachFilter(const sp<Filter>& filter);
    bool hasFilter() const;
    void onData(const sp<ABuffer>& data, int version);
    int pid() const {
        return mPid;
    }

    bool enabled() const;
    void setEnable(bool enable);

private:
    const int mPid = AML_MP_INVALID_PID;

    std::atomic_bool mEnabled {true};

    mutable std::mutex mLock;
    std::set<sp<Filter>> mFilters;

private:
    Channel(const Channel&);
    Channel& operator= (const Channel&);
};

//////////////////////////////////////////////////////////////////////////////
AmlDemuxBase::Filter::Filter(Aml_MP_Demux_SectionFilterCb cb, void* userData, int id)
: mCb(cb)
, pUserData(userData)
, mId(id)
, mVersion(-1)
{
    MLOG("ctor filter:%d", mId);
}

AmlDemuxBase::Filter::~Filter()
{
    MLOG("dtor filter:%d", mId);
}

void AmlDemuxBase::Filter::notifyListener(const sp<ABuffer>& data, int version)
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

void AmlDemuxBase::Filter::setOwner(const sp<AmlDemuxBase::Channel>& channel)
{
    mChannel = channel;
}

bool AmlDemuxBase::Filter::hasOwner() const
{
    return mChannel != nullptr;
}

////////////////////////////////////////////////////////////////////////////////
AmlDemuxBase::Channel::Channel(int pid)
: mPid(pid)
{
    MLOG("ctor channel, pid:%d", mPid);
}

AmlDemuxBase::Channel::~Channel()
{
    MLOG("dtor channel, pid:%d", mPid);
}

bool AmlDemuxBase::Channel::attachFilter(const sp<Filter>& filter)
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

bool AmlDemuxBase::Channel::detachFilter(const sp<Filter>& filter)
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

bool AmlDemuxBase::Channel::hasFilter() const
{
    std::lock_guard<std::mutex> _l(mLock);
    return !mFilters.empty();
}

void AmlDemuxBase::Channel::onData(const sp<ABuffer>& data, int version)
{
    std::set<sp<Filter>> filters;

    {
        std::lock_guard<std::mutex> _l(mLock);
        if (!enabled() || mFilters.empty()) {
            return;
        }

        filters = mFilters;
    }

    for (auto& f : filters) {
        f->notifyListener(data, version);
    }
}

bool AmlDemuxBase::Channel::enabled() const
{
    return mEnabled.load(std::memory_order_relaxed);
}

void AmlDemuxBase::Channel::setEnable(bool enable)
{
    mEnabled.store(enable, std::memory_order_relaxed);
}

////////////////////////////////////////////////////////////////////////////////
sp<AmlDemuxBase> AmlDemuxBase::create(bool isHardwareDemux)
{
    sp<AmlDemuxBase> demux;

    if (isHardwareDemux) {
        demux = new AmlHwDemux();
    } else {
        demux = new AmlSwDemux();
    }

    return demux;
}

AmlDemuxBase::AmlDemuxBase()
{

}

AmlDemuxBase::~AmlDemuxBase()
{

}

AmlDemuxBase::CHANNEL AmlDemuxBase::createChannel(int pid)
{
    sp<Channel> channel;

    {
        std::lock_guard<std::mutex> _l(mLock);
        if (isStopped()) {
            ALOGE("can't create channel for pid:%d, mStopped:%d", pid, isStopped());
            return NULL;
        }

        auto it = mChannels.find(pid);
        if (it != mChannels.end()) {
            ALOGV("return exist channel:%p", channel.get());
            channel = it->second;
            goto exit;
        }

        if (addPSISection(pid) < 0) {
            ALOGE("openHardwareChannel failed!");
            return nullptr;
        }

        channel = new Channel(pid);
        auto ret = mChannels.emplace(pid, channel);
        if (ret.second) {
            //ALOGI("create channel success!");
        }
    }

    channel->incStrong(this);

exit:
    return aml_handle_cast(channel);
}

int AmlDemuxBase::destroyChannel(CHANNEL _channel)
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

    removePSISection(pid);

    std::lock_guard<std::mutex> _l(mLock);
    auto it = mChannels.find(pid);
    if (it != mChannels.end()) {
        mChannels.erase(it);
    }

    channel->decStrong(this);
    return 0;
}

int AmlDemuxBase::openChannel(CHANNEL _channel)
{
    sp<Channel> channel = aml_handle_cast<Channel>(_channel);
    if (channel == nullptr) {
        return -1;
    }

    ALOGV("enable channel:%d", channel->pid());
    return 0;
}

int AmlDemuxBase::closeChannel(CHANNEL _channel)
{
    sp<Channel> channel = aml_handle_cast<Channel>(_channel);
    if (channel == nullptr) {
        return -1;
    }

    ALOGV("disable channel:%d", channel->pid());

    return 0;
}

AmlDemuxBase::FILTER AmlDemuxBase::createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData)
{
    sp<Filter> filter = new Filter(cb, userData, mFilterId++);

    filter->incStrong(this);
    return aml_handle_cast(filter);
}

int AmlDemuxBase::destroyFilter(FILTER _filter)
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

int AmlDemuxBase::attachFilter(FILTER _filter, CHANNEL _channel)
{
    sp<Filter> filter = aml_handle_cast<Filter>(_filter);
    sp<Channel> channel = aml_handle_cast<Channel>(_channel);

    if (filter == nullptr || channel == nullptr) {
        return -1;
    }

    channel->attachFilter(filter);

    return 0;
}

int AmlDemuxBase::detachFilter(FILTER _filter, CHANNEL _channel)
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

void AmlDemuxBase::notifyData(int pid, const sp<ABuffer>& data, int version)
{
    sp<Channel> channel;

    {
        std::unique_lock<std::mutex> _l(mLock);
        auto it = mChannels.find(pid);
        if (it != mChannels.end()) {
            channel = it->second;
        }
    }

    if (channel) {
        channel->onData(data, version);
    }
}

////////////////////////////////////////////////////////////////////////////////
AmlDemuxBase::ITsParser::ITsParser(const std::function<SectionCallback>& cb)
: mSectionCallback(cb)
{
    MLOG();
}

}
