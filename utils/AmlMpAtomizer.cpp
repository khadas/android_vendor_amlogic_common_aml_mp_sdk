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

#include <sys/types.h>

#include "AmlMpAtomizer.h"

namespace aml_mp {

// static
AmlMpAtomizer AmlMpAtomizer::gAtomizer;

// static
const char *AmlMpAtomizer::Atomize(const char *name) {
    return gAtomizer.atomize(name);
}

AmlMpAtomizer::AmlMpAtomizer() {
    for (size_t i = 0; i < 128; ++i) {
        mAtoms.push_back(std::list<std::string>());
    }
}

const char *AmlMpAtomizer::atomize(const char *name) {
    std::lock_guard<std::mutex> autoLock(mLock);

    const size_t n = mAtoms.size();
    size_t index = AmlMpAtomizer::Hash(name) % n;
    std::list<std::string> &entry = mAtoms.at(index);
    std::list<std::string>::iterator it = entry.begin();
    while (it != entry.end()) {
        if ((*it) == name) {
            return (*it).c_str();
        }
        ++it;
    }

    entry.push_back(std::string(name));

    return (*--entry.end()).c_str();
}

// static
uint32_t AmlMpAtomizer::Hash(const char *s) {
    uint32_t sum = 0;
    while (*s != '\0') {
        sum = (sum * 31) + *s;
        ++s;
    }

    return sum;
}

}  // namespace android
