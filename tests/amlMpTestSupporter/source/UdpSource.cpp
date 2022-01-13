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
#include <utils/AmlMpLog.h>
#include "UdpSource.h"
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <utils/Looper.h>
#include <cutils/properties.h>
#include <utils/AmlMpEventLooper.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define UDP_FIFO_SIZE (4 * 1024 * 1024)

static const char* mName = LOG_TAG;

namespace aml_mp {

UdpSource::UdpSource(const char* proto, const char* address, const InputParameter& inputParameter, uint32_t flags)
: Source(inputParameter, flags)
, mProto(proto)
, mAddress(address)
, mFifo(UDP_FIFO_SIZE)
{
    if (mProto == "rtp") {
        mIsRTP = true;
    }
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
        MLOGE("parsr port failed!");
        return -1;
    }

    if (strlen(mAddress.data()+p+1) > 9) {
        MLOGE("Port len is too large");
        return -1;
    }
    strcpy(port, mAddress.data()+p+1);
    strncpy(hostAddress, mAddress.data(), p);

    MLOGV("hostAddress:%s, port:%s", hostAddress, port);

    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags |= AI_NUMERICHOST;

    struct addrinfo* result = nullptr;
    ret = getaddrinfo(hostAddress, port, &hints, &result);
    if (ret != 0) {
        MLOGE("getaddrinfo failed, %s", gai_strerror(ret));
        return -1;
    }

    if (mAddrInfo) {
        freeaddrinfo(mAddrInfo);
        mAddrInfo = nullptr;
    }
    mAddrInfo = result;

    if (mAddrInfo->ai_family == AF_INET) {
        struct sockaddr_in* addr = (struct sockaddr_in*)(mAddrInfo->ai_addr);
        mIsMultiCast = IN_MULTICAST(ntohl(addr->sin_addr.s_addr));
        MLOGI("mIsMultiCast:%d", mIsMultiCast);
    }

    return 0;
}

int UdpSource::start()
{
    mSocket = ::socket(mAddrInfo->ai_family, mAddrInfo->ai_socktype, mAddrInfo->ai_protocol);
    if (mSocket < 0) {
        MLOGE("create socket failed! %s", strerror(errno));
        return -1;
    }

    int reuseAddr = 1;
    int ret = setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));
    if (ret < 0) {
        MLOGE("reuse sockaddr failed! %s", strerror(errno));
        return -1;
    }

#if 1
    int bufferSize = 1 * 1024 * 1024;
    ret = setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
    if (ret < 0) {
        MLOGE("set recvbuf failed! %s", strerror(errno));
    }

    int actualBufferSize = 0;
    #ifdef ANDROID
    socklen_t len = sizeof(actualBufferSize);
    #else
    socklen_t len = sizeof(actualBufferSize);
    #endif
    getsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, &actualBufferSize, &len);
    if (actualBufferSize != bufferSize) {
        MLOGE("actual set buffer size:%d", actualBufferSize);
    }
#endif

    ret = bind(mSocket, mAddrInfo->ai_addr, sizeof(sockaddr));
    if (ret < 0) {
        MLOGE("bind failed! %s", strerror(errno));
        return -1;
    }

   struct sockaddr localAddr;
    socklen_t localAddrLen = sizeof(localAddr);
    char hostAddress[100];
    char port[10];
    ret = getsockname(mSocket, &localAddr, &localAddrLen);
    if (ret < 0) {
        MLOGE("getsockname failed!\n");
        return -1;
    }

    ret = getnameinfo(&localAddr, localAddrLen, hostAddress, sizeof(hostAddress), port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);
    if (ret != 0) {
        MLOGE("getnameinfo failed! %s", gai_strerror(ret));
        return -1;
    }

    MLOGE("address:%s:%s", hostAddress, port);

    if (mIsMultiCast) {
        struct ip_mreq_source multiSource {};
        multiSource.imr_multiaddr = ((struct sockaddr_in*)&localAddr)->sin_addr;
        multiSource.imr_interface.s_addr = htonl(INADDR_ANY);
        ret = setsockopt(mSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multiSource, sizeof(multiSource));
        if (ret < 0) {
            MLOGE("join multicast failed! %s", strerror(errno));
            return -1;
        }

        MLOGE("join multicast success!\n");
    }

    mLooper = new Looper(Looper::PREPARE_ALLOW_NON_CALLBACKS);
    if (mLooper == nullptr) {
        MLOGE("create looper failed!");
        return -1;
    }

    bool enableDump = property_get_bool("vendor.amlmp.udpdump", false);
    if (enableDump) {
        mDumpFd = ::open("/data/aml_dump.ts", O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if (mDumpFd < 0) {
            MLOGE("open dump fil failed! %s", strerror(errno));
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

    MLOGI("join...");
    if (mReadThread.joinable()) {
        mReadThread.join();
    }

    if (mFeedThread.joinable()) {
        mFeedThread.join();
    }

    if (mIsMultiCast) {
        struct ip_mreq_source multiSource {};
        multiSource.imr_multiaddr = ((struct sockaddr_in*)mAddrInfo->ai_addr)->sin_addr;
        multiSource.imr_interface.s_addr = htonl(INADDR_ANY);
        int ret = setsockopt(mSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &multiSource, sizeof(multiSource));
        if (ret < 0) {
            MLOGE("leave multicast failed!");
        }
    }

    if (mDumpFd >= 0) {
        ::close(mDumpFd);
        mDumpFd = -1;
    }

    MLOGI("stopped!");
    return 0;
}

void UdpSource::signalQuit()
{
    mRequestQuit = true;
    if (mLooper) {
        mLooper->wake();
    }

    MLOGI("sendFeedWorkCommand");
    sendFeedWorkCommand(kWorkQuit);
}

void UdpSource::readThreadLoop()
{
    int ret = 0;

    if (mLooper == nullptr) {
        MLOGE("looper is NULL!");
        return;
    }
    Looper::setForThread(mLooper);

    ret = mLooper->addFd(mSocket, SOCKET_FD_IDENTIFIER, Looper::EVENT_INPUT|Looper::EVENT_ERROR, nullptr, nullptr);
    if (ret < 0) {
        MLOGE("looper addFd failed!");
        return;
    }

    for (;;) {
        if (mRequestQuit) {
            MLOGE("Quit!");
            break;
        }

        ret = mLooper->pollOnce(1000);
        if (ret == Looper::POLL_WAKE) {
            MLOGE("exit!");
            break;
        } else if (ret == Looper::POLL_ERROR) {
            MLOGE("error!");
            break;
        } else if (ret >= 0) {
            if (ret != SOCKET_FD_IDENTIFIER) {
                continue;
            }

            uint8_t buffer[4096];
            uint8_t* pBuf = buffer;
            ret = read(mSocket, buffer, sizeof(buffer));
            if (ret < 0) {
                MLOGE("read failed! %s", strerror(errno));
                break;
            } else if (ret == 0) {
                MLOGE("eof!");
            } else {
                doStatistic(ret);
                if (mIsRTP) {
                    if (parseRtpPayload(pBuf, ret) < 0) {
                        continue;
                    }
                }
                MLOGV("udp write data:%d", ret);
                if (mDumpFd >= 0) {
                    if (::write(mDumpFd, pBuf, ret) != ret) {
                        MLOGE("write dump file failed!");
                    }
                }

                if (mFifo.put(pBuf, ret) != ret) {
                    MLOGW("fifo full, reset!");
                    mFifo.reset();
                } else {
                    //MLOGI("sendFeedWorkCommand kWorkFeedData");
                    sendFeedWorkCommand(kWorkFeedData);
                }
            }
        }
    }

    MLOGI("UdpSource readThreadLoop exited!");
}

void UdpSource::feedThreadLoop()
{
    int len;
    sptr<ISourceReceiver> receiver = nullptr;
    static const int bufferSize = 188 * 1024;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);

    uint32_t work = 0;
    for (;;) {
        {
            std::unique_lock<std::mutex> _l(mFeedLock);
            mFeedCond.wait(_l, [this, &work] { return (work = mFeedWork) != 0;});
        }

        //MLOGI("feedWork = %#x", work);
        if (work & kWorkQuit) {
            MLOGI("quit feed thread!");
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

    MLOGI("UdpSource feedThreadLoop exited!");
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
            MLOGI("receive bitrate:%.2fMB/s, fifoSize:%d", (bitrate>>10)/1024.0, mFifo.size());

            mBitRateMeasureSize = 0;
            mLastBitRateMeasureTime = nowUs;
        }
    }
}

int UdpSource::parseRtpPayload(uint8_t*& buffer, int& size)
{
    int ext = buffer[0] & 0x10;
    int csrc = buffer[0] & 0x0F;
    int payloadType = buffer[1] & 0x7F;

    if (payloadType != 33) {
        MLOGE("payloadType:%d, NOT mpegts, drop it!", payloadType);
        size = 0;
        return -1;
    }

    if (buffer[0] & 0x20) {
        int padding = buffer[size-1];
        if (size > 12 + padding) {
            size -= padding;
        }
    }

    buffer += 12;
    size -= 12;

    size -= 4 * csrc;
    buffer += 4 * csrc;
    if (size < 0) {
        return -1;
    }

    if (ext) {
        ext = ((buffer[2]<<8|buffer[3])+1) * 4;
        if (size < ext) {
            return -1;
        }

        buffer += ext;
        size -= ext;
    }

    return 0;
}


}
