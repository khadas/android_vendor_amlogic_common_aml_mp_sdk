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

#ifndef AML_MP_MESSAGE_H_
#define AML_MP_MESSAGE_H_

#include "AmlMpData.h"
#include "AmlMpEventLooper.h"
#include "AmlMpRefBase.h"
#include <vector>

namespace aml_mp {

struct AmlMpBuffer;
struct AmlMpEventHandler;

struct AReplyToken : public AmlMpRefBase {
    explicit AReplyToken(const sptr<AmlMpEventLooper> &looper)
        : mLooper(looper),
          mReplied(false) {
    }

private:
    friend struct AmlMpMessage;
    friend struct AmlMpEventLooper;
    wptr<AmlMpEventLooper> mLooper;
    sptr<AmlMpMessage> mReply;
    bool mReplied;

    sptr<AmlMpEventLooper> getLooper() const {
        return mLooper.promote();
    }
    // if reply is not set, returns false; otherwise, it retrieves the reply and returns true
    bool retrieveReply(sptr<AmlMpMessage> *reply) {
        if (mReplied) {
            *reply = mReply;
            mReply.clear();
        }
        return mReplied;
    }
    // sets the reply for this token. returns OK or error
    int setReply(const sptr<AmlMpMessage> &reply);
};

struct AmlMpMessage : public AmlMpRefBase {
    AmlMpMessage();
    AmlMpMessage(uint32_t what, const sptr<const AmlMpEventHandler> &handler);

    // Construct an AMessage from a parcel.
    // nestingAllowed determines how many levels AMessage can be nested inside
    // AMessage. The default value here is arbitrarily set to 255.
    // FromParcel() returns NULL on error, which occurs when the input parcel
    // contains
    // - an AMessage nested deeper than maxNestingLevel; or
    // - an item whose type is not recognized by this function.
    // Types currently recognized by this function are:
    //   Item types      set/find function suffixes
    //   ==========================================
    //     int32_t                Int32
    //     int64_t                Int64
    //     size_t                 Size
    //     float                  Float
    //     double                 Double
    //     AString                String
    //     AMessage               Message
    //static sptr<AmlMpMessage> FromParcel(const Parcel &parcel,
                                   //size_t maxNestingLevel = 255);

    // Write this AMessage to a parcel.
    // All items in the AMessage must have types that are recognized by
    // FromParcel(); otherwise, TRESPASS error will occur.
    //void writeToParcel(Parcel *parcel) const;

    void setWhat(uint32_t what);
    uint32_t what() const;

    void setTarget(const sptr<const AmlMpEventHandler> &handler);

    void clear();

    void setInt32(const char *name, int32_t value);
    void setInt64(const char *name, int64_t value);
    void setSize(const char *name, size_t value);
    void setFloat(const char *name, float value);
    void setDouble(const char *name, double value);
    void setPointer(const char *name, void *value);
    void setString(const char *name, const char *s, ssize_t len = -1);
    void setString(const char *name, const std::string &s);
    void setObject(const char *name, const sptr<AmlMpRefBase> &obj);
    void setBuffer(const char *name, const sptr<AmlMpBuffer> &buffer);
    void setMessage(const char *name, const sptr<AmlMpMessage> &obj);

    void setRect(
            const char *name,
            int32_t left, int32_t top, int32_t right, int32_t bottom);

    bool contains(const char *name) const;

    bool findInt32(const char *name, int32_t *value) const;
    bool findInt64(const char *name, int64_t *value) const;
    bool findSize(const char *name, size_t *value) const;
    bool findFloat(const char *name, float *value) const;
    bool findDouble(const char *name, double *value) const;
    bool findPointer(const char *name, void **value) const;
    bool findString(const char *name, std::string *value) const;
    bool findObject(const char *name, sptr<AmlMpRefBase> *obj) const;
    bool findBuffer(const char *name, sptr<AmlMpBuffer> *buffer) const;
    bool findMessage(const char *name, sptr<AmlMpMessage> *obj) const;

    // finds signed integer types cast to int64_t
    bool findAsInt64(const char *name, int64_t *value) const;

    // finds any numeric type cast to a float
    bool findAsFloat(const char *name, float *value) const;

    bool findRect(
            const char *name,
            int32_t *left, int32_t *top, int32_t *right, int32_t *bottom) const;

    int post(int64_t delayUs = 0);

    // Posts the message to its target and waits for a response (or error)
    // before returning.
    int postAndAwaitResponse(sptr<AmlMpMessage> *response);

    // If this returns true, the sender of this message is synchronously
    // awaiting a response and the reply token is consumed from the message
    // and stored into replyID. The reply token must be used to send the response
    // using "postReply" below.
    bool senderAwaitsResponse(sptr<AReplyToken> *replyID);

    // Posts the message as a response to a reply token.  A reply token can
    // only be used once. Returns OK if the response could be posted; otherwise,
    // an error.
    int postReply(const sptr<AReplyToken> &replyID);

    // Performs a deep-copy of "this", contained messages are in turn "dup'ed".
    // Warning: RefBase items, i.e. "objects" are _not_ copied but only have
    // their refcount incremented.
    sptr<AmlMpMessage> dup() const;

    // Adds all items from other into this.
    void extend(const sptr<AmlMpMessage> &other);

    // Performs a shallow or deep comparison of |this| and |other| and returns
    // an AMessage with the differences.
    // Warning: RefBase items, i.e. "objects" are _not_ copied but only have
    // their refcount incremented.
    // This is true for AMessages that have no corresponding AMessage equivalent in |other|.
    // (E.g. there is no such key or the type is different.) On the other hand, changes in
    // the AMessage (or AMessages if deep is |false|) are returned in new objects.
    sptr<AmlMpMessage> changesFrom(const sptr<const AmlMpMessage> &other, bool deep = false) const;

    std::string debugString(int32_t indent = 0) const;

    enum Type {
        kTypeInt32,
        kTypeInt64,
        kTypeSize,
        kTypeFloat,
        kTypeDouble,
        kTypePointer,
        kTypeString,
        kTypeObject,
        kTypeMessage,
        kTypeRect,
        kTypeBuffer,
    };

    struct Rect {
        int32_t mLeft, mTop, mRight, mBottom;
    };

    size_t countEntries() const;
    const char *getEntryNameAt(size_t index, Type *type) const;

    /**
     * Retrieves the item at a specific index.
     */
    typedef AData<
        int32_t, int64_t, size_t, float, double, Rect, std::string,
        void *, sptr<AmlMpMessage>, sptr<AmlMpBuffer>, sptr<AmlMpRefBase>>::Basic ItemData;

    /**
     * Finds an item by name. This can be used if the type is unknown.
     *
     * \param name name of the item
     * Returns an empty item if no item is present with that name.
     */
    ItemData findItem(const char *name) const;

    /**
     * Sets an item of arbitrary type. Does nothing if the item value is empty.
     *
     * \param name name of the item
     * \param item value of the item
     */
    void setItem(const char *name, const ItemData &item);

    ItemData getEntryAt(size_t index) const;

    /**
     * Finds an entry by name and returns its index.
     *
     * \retval countEntries() if the entry is not found.
     */
    size_t findEntryByName(const char *name) const;

    /**
     * Sets the name of an entry based on index.
     *
     * \param index index of the entry
     * \param name (new) name of the entry
     *
     * \retval OK the name was set successfully
     * \retval BAD_INDEX invalid index
     * \retval BAD_VALUE name is invalid (null)
     * \retval ALREADY_EXISTS name is already used by another entry
     */
    int setEntryNameAt(size_t index, const char *name);

    /**
     * Sets the item of an entry based on index.
     *
     * \param index index of the entry
     * \param item new item of the entry
     *
     * \retval OK the item was set successfully
     * \retval BAD_INDEX invalid index
     * \retval BAD_VALUE item is invalid (null)
     * \retval BAD_TYPE type is unsupported (should not happen)
     */
    int setEntryAt(size_t index, const ItemData &item);

    /**
     * Removes an entry based on index.
     *
     * \param index index of the entry
     *
     * \retval OK the entry was removed successfully
     * \retval BAD_INDEX invalid index
     */
    int removeEntryAt(size_t index);

protected:
    virtual ~AmlMpMessage();

private:
    friend struct AmlMpEventLooper; // deliver()

    uint32_t mWhat;

    // used only for debugging
    AmlMpEventLooper::handler_id mTarget;

    wptr<AmlMpEventHandler> mHandler;
    wptr<AmlMpEventLooper> mLooper;

    struct Item {
        union {
            int32_t int32Value;
            int64_t int64Value;
            size_t sizeValue;
            float floatValue;
            double doubleValue;
            void *ptrValue;
            AmlMpRefBase *refValue;
            std::string *stringValue;
            Rect rectValue;
        } u;
        const char *mName;
        size_t      mNameLength;
        Type mType;
        void setName(const char *name, size_t len);
    };

    enum {
        kMaxNumItems = 64
    };
    Item mItems[kMaxNumItems];
    size_t mNumItems;

    Item *allocateItem(const char *name);
    void freeItemValue(Item *item);
    const Item *findItem(const char *name, Type type) const;

    void setObjectInternal(
            const char *name, const sptr<AmlMpRefBase> &obj, Type type);

    size_t findItemIndex(const char *name, size_t len) const;

    void deliver();

    AmlMpMessage(const AmlMpMessage&) = delete;
    AmlMpMessage& operator= (const AmlMpMessage&) = delete;
};

}  // namespace android

#endif  // A_MESSAGE_H_
