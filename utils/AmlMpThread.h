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

#ifndef AML_MP_THREAD_H_
#define AML_MP_THREAD_H_

#include <mutex>
#include <condition_variable>
#include "AmlMpRefBase.h"

namespace aml_mp {
class AmlMpThread : virtual public AmlMpRefBase
{
public:
    AmlMpThread();
    virtual ~AmlMpThread();
    virtual int run(const char* name);
    virtual void requestExit();
    virtual int readyToRun();
    int requestExitAndWait();
    int join();
    bool isRunning();
    int getTid() const;

protected:
    bool exitPending() const;

private:
    virtual bool threadLoop() = 0;
    static int _threadLoop(void* user);
    mutable std::mutex mLock;
    std::condition_variable mThreadExitedCondition;
    int mStatus;
    volatile bool mExitPending;
    volatile bool mRunning;
    sptr<AmlMpThread> mHoldSelf;
    pthread_t mThread;
    int mTid;

private:
    AmlMpThread(const AmlMpThread&) = delete;
    AmlMpThread& operator= (const AmlMpThread&) = delete;
};
}


#endif
