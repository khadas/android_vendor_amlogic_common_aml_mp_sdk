/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _UDP_SOURCE_H_
#define _UDP_SOURCE_H_

#include "Source.h"
#include <string>
#include <thread>
#include <utils/AmlMpFifo.h>
#include <utils/AmlMpLooper.h>
#include <mutex>
#include <condition_variable>

struct addrinfo;

namespace aml_mp {

class UdpSource : public Source
{
public:
    UdpSource(const char* proto, const char* address, const InputParameter& inputParameter, uint32_t flags);
    ~UdpSource();

    virtual int initCheck() override;
    virtual int start() override;
    virtual int stop() override;
    virtual void signalQuit() override;

private:
    void readThreadLoop();
    void feedThreadLoop();
    void doStatistic(int size);
    int parseRtpPayload(uint8_t*& buffer, int& size);

    std::string mProto;
    std::string mAddress;
    bool mIsRTP = false;
    struct addrinfo* mAddrInfo = nullptr;
    bool mIsMultiCast = false;
    int mSocket = -1;
    std::thread mReadThread;
    sptr<Looper> mLooper;

    std::thread mFeedThread;
    std::mutex mFeedLock;
    std::condition_variable mFeedCond;
    AmlMpFifo mFifo;
    uint32_t mFeedWork{};

    enum FeedWork {
        kWorkFeedData   = 1 << 0,
        kWorkQuit       = 1 << 1,
    };

    void sendFeedWorkCommand(FeedWork work) {
        std::lock_guard<std::mutex> _l(mFeedLock);
        mFeedWork |= work;
        mFeedCond.notify_all();
    }

    void signalFeedWorkDone(FeedWork work) {
        std::lock_guard<std::mutex> _l(mFeedLock);
        mFeedWork &= ~ work;
    }

    static const int SOCKET_FD_IDENTIFIER = 0x100;

    int mDumpFd = -1;

    std::atomic_bool mRequestQuit = false;
    int64_t mLastBitRateMeasureTime = -1;
    int64_t mBitRateMeasureSize = 0;

private:
    UdpSource(const UdpSource&) = delete;
    UdpSource& operator= (const UdpSource&) = delete;
};



}
#endif
