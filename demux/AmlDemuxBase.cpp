/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_AmlDemuxBase"
#include <utils/AmlMpLog.h>
#include "AmlDemuxBase.h"
#include "AmlHwDemux.h"
#include "AmlSwDemux.h"
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpBuffer.h>
#include <sstream>
#include <set>

static const char* mName = LOG_TAG;

namespace aml_mp {

////////////////////////////////////////////////////////////////////////////////
struct AmlDemuxBase::Filter : public AmlMpHandle
{
    Filter(Aml_MP_Demux_SectionFilterCb cb, void* userData, int id);
    ~Filter();

    void notifyListener(int pid, const sptr<AmlMpBuffer>& data, int version);
    void setOwner(const sptr<AmlDemuxBase::Channel>& channel);
    bool hasOwner() const;
    int id() const {
        return mId;
    }

    wptr<AmlDemuxBase::Channel> getOwner() const {
        return mChannel;
    }

private:
    Aml_MP_Demux_SectionFilterCb mCb;
    void* pUserData;
    const int mId;
    int mVersion;
    wptr<AmlDemuxBase::Channel> mChannel;

private:
    Filter(const Filter&);
    Filter& operator= (const Filter&);
};

struct AmlDemuxBase::Channel : public AmlMpHandle
{
    Channel(int pid);
    ~Channel();
    bool attachFilter(const sptr<Filter>& filter);
    bool detachFilter(const sptr<Filter>& filter);
    bool hasFilter() const;
    void onData(int pid, const sptr<AmlMpBuffer>& data, int version);
    int pid() const {
        return mPid;
    }

    bool enabled() const;
    void setEnable(bool enable);

private:
    const int mPid = AML_MP_INVALID_PID;

    std::atomic_bool mEnabled {true};

    mutable std::mutex mLock;
    std::set<sptr<Filter>> mFilters;

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

void AmlDemuxBase::Filter::notifyListener(int pid, const sptr<AmlMpBuffer>& data, int version)
{
    if (mCb == nullptr) {
        MLOGW("user cb is NULL!");
        return;
    }

    if (mVersion >= 0 && version >= 0 && mVersion == version) {
        return;
    }

    mVersion = version;
    if (mVersion >= 0) {
        MLOGI("notify filter:%d, version:%d", mId, mVersion);
    }

    mCb(pid, data->size(), data->base(), pUserData);
}

void AmlDemuxBase::Filter::setOwner(const sptr<AmlDemuxBase::Channel>& channel)
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

bool AmlDemuxBase::Channel::attachFilter(const sptr<Filter>& filter)
{
    {
        std::lock_guard<std::mutex> _l(mLock);
        mFilters.insert(filter);
        MLOGV("attach filter:%d to channel:%d, totoal filters num:%d",
            filter->id(), mPid, mFilters.size());
    }

    filter->setOwner(this);

    return true;
}

bool AmlDemuxBase::Channel::detachFilter(const sptr<Filter>& filter)
{
    std::lock_guard<std::mutex> _l(mLock);

    if (mFilters.find(filter) != mFilters.end()) {
        filter->setOwner(nullptr);
        mFilters.erase(filter);
        MLOGV("detach filter:%d from channel:%d, total filters num:%d",
            filter->id(), mPid, mFilters.size());
    }

    return true;
}

bool AmlDemuxBase::Channel::hasFilter() const
{
    std::lock_guard<std::mutex> _l(mLock);
    return !mFilters.empty();
}

void AmlDemuxBase::Channel::onData(int pid, const sptr<AmlMpBuffer>& data, int version)
{
    std::set<sptr<Filter>> filters;

    {
        std::lock_guard<std::mutex> _l(mLock);
        if (!enabled() || mFilters.empty()) {
            return;
        }

        filters = mFilters;
    }

    for (auto& f : filters) {
        f->notifyListener(pid, data, version);
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
sptr<AmlDemuxBase> AmlDemuxBase::create(bool isHardwareDemux)
{
    sptr<AmlDemuxBase> demux;

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

AmlDemuxBase::CHANNEL AmlDemuxBase::createChannel(int pid, bool checkCRC)
{
    Channel* channel;

    {
        std::lock_guard<std::mutex> _l(mLock);
        if (isStopped()) {
            MLOGE("can't create channel for pid:%d, mStopped:%d", pid, isStopped());
            return NULL;
        }

        auto it = mChannels.find(pid);
        if (it != mChannels.end()) {
            MLOGV("return exist channel:%p", channel);
            channel = it->second;
            goto exit;
        }

        if (addPSISection(pid, checkCRC) < 0) {
            MLOGE("openHardwareChannel failed!");
            return nullptr;
        }

        channel = new Channel(pid);
        auto ret = mChannels.emplace(pid, channel);
        if (ret.second) {
            //MLOGI("create channel success!");
        }
    }

    channel->incStrong(this);

exit:
    return aml_handle_cast(channel);
}

int AmlDemuxBase::destroyChannel(CHANNEL _channel)
{
    Channel* channel = aml_handle_cast<Channel>(_channel);
    if (channel == nullptr) {
        return -1;
    }

    int pid = channel->pid();
    MLOG("channel:%d", pid);

    if (channel->hasFilter()) {
        MLOGI("channel:%d has filter, don't do destroy!", channel->pid());
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
    Channel* channel = aml_handle_cast<Channel>(_channel);
    if (channel == nullptr) {
        return -1;
    }

    MLOGV("enable channel:%d", channel->pid());
    return 0;
}

int AmlDemuxBase::closeChannel(CHANNEL _channel)
{
    Channel* channel = aml_handle_cast<Channel>(_channel);
    if (channel == nullptr) {
        return -1;
    }

    MLOGV("disable channel:%d", channel->pid());

    return 0;
}

AmlDemuxBase::FILTER AmlDemuxBase::createFilter(Aml_MP_Demux_SectionFilterCb cb, void* userData)
{
    Filter* filter(new Filter(cb, userData, mFilterId++));

    filter->incStrong(this);
    return aml_handle_cast(filter);
}

int AmlDemuxBase::destroyFilter(FILTER _filter)
{
    Filter* filter = aml_handle_cast<Filter>(_filter);
    if (filter == nullptr) {
        return -1;
    }

    if (filter->hasOwner()) {
        MLOGW("filter has not been detached before!!!");
        sptr<Channel> channel = filter->getOwner().promote();
        if (channel != nullptr) {
            channel->detachFilter(filter);
        }
    }

    filter->decStrong(this);
    return 0;
}

int AmlDemuxBase::attachFilter(FILTER _filter, CHANNEL _channel)
{
    Filter* filter = aml_handle_cast<Filter>(_filter);
    Channel* channel = aml_handle_cast<Channel>(_channel);

    if (filter == nullptr || channel == nullptr) {
        return -1;
    }

    channel->attachFilter(filter);

    return 0;
}

int AmlDemuxBase::detachFilter(FILTER _filter, CHANNEL _channel)
{
    Filter* filter = aml_handle_cast<Filter>(_filter);
    Channel* channel = aml_handle_cast<Channel>(_channel);

    if (filter == nullptr || channel == nullptr) {
        return -1;
    }

    if (!channel->hasFilter()) {
        MLOGW("channel has no filter!!!");
        return -1;
    }

    channel->detachFilter(filter);

    return 0;
}

void AmlDemuxBase::notifyData(int pid, const sptr<AmlMpBuffer>& data, int version)
{
    sptr<Channel> channel;

    {
        std::unique_lock<std::mutex> _l(mLock);
        auto it = mChannels.find(pid);
        if (it != mChannels.end()) {
            channel = it->second;
        }
    }

    if (channel) {
        channel->onData(pid, data, version);
    }
}

////////////////////////////////////////////////////////////////////////////////
AmlDemuxBase::ITsParser::ITsParser(const std::function<SectionCallback>& cb)
: mSectionCallback(cb)
{
    MLOG();
}

}
