/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_FileSource"
#include <utils/AmlMpLog.h>
#include <sys/stat.h>
#include <unistd.h>
#include "FileSource.h"
#include <fcntl.h>
#include <sys/types.h>

static const char* mName = LOG_TAG;

namespace aml_mp {
FileSource::FileSource(const char* filepath, const InputParameter& inputParameter, uint32_t flags)
: Source(inputParameter, flags)
, mFilePath(filepath)
{

}

FileSource::~FileSource()
{
    stop();
}

int FileSource::initCheck()
{
    struct stat s;
    int ret = stat(mFilePath.c_str(), &s);
    if (ret < 0) {
        MLOGE("stat file failed!");
        return ret;
    }

    return 0;
}

int FileSource::start()
{
    mFd = ::open(mFilePath.c_str(), O_RDONLY);
    if (mFd < 0) {
        MLOGE("open %s failed, %s", mFilePath.c_str(), strerror(errno));
        return -1;
    }

    mThread = std::thread([this] {
        threadLoop();
    });

    sendWorkCommand(kWorkFeedData);

    return 0;
}

int FileSource::stop()
{
    signalQuit();

    if (mThread.joinable()) {
        MLOGI("join...");
        mThread.join();
        MLOGI("join done!");
    }

    if (mFd >= 0) {
        ::close(mFd);
        mFd = -1;
    }

    return 0;
}

int FileSource::restart()
{
    sendWorkCommand(kWorkRestart);
    return 0;
}

void FileSource::signalQuit()
{
    sendWorkCommand(kWorkQuit);
}

void FileSource::threadLoop()
{
    const int bufferSize = 188 * 1024;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);
    const uint8_t* data = buffer.get();
    int size = 0;
    int ret = 0;
    int written = 0;
    sptr<ISourceReceiver> receiver = nullptr;
    uint32_t work = 0;
    for (;;) {
        {
            std::unique_lock<std::mutex> _l(mLock);
            mCond.wait(_l, [this, &work] {return (work = mWork) != 0; });
        }

        if (work & kWorkQuit) {
            MLOGI("Quit!");
            signalWorkDone(kWorkQuit);
            break;
        }

        if (work & kWorkRestart) {
            MLOGI("restart!");
            size = 0;
            signalWorkDone(kWorkRestart);

            ret = ::lseek64(mFd, SEEK_SET, 0);
            if (ret < 0) {
                MLOGE("seek to begin failed! %s", strerror(errno));
                break;
            }

            MLOGE("seek to begin!");
        }

        if (size == 0) {
            size = read(mFd, buffer.get(), bufferSize);
            data = buffer.get();
        }

        if (size < 0) {
            MLOGE("read return %d", size);
            break;
        } else if (size == 0) {
            sendWorkCommand(kWorkRestart);
            continue;
        } else {
            receiver = sourceReceiver();
            if (receiver == nullptr) {
                usleep(100*1000);
                MLOGI("receiver null!");
                continue;
            }

            written = receiver->writeData(data, size);
            if (written < 0) {
                //MLOGI("written < 0");
                written = 0;
                usleep(10*1000);
            } else if (written == 0) {
                written = size;
            }
            usleep(20*1000);

            data += written;
            size -= written;
        }
    }
}

}
