/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpEventLooper"

//#include <media/stagefright/foundation/ADebug.h>

#include <utils/Log.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "AmlMpEventLooper.h"

#include "AmlMpEventHandler.h"
#include "AmlMpEventLooperRoster.h"
#include "AmlMpMessage.h"

namespace aml_mp {

AmlMpEventLooperRoster gLooperRoster;

struct AmlMpEventLooper::LooperThread : public AmlMpThread {
    LooperThread(AmlMpEventLooper *looper, bool canCallJava)
        : AmlMpThread(),
          mLooper(looper),
          mThreadId(NULL) {
    }

    virtual int readyToRun() {
        mThreadId = getTid();
        ALOGI("readyToRun mThreadId:%d", mThreadId);

        return AmlMpThread::readyToRun();
    }

    virtual bool threadLoop() {
        return mLooper->loop();
    }

    bool isCurrentThread() const {
        return mThreadId == syscall(__NR_gettid);
    }

protected:
    virtual ~LooperThread() {}

private:
    AmlMpEventLooper *mLooper;
    int mThreadId = -1;

    LooperThread(const LooperThread&) = delete;
    LooperThread& operator= (const LooperThread&) = delete;
};

// static
int64_t AmlMpEventLooper::GetNowUs() {
    //return systemTime(SYSTEM_TIME_MONOTONIC) / 1000LL;
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

AmlMpEventLooper::AmlMpEventLooper()
    : mRunningLocally(false) {
    // clean up stale AHandlers. Doing it here instead of in the destructor avoids
    // the side effect of objects being deleted from the unregister function recursively.
    gLooperRoster.unregisterStaleHandlers();
}

AmlMpEventLooper::~AmlMpEventLooper() {
    stop();
    // stale AHandlers are now cleaned up in the constructor of the next AmlMpEventLooper to come along
}

void AmlMpEventLooper::setName(const char *name) {
    mName = name;
}

AmlMpEventLooper::handler_id AmlMpEventLooper::registerHandler(const sptr<AmlMpEventHandler> &handler) {
    return gLooperRoster.registerHandler(this, handler);
}

void AmlMpEventLooper::unregisterHandler(handler_id handlerID) {
    gLooperRoster.unregisterHandler(handlerID);
}

int AmlMpEventLooper::start(
        bool runOnCallingThread, bool canCallJava, int32_t priority) {
    if (runOnCallingThread) {
        {
            std::lock_guard<std::mutex> autoLock(mLock);

            if (mThread != NULL || mRunningLocally) {
                return -EINVAL;
            }

            mRunningLocally = true;
        }

        do {
        } while (loop());

        return 0;
    }

    std::lock_guard<std::mutex> autoLock(mLock);

    if (mThread != NULL || mRunningLocally) {
        return -EINVAL;
    }

    mThread = new LooperThread(this, canCallJava);

    int err = mThread->run(
            mName.empty() ? "AmlMpEventLooper" : mName.c_str());
    if (err != 0) {
        mThread.clear();
    }

    return err;
}

int AmlMpEventLooper::stop() {
    sptr<LooperThread> thread;
    bool runningLocally;

    {
        std::lock_guard<std::mutex> autoLock(mLock);

        thread = mThread;
        runningLocally = mRunningLocally;
        mThread.clear();
        mRunningLocally = false;
    }

    if (thread == NULL && !runningLocally) {
        return -EINVAL;
    }

    if (thread != NULL) {
        thread->requestExit();
    }

    mQueueChangedCondition.notify_one();
    {
        std::lock_guard<std::mutex> autoLock(mRepliesLock);
        mRepliesCondition.notify_all();
    }

    if (!runningLocally && !thread->isCurrentThread()) {
        // If not running locally and this thread _is_ the looper thread,
        // the loop() function will return and never be called again.
        thread->requestExitAndWait();
    } else {
        ALOGE("CHECK!!!! runningLocally:%d", runningLocally);
    }

    return 0;
}

void AmlMpEventLooper::post(const sptr<AmlMpMessage> &msg, int64_t delayUs) {
    std::lock_guard<std::mutex> autoLock(mLock);

    int64_t whenUs;
    if (delayUs > 0) {
        int64_t nowUs = GetNowUs();
        whenUs = (delayUs > INT64_MAX - nowUs ? INT64_MAX : nowUs + delayUs);

    } else {
        whenUs = GetNowUs();
    }

    std::list<Event>::iterator it = mEventQueue.begin();
    while (it != mEventQueue.end() && (*it).mWhenUs <= whenUs) {
        ++it;
    }

    Event event;
    event.mWhenUs = whenUs;
    event.mMessage = msg;

    if (it == mEventQueue.begin()) {
        mQueueChangedCondition.notify_one();
    }

    mEventQueue.insert(it, event);
}

bool AmlMpEventLooper::loop() {
    Event event;

    {
        std::unique_lock<std::mutex> autoLock(mLock);
        if (mThread == NULL && !mRunningLocally) {
            return false;
        }
        if (mEventQueue.empty()) {
            mQueueChangedCondition.wait(autoLock);
            return true;
        }
        int64_t whenUs = (*mEventQueue.begin()).mWhenUs;
        int64_t nowUs = GetNowUs();

        if (whenUs > nowUs) {
            int64_t delayUs = whenUs - nowUs;
            if (delayUs > INT64_MAX / 1000) {
                delayUs = INT64_MAX / 1000;
            }
            mQueueChangedCondition.wait_for(autoLock, std::chrono::nanoseconds(delayUs * 1000ll));

            return true;
        }

        event = *mEventQueue.begin();
        mEventQueue.erase(mEventQueue.begin());
    }

    event.mMessage->deliver();

    // NOTE: It's important to note that at this point our "AmlMpEventLooper" object
    // may no longer exist (its final reference may have gone away while
    // delivering the message). We have made sure, however, that loop()
    // won't be called again.

    return true;
}

// to be called by AMessage::postAndAwaitResponse only
sptr<AReplyToken> AmlMpEventLooper::createReplyToken() {
    return new AReplyToken(this);
}

// to be called by AMessage::postAndAwaitResponse only
int AmlMpEventLooper::awaitResponse(const sptr<AReplyToken> &replyToken, sptr<AmlMpMessage> *response) {
    // return status in case we want to handle an interrupted wait
    std::unique_lock<std::mutex> autoLock(mRepliesLock);
    assert(replyToken != NULL);
    while (!replyToken->retrieveReply(response)) {
        {
            std::unique_lock<std::mutex> autoLock(mLock);
            if (mThread == NULL) {
                return -ENOENT;
            }
        }
        mRepliesCondition.wait(autoLock);
    }
    return 0;
}

int AmlMpEventLooper::postReply(const sptr<AReplyToken> &replyToken, const sptr<AmlMpMessage> &reply) {
    std::unique_lock<std::mutex> autoLock(mRepliesLock);
    int err = replyToken->setReply(reply);
    if (err == 0) {
        mRepliesCondition.notify_all();
    }
    return err;
}

}  // namespace aml_mp
