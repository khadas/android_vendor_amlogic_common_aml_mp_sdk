/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo_UdpSource"
#include <utils/Log.h>
#include "UdpSource.h"
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <utils/Looper.h>
#include <cutils/properties.h>
#include <utils/AmlMpEventLooper.h>

#define UDP_FIFO_SIZE (4 * 1024 * 1024)

namespace aml_mp {

UdpSource::UdpSource(const char* address, int programNumber, uint32_t flags)
: Source(programNumber, flags)
, mAddress(address)
, mFifo(UDP_FIFO_SIZE)
{
}

UdpSource::~UdpSource()
{
    stop();

    if (mAddrInfo) {
        freeaddrinfo(mAddrInfo);
        mAddrInfo = nullptr;
    }
}

int UdpSource::initCheck()
{
    char hostAddress[100];
    char port[10];
    int ret = 0;

    auto p = mAddress.find_last_of(':');
    if (p == std::string::npos) {
        ALOGE("parsr port failed!");
        return -1;
    }

    if (strlen(mAddress.data()+p+1) > 9) {
        ALOGE("Port len is too large");
        return -1;
    }
    strcpy(port, mAddress.data()+p+1);
    strncpy(hostAddress, mAddress.data(), p);

    ALOGV("hostAddress:%s, port:%s", hostAddress, port);

    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags |= AI_NUMERICHOST;

    struct addrinfo* result = nullptr;
    ret = getaddrinfo(hostAddress, port, &hints, &result);
    if (ret != 0) {
        ALOGE("getaddrinfo failed, %s", gai_strerror(ret));
        return -1;
    }

    if (mAddrInfo) {
        freeaddrinfo(mAddrInfo);
        mAddrInfo = nullptr;
    }
    mAddrInfo = result;

    return 0;
}

int UdpSource::start()
{
    mSocket = ::socket(mAddrInfo->ai_family, mAddrInfo->ai_socktype, mAddrInfo->ai_protocol);
    if (mSocket < 0) {
        ALOGE("create socket failed! %s", strerror(errno));
        return -1;
    }

    int reuseAddr = 1;
    int ret = setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));
    if (ret < 0) {
        ALOGE("reuse sockaddr failed! %s", strerror(errno));
        return -1;
    }

#if 0
    int bufferSize = 65536;
    ret = setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
    if (ret < 0) {
        ALOGE("set recvbuf failed! %s", strerror(errno));
    }

    int actualBufferSize = 0;
    int len = sizeof(actualBufferSize);
    getsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, &actualBufferSize, &len);
    if (actualBufferSize != bufferSize) {
        ALOGE("actual set buffer size:%d", actualBufferSize);
    }
#endif

    ret = bind(mSocket, mAddrInfo->ai_addr, sizeof(sockaddr));
    if (ret < 0) {
        ALOGE("bind failed! %s", strerror(errno));
        return -1;
    }

   struct sockaddr localAddr;
    socklen_t localAddrLen = sizeof(localAddr);
    char hostAddress[100];
    char port[10];
    ret = getsockname(mSocket, &localAddr, &localAddrLen);
    if (ret < 0) {
        ALOGE("getsockname failed!\n");
        return -1;
    }

    ret = getnameinfo(&localAddr, localAddrLen, hostAddress, sizeof(hostAddress), port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);
    if (ret != 0) {
        ALOGE("getnameinfo failed! %s", gai_strerror(ret));
        return -1;
    }

    ALOGE("address:%s:%s", hostAddress, port);

    struct ip_mreq_source multiSource{};
    multiSource.imr_multiaddr = ((struct sockaddr_in*)&localAddr)->sin_addr;
    multiSource.imr_interface.s_addr = htonl(INADDR_ANY);
    ret = setsockopt(mSocket,IPPROTO_IP, IP_ADD_MEMBERSHIP, &multiSource, sizeof(multiSource));
    if (ret < 0) {
        ALOGE("join multicast failed! %s", strerror(errno));
        return -1;
    }

    ALOGE("join multicast success!\n");

    mLooper = new Looper(Looper::PREPARE_ALLOW_NON_CALLBACKS);
    if (mLooper == nullptr) {
        ALOGE("create looper failed!");
        return -1;
    }

    bool enableDump = property_get_bool("vendor.amlmp.udpdump", false);
    if (enableDump) {
        mDumpFd = ::open("/data/aml_dump.ts", O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if (mDumpFd < 0) {
            ALOGE("open dump fil failed! %s", strerror(errno));
        }
    }

    mReadThread = std::thread([this] {
        readThreadLoop();
    });

    mFeedThread = std::thread([this] {
        feedThreadLoop();
    });

    return 0;
}

int UdpSource::stop()
{
    signalQuit();

    ALOGI("join...");
    if (mReadThread.joinable()) {
        mReadThread.join();
    }

    if (mFeedThread.joinable()) {
        mFeedThread.join();
    }

    struct ip_mreq_source multiSource{};
    multiSource.imr_multiaddr = ((struct sockaddr_in*)mAddrInfo->ai_addr)->sin_addr;
    multiSource.imr_interface.s_addr = htonl(INADDR_ANY);
    int ret = setsockopt(mSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &multiSource, sizeof(multiSource));
    if (ret < 0) {
        ALOGE("leave multicast failed!");
    }

    if (mDumpFd >= 0) {
        ::close(mDumpFd);
        mDumpFd = -1;
    }

    ALOGI("stopped!");
    return 0;
}

void UdpSource::signalQuit()
{
    mRequestQuit = true;
    if (mLooper) {
        mLooper->wake();
    }

    ALOGI("sendFeedWorkCommand");
    sendFeedWorkCommand(kWorkQuit);
}

void UdpSource::readThreadLoop()
{
    int ret = 0;

    if (mLooper == nullptr) {
        ALOGE("looper is NULL!");
        return;
    }
    Looper::setForThread(mLooper);

    ret = mLooper->addFd(mSocket, SOCKET_FD_IDENTIFIER, Looper::EVENT_INPUT|Looper::EVENT_ERROR, nullptr, nullptr);
    if (ret < 0) {
        ALOGE("looper addFd failed!");
        return;
    }

    for (;;) {
        if (mRequestQuit) {
            ALOGE("Quit!");
            break;
        }

        ret = mLooper->pollOnce(1000);
        if (ret == Looper::POLL_WAKE) {
            ALOGE("exit!");
            break;
        } else if (ret == Looper::POLL_ERROR) {
            ALOGE("error!");
            break;
        } else if (ret >= 0) {
            if (ret != SOCKET_FD_IDENTIFIER) {
                continue;
            }

            uint8_t buffer[4096];
            ret = read(mSocket, buffer, sizeof(buffer));
            if (ret < 0) {
                ALOGE("read failed! %s", strerror(errno));
                break;
            } else if (ret == 0) {
                ALOGE("eof!");
            } else {
                doStatistic(ret);
                ALOGV("udp write data:%d", ret);
                if (mDumpFd >= 0) {
                    if (::write(mDumpFd, buffer, ret) != ret) {
                        ALOGE("write dump file failed!");
                    }
                }

                if (mFifo.put(buffer, ret) != ret) {
                    ALOGW("fifo full, reset!");
                    mFifo.reset();
                } else {
                    //ALOGI("sendFeedWorkCommand kWorkFeedData");
                    sendFeedWorkCommand(kWorkFeedData);
                }
            }
        }
    }

    ALOGI("UdpSource readThreadLoop exited!");
}

void UdpSource::feedThreadLoop()
{
    int len;
    sptr<ISourceReceiver> receiver = nullptr;
    const int bufferSize = 188 * 1024;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);

    uint32_t work = 0;
    for (;;) {
        {
            std::unique_lock<std::mutex> _l(mFeedLock);
            mFeedCond.wait(_l, [this, &work] { return (work = mFeedWork) != 0;});
        }

        //ALOGI("feedWork = %#x", work);
        if (work & kWorkQuit) {
            ALOGI("quit feed thread!");
            signalFeedWorkDone(kWorkQuit);
            break;
        }

        if (work & kWorkFeedData) {
            if (mFifo.empty()) {
                continue;
            }

            len = mFifo.get(buffer.get(), bufferSize);
            if (len <= 0) {
                signalFeedWorkDone(kWorkFeedData);
            } else  {
                receiver = sourceReceiver();
                if (receiver != nullptr) {
                    receiver->writeData(buffer.get(), len);
                }
            }
        }
    }

    ALOGI("UdpSource feedThreadLoop exited!");
}

void UdpSource::doStatistic(int size)
{
    int64_t nowUs = AmlMpEventLooper::GetNowUs();
    if (mLastBitRateMeasureTime == -1) {
        mLastBitRateMeasureTime = nowUs;
    } else {
        mBitRateMeasureSize += size;
        int64_t period = nowUs - mLastBitRateMeasureTime;
        if (period >= 2000000) {
            int64_t bitrate = mBitRateMeasureSize * 1000000 / period;
            ALOGI("receive bitrate:%.2fMB/s, fifoSize:%d", (bitrate>>10)/1024.0, mFifo.size());

            mBitRateMeasureSize = 0;
            mLastBitRateMeasureTime = nowUs;
        }
    }
}


}
