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

#ifndef _FILE_SOURCE_H_
#define _FILE_SOURCE_H_

#include "Source.h"
#include <string>
#include <thread>


namespace aml_mp {
using android::sp;

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
