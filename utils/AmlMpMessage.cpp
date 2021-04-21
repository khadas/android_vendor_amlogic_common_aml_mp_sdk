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

#define LOG_TAG "AmlMpMessage"
//#define LOG_NDEBUG 0
//#define DUMP_STATS

#include <ctype.h>

#include "AmlMpMessage.h"

//#include <binder/Parcel.h>
#include <utils/AmlMpLog.h>

#include "AmlMpAtomizer.h"
#include "AmlMpBuffer.h"
#include "AmlMpEventLooperRoster.h"
#include "AmlMpEventHandler.h"
#include <string>

//#include <media/stagefright/foundation/hexdump.h>
#ifdef ANDROID
#include <media/stagefright/foundation/ADebug.h>
#endif

static const char* mName = LOG_TAG;

namespace aml_mp {

extern AmlMpEventLooperRoster gLooperRoster;

int AReplyToken::setReply(const sptr<AmlMpMessage> &reply) {
    if (mReplied) {
        MLOGE("trying to post a duplicate reply");
        return -EBUSY;
    }
#ifndef ANDROID
    CHECK(mReply == NULL);
#endif
    mReply = reply;
    mReplied = true;
    return 0;
}

AmlMpMessage::AmlMpMessage(void)
    : mWhat(0),
      mTarget(0),
      mNumItems(0) {
}

AmlMpMessage::AmlMpMessage(uint32_t what, const sptr<const AmlMpEventHandler> &handler)
    : mWhat(what),
      mNumItems(0) {
    setTarget(handler);
}

AmlMpMessage::~AmlMpMessage() {
    clear();
}

void AmlMpMessage::setWhat(uint32_t what) {
    mWhat = what;
}

uint32_t AmlMpMessage::what() const {
    return mWhat;
}

void AmlMpMessage::setTarget(const sptr<const AmlMpEventHandler> &handler) {
    if (handler == NULL) {
        mTarget = 0;
        mHandler.clear();
        mLooper.clear();
    } else {
        mTarget = handler->id();
        mHandler = handler->getHandler();
        mLooper = handler->getLooper();
    }
}

void AmlMpMessage::clear() {
    for (size_t i = 0; i < mNumItems; ++i) {
        Item *item = &mItems[i];
        delete[] item->mName;
        item->mName = NULL;
        freeItemValue(item);
    }
    mNumItems = 0;
}

void AmlMpMessage::freeItemValue(Item *item) {
    switch (item->mType) {
        case kTypeString:
        {
            delete item->u.stringValue;
            break;
        }

        case kTypeObject:
        case kTypeMessage:
        case kTypeBuffer:
        {
            if (item->u.refValue != NULL) {
                item->u.refValue->decStrong(this);
            }
            break;
        }

        default:
            break;
    }
    item->mType = kTypeInt32; // clear type
}

#ifdef DUMP_STATS
#include <utils/Mutex.h>

Mutex gLock;
static int32_t gFindItemCalls = 1;
static int32_t gDupCalls = 1;
static int32_t gAverageNumItems = 0;
static int32_t gAverageNumChecks = 0;
static int32_t gAverageNumMemChecks = 0;
static int32_t gAverageDupItems = 0;
static int32_t gLastChecked = -1;

static void reportStats() {
    int32_t time = (AmlMpEventLooper::GetNowUs() / 1000);
    if (time / 1000 != gLastChecked / 1000) {
        gLastChecked = time;
        MLOGI("called findItemIx %zu times (for len=%.1f i=%.1f/%.1f mem) dup %zu times (for len=%.1f)",
                gFindItemCalls,
                gAverageNumItems / (float)gFindItemCalls,
                gAverageNumChecks / (float)gFindItemCalls,
                gAverageNumMemChecks / (float)gFindItemCalls,
                gDupCalls,
                gAverageDupItems / (float)gDupCalls);
        gFindItemCalls = gDupCalls = 1;
        gAverageNumItems = gAverageNumChecks = gAverageNumMemChecks = gAverageDupItems = 0;
        gLastChecked = time;
    }
}
#endif

inline size_t AmlMpMessage::findItemIndex(const char *name, size_t len) const {
#ifdef DUMP_STATS
    size_t memchecks = 0;
#endif
    size_t i = 0;
    for (; i < mNumItems; i++) {
        if (len != mItems[i].mNameLength) {
            continue;
        }
#ifdef DUMP_STATS
        ++memchecks;
#endif
        if (!memcmp(mItems[i].mName, name, len)) {
            break;
        }
    }
#ifdef DUMP_STATS
    {
        Mutex::Autolock _l(gLock);
        ++gFindItemCalls;
        gAverageNumItems += mNumItems;
        gAverageNumMemChecks += memchecks;
        gAverageNumChecks += i;
        reportStats();
    }
#endif
    return i;
}

// assumes item's name was uninitialized or NULL
void AmlMpMessage::Item::setName(const char *name, size_t len) {
    mNameLength = len;
    mName = new char[len + 1];
    memcpy((void*)mName, name, len + 1);
}

AmlMpMessage::Item *AmlMpMessage::allocateItem(const char *name) {
    size_t len = strlen(name);
    size_t i = findItemIndex(name, len);
    Item *item;

    if (i < mNumItems) {
        item = &mItems[i];
        freeItemValue(item);
    } else {
        CHECK(mNumItems < kMaxNumItems);
        i = mNumItems++;
        item = &mItems[i];
        item->mType = kTypeInt32;
        item->setName(name, len);
    }

    return item;
}

const AmlMpMessage::Item *AmlMpMessage::findItem(
        const char *name, Type type) const {
    size_t i = findItemIndex(name, strlen(name));
    if (i < mNumItems) {
        const Item *item = &mItems[i];
        return item->mType == type ? item : NULL;

    }
    return NULL;
}

bool AmlMpMessage::findAsFloat(const char *name, float *value) const {
    size_t i = findItemIndex(name, strlen(name));
    if (i < mNumItems) {
        const Item *item = &mItems[i];
        switch (item->mType) {
            case kTypeFloat:
                *value = item->u.floatValue;
                return true;
            case kTypeDouble:
                *value = (float)item->u.doubleValue;
                return true;
            case kTypeInt64:
                *value = (float)item->u.int64Value;
                return true;
            case kTypeInt32:
                *value = (float)item->u.int32Value;
                return true;
            case kTypeSize:
                *value = (float)item->u.sizeValue;
                return true;
            default:
                return false;
        }
    }
    return false;
}

bool AmlMpMessage::findAsInt64(const char *name, int64_t *value) const {
    size_t i = findItemIndex(name, strlen(name));
    if (i < mNumItems) {
        const Item *item = &mItems[i];
        switch (item->mType) {
            case kTypeInt64:
                *value = item->u.int64Value;
                return true;
            case kTypeInt32:
                *value = item->u.int32Value;
                return true;
            default:
                return false;
        }
    }
    return false;
}

bool AmlMpMessage::contains(const char *name) const {
    size_t i = findItemIndex(name, strlen(name));
    return i < mNumItems;
}

#define BASIC_TYPE(NAME,FIELDNAME,TYPENAME)                             \
void AmlMpMessage::set##NAME(const char *name, TYPENAME value) {            \
    Item *item = allocateItem(name);                                    \
                                                                        \
    item->mType = kType##NAME;                                          \
    item->u.FIELDNAME = value;                                          \
}                                                                       \
                                                                        \
/* NOLINT added to avoid incorrect warning/fix from clang.tidy */       \
bool AmlMpMessage::find##NAME(const char *name, TYPENAME *value) const {  /* NOLINT */ \
    const Item *item = findItem(name, kType##NAME);                     \
    if (item) {                                                         \
        *value = item->u.FIELDNAME;                                     \
        return true;                                                    \
    }                                                                   \
    return false;                                                       \
}

BASIC_TYPE(Int32,int32Value,int32_t)
BASIC_TYPE(Int64,int64Value,int64_t)
BASIC_TYPE(Size,sizeValue,size_t)
BASIC_TYPE(Float,floatValue,float)
BASIC_TYPE(Double,doubleValue,double)
BASIC_TYPE(Pointer,ptrValue,void *)

#undef BASIC_TYPE

void AmlMpMessage::setString(
        const char *name, const char *s, ssize_t len) {
    Item *item = allocateItem(name);
    item->mType = kTypeString;
    item->u.stringValue = new std::string(s, len < 0 ? strlen(s) : len);
}

void AmlMpMessage::setString(
        const char *name, const std::string &s) {
    setString(name, s.c_str(), s.size());
}

void AmlMpMessage::setObjectInternal(
        const char *name, const sptr<AmlMpRefBase> &obj, Type type) {
    Item *item = allocateItem(name);
    item->mType = type;

    if (obj != NULL) { obj->incStrong(this); }
    item->u.refValue = obj.get();
}

void AmlMpMessage::setObject(const char *name, const sptr<AmlMpRefBase> &obj) {
    setObjectInternal(name, obj, kTypeObject);
}

void AmlMpMessage::setBuffer(const char *name, const sptr<AmlMpBuffer> &buffer) {
    setObjectInternal(name, sptr<AmlMpRefBase>(buffer), kTypeBuffer);
}

void AmlMpMessage::setMessage(const char *name, const sptr<AmlMpMessage> &obj) {
    Item *item = allocateItem(name);
    item->mType = kTypeMessage;

    if (obj != NULL) { obj->incStrong(this); }
    item->u.refValue = obj.get();
}

void AmlMpMessage::setRect(
        const char *name,
        int32_t left, int32_t top, int32_t right, int32_t bottom) {
    Item *item = allocateItem(name);
    item->mType = kTypeRect;

    item->u.rectValue.mLeft = left;
    item->u.rectValue.mTop = top;
    item->u.rectValue.mRight = right;
    item->u.rectValue.mBottom = bottom;
}

bool AmlMpMessage::findString(const char *name, std::string *value) const {
    const Item *item = findItem(name, kTypeString);
    if (item) {
        *value = *item->u.stringValue;
        return true;
    }
    return false;
}

bool AmlMpMessage::findObject(const char *name, sptr<AmlMpRefBase> *obj) const {
    const Item *item = findItem(name, kTypeObject);
    if (item) {
        *obj = item->u.refValue;
        return true;
    }
    return false;
}

bool AmlMpMessage::findBuffer(const char *name, sptr<AmlMpBuffer> *buf) const {
    const Item *item = findItem(name, kTypeBuffer);
    if (item) {
        *buf = (AmlMpBuffer *)(item->u.refValue);
        return true;
    }
    return false;
}

bool AmlMpMessage::findMessage(const char *name, sptr<AmlMpMessage> *obj) const {
    const Item *item = findItem(name, kTypeMessage);
    if (item) {
        *obj = static_cast<AmlMpMessage *>(item->u.refValue);
        return true;
    }
    return false;
}

bool AmlMpMessage::findRect(
        const char *name,
        int32_t *left, int32_t *top, int32_t *right, int32_t *bottom) const {
    const Item *item = findItem(name, kTypeRect);
    if (item == NULL) {
        return false;
    }

    *left = item->u.rectValue.mLeft;
    *top = item->u.rectValue.mTop;
    *right = item->u.rectValue.mRight;
    *bottom = item->u.rectValue.mBottom;

    return true;
}

void AmlMpMessage::deliver() {
    sptr<AmlMpEventHandler> handler = mHandler.promote();
    if (handler == NULL) {
        MLOGW("failed to deliver message as target handler %d is gone.", mTarget);
        return;
    }

    handler->deliverMessage(this);
}

int AmlMpMessage::post(int64_t delayUs) {
    sptr<AmlMpEventLooper> looper = mLooper.promote();
    if (looper == NULL) {
        MLOGW("failed to post message as target looper for handler %d is gone.", mTarget);
        return -ENOENT;
    }

    looper->post(this, delayUs);
    return 0;
}

int AmlMpMessage::postAndAwaitResponse(sptr<AmlMpMessage> *response) {
    sptr<AmlMpEventLooper> looper = mLooper.promote();
    if (looper == NULL) {
        MLOGW("failed to post message as target looper for handler %d is gone.", mTarget);
        return -ENOENT;
    }

    sptr<AReplyToken> token = looper->createReplyToken();
    if (token == NULL) {
        MLOGE("failed to create reply token");
        return -ENOMEM;
    }
    setObject("replyID", token);

    looper->post(this, 0 /* delayUs */);
    return looper->awaitResponse(token, response);
}

int AmlMpMessage::postReply(const sptr<AReplyToken> &replyToken) {
    if (replyToken == NULL) {
        MLOGW("failed to post reply to a NULL token");
        return -ENOENT;
    }
    sptr<AmlMpEventLooper> looper = replyToken->getLooper();
    if (looper == NULL) {
        MLOGW("failed to post reply as target looper is gone.");
        return -ENOENT;
    }
    return looper->postReply(replyToken, this);
}

bool AmlMpMessage::senderAwaitsResponse(sptr<AReplyToken> *replyToken) {
    sptr<AmlMpRefBase> tmp;
    bool found = findObject("replyID", &tmp);

    if (!found) {
        return false;
    }

    *replyToken = static_cast<AReplyToken *>(tmp.get());
    tmp.clear();
    setObject("replyID", tmp);
    // TODO: delete Object instead of setting it to NULL

    return *replyToken != NULL;
}

sptr<AmlMpMessage> AmlMpMessage::dup() const {
    sptr<AmlMpMessage> msg = new AmlMpMessage(mWhat, mHandler.promote());
    msg->mNumItems = mNumItems;

#ifdef DUMP_STATS
    {
        Mutex::Autolock _l(gLock);
        ++gDupCalls;
        gAverageDupItems += mNumItems;
        reportStats();
    }
#endif

    for (size_t i = 0; i < mNumItems; ++i) {
        const Item *from = &mItems[i];
        Item *to = &msg->mItems[i];

        to->setName(from->mName, from->mNameLength);
        to->mType = from->mType;

        switch (from->mType) {
            case kTypeString:
            {
                to->u.stringValue =
                    new std::string(*from->u.stringValue);
                break;
            }

            case kTypeObject:
            case kTypeBuffer:
            {
                to->u.refValue = from->u.refValue;
                to->u.refValue->incStrong(msg.get());
                break;
            }

            case kTypeMessage:
            {
                sptr<AmlMpMessage> copy =
                    static_cast<AmlMpMessage *>(from->u.refValue)->dup();

                to->u.refValue = copy.get();
                to->u.refValue->incStrong(msg.get());
                break;
            }

            default:
            {
                to->u = from->u;
                break;
            }
        }
    }

    return msg;
}

static void appendIndent(std::string *s, int32_t indent) {
    static const char kWhitespace[] =
        "                                        "
        "                                        ";

    //CHECK_LT((size_t)indent, sizeof(kWhitespace));

    s->append(kWhitespace, indent);
}

static bool isFourcc(uint32_t what) {
    return isprint(what & 0xff)
        && isprint((what >> 8) & 0xff)
        && isprint((what >> 16) & 0xff)
        && isprint((what >> 24) & 0xff);
}

std::string AmlMpMessage::debugString(int32_t indent) const {
    std::string s = "AmlMpMessage(what = ";
#if 0
    std::string tmp;
    if (isFourcc(mWhat)) {
        tmp = AStringPrintf(
                "'%c%c%c%c'",
                (char)(mWhat >> 24),
                (char)((mWhat >> 16) & 0xff),
                (char)((mWhat >> 8) & 0xff),
                (char)(mWhat & 0xff));
    } else {
        tmp = std::stringPrintf("0x%08x", mWhat);
    }
    s.append(tmp);

    if (mTarget != 0) {
        tmp = AStringPrintf(", target = %d", mTarget);
        s.append(tmp);
    }
    s.append(") = {\n");

    for (size_t i = 0; i < mNumItems; ++i) {
        const Item &item = mItems[i];

        switch (item.mType) {
            case kTypeInt32:
                tmp = AStringPrintf(
                        "int32_t %s = %d", item.mName, item.u.int32Value);
                break;
            case kTypeInt64:
                tmp = AStringPrintf(
                        "int64_t %s = %lld", item.mName, item.u.int64Value);
                break;
            case kTypeSize:
                tmp = AStringPrintf(
                        "size_t %s = %d", item.mName, item.u.sizeValue);
                break;
            case kTypeFloat:
                tmp = AStringPrintf(
                        "float %s = %f", item.mName, item.u.floatValue);
                break;
            case kTypeDouble:
                tmp = AStringPrintf(
                        "double %s = %f", item.mName, item.u.doubleValue);
                break;
            case kTypePointer:
                tmp = AStringPrintf(
                        "void *%s = %p", item.mName, item.u.ptrValue);
                break;
            case kTypeString:
                tmp = AStringPrintf(
                        "string %s = \"%s\"",
                        item.mName,
                        item.u.stringValue->c_str());
                break;
            case kTypeObject:
                tmp = AStringPrintf(
                        "AmlMpRefBase *%s = %p", item.mName, item.u.refValue);
                break;
            case kTypeBuffer:
            {
                sptr<AmlMpBuffer> buffer = static_cast<AmlMpBuffer *>(item.u.refValue);

                if (buffer != NULL && buffer->data() != NULL && buffer->size() <= 64) {
                    tmp = AStringPrintf("Buffer %s = {\n", item.mName);
                    hexdump(buffer->data(), buffer->size(), indent + 4, &tmp);
                    appendIndent(&tmp, indent + 2);
                    tmp.append("}");
                } else {
                    tmp = AStringPrintf(
                            "Buffer *%s = %p", item.mName, buffer.get());
                }
                break;
            }
            case kTypeMessage:
                tmp = AStringPrintf(
                        "AmlMpMessage %s = %s",
                        item.mName,
                        static_cast<AmlMpMessage *>(
                            item.u.refValue)->debugString(
                                indent + strlen(item.mName) + 14).c_str());
                break;
            case kTypeRect:
                tmp = AStringPrintf(
                        "Rect %s(%d, %d, %d, %d)",
                        item.mName,
                        item.u.rectValue.mLeft,
                        item.u.rectValue.mTop,
                        item.u.rectValue.mRight,
                        item.u.rectValue.mBottom);
                break;
            default:
                TRESPASS();
        }

        appendIndent(&s, indent);
        s.append("  ");
        s.append(tmp);
        s.append("\n");
    }

    appendIndent(&s, indent);
    s.append("}");
#endif

    return s;
}

// static
#if 0
sptr<AmlMpMessage> AmlMpMessage::FromParcel(const Parcel &parcel, size_t maxNestingLevel) {
    int32_t what = parcel.readInt32();
    sptr<AmlMpMessage> msg = new AmlMpMessage();
    msg->setWhat(what);

    msg->mNumItems = static_cast<size_t>(parcel.readInt32());
    if (msg->mNumItems > kMaxNumItems) {
        MLOGE("Too large number of items clipped.");
        msg->mNumItems = kMaxNumItems;
    }

    for (size_t i = 0; i < msg->mNumItems; ++i) {
        Item *item = &msg->mItems[i];

        const char *name = parcel.readCString();
        if (name == NULL) {
            MLOGE("Failed reading name for an item. Parsing aborted.");
            msg->mNumItems = i;
            break;
        }

        item->mType = static_cast<Type>(parcel.readInt32());
        // setName() happens below so that we don't leak memory when parsing
        // is aborted in the middle.
        switch (item->mType) {
            case kTypeInt32:
            {
                item->u.int32Value = parcel.readInt32();
                break;
            }

            case kTypeInt64:
            {
                item->u.int64Value = parcel.readInt64();
                break;
            }

            case kTypeSize:
            {
                item->u.sizeValue = static_cast<size_t>(parcel.readInt32());
                break;
            }

            case kTypeFloat:
            {
                item->u.floatValue = parcel.readFloat();
                break;
            }

            case kTypeDouble:
            {
                item->u.doubleValue = parcel.readDouble();
                break;
            }

            case kTypeString:
            {
                const char *stringValue = parcel.readCString();
                if (stringValue == NULL) {
                    MLOGE("Failed reading string value from a parcel. "
                        "Parsing aborted.");
                    msg->mNumItems = i;
                    continue;
                    // The loop will terminate subsequently.
                } else {
                    item->u.stringValue = new std::string(stringValue);
                }
                break;
            }

            case kTypeMessage:
            {
                if (maxNestingLevel == 0) {
                    MLOGE("Too many levels of AmlMpMessage nesting.");
                    return NULL;
                }
                sptr<AmlMpMessage> subMsg = AmlMpMessage::FromParcel(
                        parcel,
                        maxNestingLevel - 1);
                if (subMsg == NULL) {
                    // This condition will be triggered when there exists an
                    // object that cannot cross process boundaries or when the
                    // level of nested AmlMpMessage is too deep.
                    return NULL;
                }
                subMsg->incStrong(msg.get());

                item->u.refValue = subMsg.get();
                break;
            }

            default:
            {
                MLOGE("This type of object cannot cross process boundaries.");
                return NULL;
            }
        }

        item->setName(name, strlen(name));
    }

    return msg;
}

void AmlMpMessage::writeToParcel(Parcel *parcel) const {
    parcel->writeInt32(static_cast<int32_t>(mWhat));
    parcel->writeInt32(static_cast<int32_t>(mNumItems));

    for (size_t i = 0; i < mNumItems; ++i) {
        const Item &item = mItems[i];

        parcel->writeCString(item.mName);
        parcel->writeInt32(static_cast<int32_t>(item.mType));

        switch (item.mType) {
            case kTypeInt32:
            {
                parcel->writeInt32(item.u.int32Value);
                break;
            }

            case kTypeInt64:
            {
                parcel->writeInt64(item.u.int64Value);
                break;
            }

            case kTypeSize:
            {
                parcel->writeInt32(static_cast<int32_t>(item.u.sizeValue));
                break;
            }

            case kTypeFloat:
            {
                parcel->writeFloat(item.u.floatValue);
                break;
            }

            case kTypeDouble:
            {
                parcel->writeDouble(item.u.doubleValue);
                break;
            }

            case kTypeString:
            {
                parcel->writeCString(item.u.stringValue->c_str());
                break;
            }

            case kTypeMessage:
            {
                static_cast<AmlMpMessage *>(item.u.refValue)->writeToParcel(parcel);
                break;
            }

            default:
            {
                MLOGE("This type of object cannot cross process boundaries.");
                TRESPASS();
            }
        }
    }
}
#endif

sptr<AmlMpMessage> AmlMpMessage::changesFrom(const sptr<const AmlMpMessage> &other, bool deep) const {
    if (other == NULL) {
        return const_cast<AmlMpMessage*>(this);
    }

    sptr<AmlMpMessage> diff = new AmlMpMessage;
    if (mWhat != other->mWhat) {
        diff->setWhat(mWhat);
    }
    if (mHandler != other->mHandler) {
        diff->setTarget(mHandler.promote());
    }

    for (size_t i = 0; i < mNumItems; ++i) {
        const Item &item = mItems[i];
        const Item *oitem = other->findItem(item.mName, item.mType);
        switch (item.mType) {
            case kTypeInt32:
                if (oitem == NULL || item.u.int32Value != oitem->u.int32Value) {
                    diff->setInt32(item.mName, item.u.int32Value);
                }
                break;

            case kTypeInt64:
                if (oitem == NULL || item.u.int64Value != oitem->u.int64Value) {
                    diff->setInt64(item.mName, item.u.int64Value);
                }
                break;

            case kTypeSize:
                if (oitem == NULL || item.u.sizeValue != oitem->u.sizeValue) {
                    diff->setSize(item.mName, item.u.sizeValue);
                }
                break;

            case kTypeFloat:
                if (oitem == NULL || item.u.floatValue != oitem->u.floatValue) {
                    diff->setFloat(item.mName, item.u.sizeValue);
                }
                break;

            case kTypeDouble:
                if (oitem == NULL || item.u.doubleValue != oitem->u.doubleValue) {
                    diff->setDouble(item.mName, item.u.sizeValue);
                }
                break;

            case kTypeString:
                if (oitem == NULL || *item.u.stringValue != *oitem->u.stringValue) {
                    diff->setString(item.mName, *item.u.stringValue);
                }
                break;

            case kTypeRect:
                if (oitem == NULL || memcmp(&item.u.rectValue, &oitem->u.rectValue, sizeof(Rect))) {
                    diff->setRect(
                            item.mName, item.u.rectValue.mLeft, item.u.rectValue.mTop,
                            item.u.rectValue.mRight, item.u.rectValue.mBottom);
                }
                break;

            case kTypePointer:
                if (oitem == NULL || item.u.ptrValue != oitem->u.ptrValue) {
                    diff->setPointer(item.mName, item.u.ptrValue);
                }
                break;

            case kTypeBuffer:
            {
                sptr<AmlMpBuffer> myBuf = static_cast<AmlMpBuffer *>(item.u.refValue);
                if (myBuf == NULL) {
                    if (oitem == NULL || oitem->u.refValue != NULL) {
                        diff->setBuffer(item.mName, NULL);
                    }
                    break;
                }
                sptr<AmlMpBuffer> oBuf = oitem == NULL ? NULL : static_cast<AmlMpBuffer *>(oitem->u.refValue);
                if (oBuf == NULL
                        || myBuf->size() != oBuf->size()
                        || (!myBuf->data() ^ !oBuf->data()) // data nullness differs
                        || (myBuf->data() && memcmp(myBuf->data(), oBuf->data(), myBuf->size()))) {
                    diff->setBuffer(item.mName, myBuf);
                }
                break;
            }

            case kTypeMessage:
            {
                sptr<AmlMpMessage> myMsg = static_cast<AmlMpMessage *>(item.u.refValue);
                if (myMsg == NULL) {
                    if (oitem == NULL || oitem->u.refValue != NULL) {
                        diff->setMessage(item.mName, NULL);
                    }
                    break;
                }
                sptr<AmlMpMessage> oMsg =
                    oitem == NULL ? NULL : static_cast<AmlMpMessage *>(oitem->u.refValue);
                sptr<AmlMpMessage> changes = myMsg->changesFrom(oMsg, deep);
                if (changes->countEntries()) {
                    diff->setMessage(item.mName, deep ? changes : myMsg);
                }
                break;
            }

            case kTypeObject:
                if (oitem == NULL || item.u.refValue != oitem->u.refValue) {
                    diff->setObject(item.mName, item.u.refValue);
                }
                break;

            default:
            {
                MLOGE("Unknown type %d", item.mType);
                TRESPASS();
            }
        }
    }
    return diff;
}

size_t AmlMpMessage::countEntries() const {
    return mNumItems;
}

const char *AmlMpMessage::getEntryNameAt(size_t index, Type *type) const {
    if (index >= mNumItems) {
        *type = kTypeInt32;

        return NULL;
    }

    *type = mItems[index].mType;

    return mItems[index].mName;
}

AmlMpMessage::ItemData AmlMpMessage::getEntryAt(size_t index) const {
    ItemData it;
    if (index < mNumItems) {
        switch (mItems[index].mType) {
            case kTypeInt32:    it.set(mItems[index].u.int32Value); break;
            case kTypeInt64:    it.set(mItems[index].u.int64Value); break;
            case kTypeSize:     it.set(mItems[index].u.sizeValue); break;
            case kTypeFloat:    it.set(mItems[index].u.floatValue); break;
            case kTypeDouble:   it.set(mItems[index].u.doubleValue); break;
            case kTypePointer:  it.set(mItems[index].u.ptrValue); break;
            case kTypeRect:     it.set(mItems[index].u.rectValue); break;
            case kTypeString:   it.set(*mItems[index].u.stringValue); break;
            case kTypeObject: {
                sptr<AmlMpRefBase> obj = mItems[index].u.refValue;
                it.set(obj);
                break;
            }
            case kTypeMessage: {
                sptr<AmlMpMessage> msg = static_cast<AmlMpMessage *>(mItems[index].u.refValue);
                it.set(msg);
                break;
            }
            case kTypeBuffer: {
                sptr<AmlMpBuffer> buf = static_cast<AmlMpBuffer *>(mItems[index].u.refValue);
                it.set(buf);
                break;
            }
            default:
                break;
        }
    }
    return it;
}

int AmlMpMessage::setEntryNameAt(size_t index, const char *name) {
    if (index >= mNumItems) {
        return -EOVERFLOW;
    }
    if (name == nullptr) {
        return -EOVERFLOW;
    }
    if (!strcmp(name, mItems[index].mName)) {
        return 0; // name has not changed
    }
    size_t len = strlen(name);
    if (findItemIndex(name, len) < mNumItems) {
        return -EEXIST;
    }
    delete[] mItems[index].mName;
    mItems[index].mName = nullptr;
    mItems[index].setName(name, len);
    return 0;
}

int AmlMpMessage::setEntryAt(size_t index, const ItemData &item) {
    std::string stringValue;
    sptr<AmlMpRefBase> refValue;
    sptr<AmlMpMessage> msgValue;
    sptr<AmlMpBuffer> bufValue;

    if (index >= mNumItems) {
        return -EOVERFLOW;
    }
    if (!item.used()) {
        return -EINVAL;
    }
    Item *dst = &mItems[index];
    freeItemValue(dst);

    // some values can be directly set with the getter. others need items to be allocated
    if (item.find(&dst->u.int32Value)) {
        dst->mType = kTypeInt32;
    } else if (item.find(&dst->u.int64Value)) {
        dst->mType = kTypeInt64;
    } else if (item.find(&dst->u.sizeValue)) {
        dst->mType = kTypeSize;
    } else if (item.find(&dst->u.floatValue)) {
        dst->mType = kTypeFloat;
    } else if (item.find(&dst->u.doubleValue)) {
        dst->mType = kTypeDouble;
    } else if (item.find(&dst->u.ptrValue)) {
        dst->mType = kTypePointer;
    } else if (item.find(&dst->u.rectValue)) {
        dst->mType = kTypeRect;
    } else if (item.find(&stringValue)) {
        dst->u.stringValue = new std::string(stringValue);
        dst->mType = kTypeString;
    } else if (item.find(&refValue)) {
        if (refValue != NULL) { refValue->incStrong(this); }
        dst->u.refValue = refValue.get();
        dst->mType = kTypeObject;
    } else if (item.find(&msgValue)) {
        if (msgValue != NULL) { msgValue->incStrong(this); }
        dst->u.refValue = msgValue.get();
        dst->mType = kTypeMessage;
    } else if (item.find(&bufValue)) {
        if (bufValue != NULL) { bufValue->incStrong(this); }
        dst->u.refValue = bufValue.get();
        dst->mType = kTypeBuffer;
    } else {
        // unsupported item - we should not be here.
        dst->mType = kTypeInt32;
        dst->u.int32Value = 0xDEADDEAD;
        return -EINVAL;
    }
    return 0;
}

int AmlMpMessage::removeEntryAt(size_t index) {
    if (index >= mNumItems) {
        return -EOVERFLOW;
    }
    // delete entry data and objects
    --mNumItems;
    delete[] mItems[index].mName;
    mItems[index].mName = nullptr;
    freeItemValue(&mItems[index]);

    // swap entry with last entry and clear last entry's data
    if (index < mNumItems) {
        mItems[index] = mItems[mNumItems];
        mItems[mNumItems].mName = nullptr;
        mItems[mNumItems].mType = kTypeInt32;
    }
    return 0;
}

void AmlMpMessage::setItem(const char *name, const ItemData &item) {
    if (item.used()) {
        Item *it = allocateItem(name);
        if (it != nullptr) {
            setEntryAt(it - mItems, item);
        }
    }
}

AmlMpMessage::ItemData AmlMpMessage::findItem(const char *name) const {
    return getEntryAt(findEntryByName(name));
}

void AmlMpMessage::extend(const sptr<AmlMpMessage> &other) {
    // ignore null messages
    if (other == nullptr) {
        return;
    }

    for (size_t ix = 0; ix < other->mNumItems; ++ix) {
        Item *it = allocateItem(other->mItems[ix].mName);
        if (it != nullptr) {
            ItemData data = other->getEntryAt(ix);
            setEntryAt(it - mItems, data);
        }
    }
}

size_t AmlMpMessage::findEntryByName(const char *name) const {
    return name == nullptr ? countEntries() : findItemIndex(name, strlen(name));
}

}  // namespace android
