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

#ifndef AML_MP_LOOPER_ROSTER_H_

#define AML_MP_LOOPER_ROSTER_H_

#include "AmlMpEventLooper.h"
#include <map>
#include <string>

namespace aml_mp {

struct AmlMpEventLooperRoster {
    AmlMpEventLooperRoster();

    AmlMpEventLooper::handler_id registerHandler(
            const sptr<AmlMpEventLooper> &looper, const sptr<AmlMpEventHandler> &handler);

    void unregisterHandler(AmlMpEventLooper::handler_id handlerID);
    void unregisterStaleHandlers();

    void dump(int fd, const std::vector<std::string>& args);

private:
    struct HandlerInfo {
        wptr<AmlMpEventLooper> mLooper;
        wptr<AmlMpEventHandler> mHandler;
    };

    std::mutex mLock;
    std::map<AmlMpEventLooper::handler_id, HandlerInfo> mHandlers;
    AmlMpEventLooper::handler_id mNextHandlerID;

    AmlMpEventLooperRoster(const AmlMpEventLooperRoster&) = delete;
    AmlMpEventLooperRoster& operator=(const AmlMpEventLooperRoster&) = delete;
};

}  // namespace aml_mp

#endif  // A_LOOPER_ROSTER_H_
