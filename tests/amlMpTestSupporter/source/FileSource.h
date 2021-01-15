/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _FILE_SOURCE_H_
#define _FILE_SOURCE_H_

#include "Source.h"
#include <string>
#include <thread>


namespace aml_mp {
class FileSource : public Source
{
public:
    FileSource(const char* address, int programNumber, uint32_t flags);
    ~FileSource();

    virtual int initCheck() override;
    virtual int start() override;
    virtual int stop() override;
    virtual int restart() override;
    virtual void signalQuit() override;

private:
    void threadLoop();

    std::string mFilePath;
    int mFd = -1;
    std::thread mThread;

    enum Work {
        kWorkFeedData   = 1 << 0,
        kWorkRestart    = 1 << 1,
        kWorkQuit       = 1 << 2,
    };

    void sendWorkCommand(Work work) {
        std::lock_guard<std::mutex> _l(mLock);
        mWork |= work;
        mCond.notify_all();
    }

    void signalWorkDone(Work work) {
        std::lock_guard<std::mutex> _l(mLock);
        mWork &= ~ work;
    }

    std::mutex mLock;
    std::condition_variable mCond;
    uint32_t mWork{};

    FileSource(const FileSource&) = delete;
    FileSource& operator= (const FileSource&) = delete;
};

}



#endif
