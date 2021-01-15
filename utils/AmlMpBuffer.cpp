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

#include "AmlMpBuffer.h"
#include <assert.h>

namespace aml_mp {

AmlMpBuffer::AmlMpBuffer(size_t capacity)
    : mRangeOffset(0),
      mInt32Data(0),
      mOwnsData(true) {
    mData = malloc(capacity);
    if (mData == NULL) {
        mCapacity = 0;
        mRangeLength = 0;
    } else {
        mCapacity = capacity;
        mRangeLength = capacity;
    }
}

AmlMpBuffer::AmlMpBuffer(void *data, size_t capacity)
    : mData(data),
      mCapacity(capacity),
      mRangeOffset(0),
      mRangeLength(capacity),
      mInt32Data(0),
      mOwnsData(false) {
}

// static
sptr<AmlMpBuffer> AmlMpBuffer::CreateAsCopy(const void *data, size_t capacity)
{
    sptr<AmlMpBuffer> res = new AmlMpBuffer(capacity);
    if (res->base() == NULL) {
        return NULL;
    }
    memcpy(res->data(), data, capacity);
    return res;
}

AmlMpBuffer::~AmlMpBuffer() {
    if (mOwnsData) {
        if (mData != NULL) {
            free(mData);
            mData = NULL;
        }
    }
}

void AmlMpBuffer::setRange(size_t offset, size_t size) {
    assert(offset <= mCapacity);
    assert(offset + size <= mCapacity);

    mRangeOffset = offset;
    mRangeLength = size;
}

}  // namespace android

