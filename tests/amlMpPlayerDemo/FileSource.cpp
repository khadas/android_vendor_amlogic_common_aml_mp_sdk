/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#define LOG_TAG "AmlMpPlayerDemo_FileSource"
#include <utils/Log.h>
#include <sys/stat.h>
#include "FileSource.h"

namespace aml_mp {
FileSource::FileSource(const char* filepath, int programNumber, uint32_t flags)
: Source(programNumber, flags)
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
        ALOGE("stat file failed!");
        return ret;
    }

    return 0;
}

int FileSource::start()
{
    mFd = ::open(mFilePath.c_str(), O_RDONLY);
    if (mFd < 0) {
        ALOGE("open %s failed, %s", mFilePath.c_str(), strerror(errno));
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
        ALOGI("join...");
        mThread.join();
        ALOGI("join done!");
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
    sp<ISourceReceiver> receiver = nullptr;
    uint32_t work = 0;
    for (;;) {
        {
            std::unique_lock<std::mutex> _l(mLock);
            mCond.wait(_l, [this, &work] {return (work = mWork) != 0; });
        }

        if (work & kWorkQuit) {
            ALOGI("Quit!");
            signalWorkDone(kWorkQuit);
            break;
        }

        if (work & kWorkRestart) {
            ALOGI("restart!");
            size = 0;
            signalWorkDone(kWorkRestart);

            ret = ::lseek64(mFd, SEEK_SET, 0);
            if (ret < 0) {
                ALOGE("seek to begin failed! %s", strerror(errno));
                break;
            }

            ALOGE("seek to begin!");
        }

        if (size == 0) {
            size = read(mFd, buffer.get(), bufferSize);
            data = buffer.get();
        }

        if (size < 0) {
            ALOGE("read return %d", size);
            break;
        } else if (size == 0) {
            sendWorkCommand(kWorkRestart);
            continue;
        } else {
            receiver = sourceReceiver();
            if (receiver == nullptr) {
                usleep(100*1000);
                ALOGI("receiver null!");
                continue;
            }

            written = receiver->writeData(data, size);
            if (written < 0) {
                ALOGI("written < 0");
                written = 0;
                usleep(10*1000);
            } else if (written == 0) {
                written = size;
            }

            data += written;
            size -= written;
        }
    }
}

}
