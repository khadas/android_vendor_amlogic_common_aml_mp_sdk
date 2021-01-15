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

#ifndef AML_MP_EVENT_HANDLER_H_

#define AML_MP_EVENT_HANDLER_H_

#include "AmlMpEventLooper.h"
#include <map>
#include "AmlMpRefBase.h"

namespace aml_mp {

struct AmlMpMessage;

struct AmlMpEventHandler : public AmlMpRefBase {
    AmlMpEventHandler()
        : mID(0),
          mVerboseStats(false),
          mMessageCounter(0) {
    }

    AmlMpEventLooper::handler_id id() const {
        return mID;
    }

    sptr<AmlMpEventLooper> looper() const {
        return mLooper.promote();
    }

    wptr<AmlMpEventLooper> getLooper() const {
        return mLooper;
    }

    wptr<AmlMpEventHandler> getHandler() const {
        // allow getting a weak reference to a const handler
        return const_cast<AmlMpEventHandler *>(this);
    }

protected:
    virtual void onMessageReceived(const sptr<AmlMpMessage> &msg) = 0;

private:
    friend struct AmlMpMessage;      // deliverMessage()
    friend struct AmlMpEventLooperRoster; // setID()

    AmlMpEventLooper::handler_id mID;
    wptr<AmlMpEventLooper> mLooper;

    inline void setID(AmlMpEventLooper::handler_id id, const wptr<AmlMpEventLooper> &looper) {
        mID = id;
        mLooper = looper;
    }

    bool mVerboseStats;
    uint32_t mMessageCounter;
    std::map<uint32_t, uint32_t> mMessages;

    void deliverMessage(const sptr<AmlMpMessage> &msg);

    AmlMpEventHandler(const AmlMpEventHandler&) = delete;
    AmlMpEventHandler& operator= (const AmlMpEventHandler&) = delete;
};

}  // namespace aml_mp

#endif  // A_HANDLER_H_
