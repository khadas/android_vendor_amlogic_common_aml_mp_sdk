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

#ifndef AML_MP_ATOMIZER_H_

#define AML_MP_ATOMIZER_H_

#include <stdint.h>

//#include <media/stagefright/foundation/ABase.h>
#include <string>
#include <list>
#include <vector>
#include <mutex>
//#include <utils/threads.h>

namespace aml_mp {

struct AmlMpAtomizer {
    static const char *Atomize(const char *name);

private:
    static AmlMpAtomizer gAtomizer;

    std::mutex mLock;
    std::vector<std::list<std::string> > mAtoms;

    AmlMpAtomizer();

    const char *atomize(const char *name);

    static uint32_t Hash(const char *s);

    AmlMpAtomizer(const AmlMpAtomizer&) = delete;
    AmlMpAtomizer& operator= (const AmlMpAtomizer&) =delete;
};

}  // namespace android

#endif  // A_ATOMIZER_H_
