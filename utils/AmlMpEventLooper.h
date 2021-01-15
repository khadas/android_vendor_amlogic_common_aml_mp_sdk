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

#ifndef AML_MP_EVENT_LOOPER_H_

#define AML_MP_EVENT_LOOPER_H_

//#include <media/stagefright/foundation/ABase.h>
#include <string>
//#include <utils/Errors.h>
#include <vector>
#include <list>
#include "AmlMpRefBase.h"
#include "AmlMpThread.h"

namespace aml_mp {

struct AmlMpEventHandler;
struct AmlMpMessage;
struct AReplyToken;

struct AmlMpEventLooper : public AmlMpRefBase {
    typedef int32_t event_id;
    typedef int32_t handler_id;

    AmlMpEventLooper();

    // Takes effect in a subsequent call to start().
    void setName(const char *name);

    handler_id registerHandler(const sptr<AmlMpEventHandler> &handler);
    void unregisterHandler(handler_id handlerID);

    int start(
            bool runOnCallingThread = false,
            bool canCallJava = false,
            int32_t priority = 0
            );

    int stop();

    static int64_t GetNowUs();

    const char *getName() const {
        return mName.c_str();
    }

protected:
    virtual ~AmlMpEventLooper();

private:
    friend struct AmlMpMessage;       // post()

    struct Event {
        int64_t mWhenUs;
        sptr<AmlMpMessage> mMessage;
    };

    std::mutex mLock;
    std::condition_variable mQueueChangedCondition;

    std::string mName;

    std::list<Event> mEventQueue;

    struct LooperThread;
    sptr<LooperThread> mThread;
    bool mRunningLocally;

    // use a separate lock for reply handling, as it is always on another thread
    // use a central lock, however, to avoid creating a mutex for each reply
    std::mutex mRepliesLock;
    std::condition_variable mRepliesCondition;

    // START --- methods used only by AMessage

    // posts a message on this looper with the given timeout
    void post(const sptr<AmlMpMessage> &msg, int64_t delayUs);

    // creates a reply token to be used with this looper
    sptr<AReplyToken> createReplyToken();
    // waits for a response for the reply token.  If status is OK, the response
    // is stored into the supplied variable.  Otherwise, it is unchanged.
    int awaitResponse(const sptr<AReplyToken> &replyToken, sptr<AmlMpMessage> *response);
    // posts a reply for a reply token.  If the reply could be successfully posted,
    // it returns OK. Otherwise, it returns an error value.
    int postReply(const sptr<AReplyToken> &replyToken, const sptr<AmlMpMessage> &msg);

    // END --- methods used only by AMessage

    bool loop();

   AmlMpEventLooper(const AmlMpEventLooper&) = delete;
   AmlMpEventLooper& operator=(const AmlMpEventLooper&) = delete;
};

} // namespace android

#endif  // A_LOOPER_H_
