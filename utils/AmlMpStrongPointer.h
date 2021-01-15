/*
 * Copyright (C) 2005 The Android Open Source Project
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

#ifndef AML_MP_ANDROID_STRONG_POINTER_H
#define AML_MP_ANDROID_STRONG_POINTER_H

#include <functional>
#include <type_traits>  // for common_type.

// ---------------------------------------------------------------------------
namespace aml_mp {

template<typename T> class wptr;

// ---------------------------------------------------------------------------

// TODO: Maybe remove sp<> ? wp<> comparison? These are dangerous: If the wp<>
// was created before the sp<>, and they point to different objects, they may
// compare equal even if they are entirely unrelated. E.g. CameraService
// currently performa such comparisons.

#define AML_MP_COMPARE_STRONG(_op_)                                           \
template<typename U>                                            \
inline bool operator _op_ (const sptr<U>& o) const {              \
    return m_ptr _op_ o.m_ptr;                                  \
}                                                               \
template<typename U>                                            \
inline bool operator _op_ (const U* o) const {                  \
    return m_ptr _op_ o;                                        \
}                                                               \
/* Needed to handle type inference for nullptr: */              \
inline bool operator _op_ (const T* o) const {                  \
    return m_ptr _op_ o;                                        \
}

template<template<typename C> class comparator, typename T, typename U>
static inline bool _sptr_compare_(T* a, U* b) {
    return comparator<typename std::common_type<T*, U*>::type>()(a, b);
}

// Use std::less and friends to avoid undefined behavior when ordering pointers
// to different objects.
#define AML_MP_COMPARE_STRONG_FUNCTIONAL(_op_, _compare_)               \
template<typename U>                                             \
inline bool operator _op_ (const sptr<U>& o) const {               \
    return _sptr_compare_<_compare_>(m_ptr, o.m_ptr);              \
}                                                                \
template<typename U>                                             \
inline bool operator _op_ (const U* o) const {                   \
    return _sptr_compare_<_compare_>(m_ptr, o);                    \
}
// ---------------------------------------------------------------------------

template<typename T>
class sptr {
public:
    inline sptr() : m_ptr(nullptr) { }

    sptr(T* other);  // NOLINT(implicit)
    sptr(const sptr<T>& other);
    sptr(sptr<T>&& other) noexcept;
    template<typename U> sptr(U* other);  // NOLINT(implicit)
    template<typename U> sptr(const sptr<U>& other);  // NOLINT(implicit)
    template<typename U> sptr(sptr<U>&& other);  // NOLINT(implicit)

    ~sptr();

    // Assignment

    sptr& operator = (T* other);
    sptr& operator = (const sptr<T>& other);
    sptr& operator=(sptr<T>&& other) noexcept;

    template<typename U> sptr& operator = (const sptr<U>& other);
    template<typename U> sptr& operator = (sptr<U>&& other);
    template<typename U> sptr& operator = (U* other);

    //! sptrecial optimization for use by ProcessState (and nobody else).
    void force_set(T* other);

    // Reset

    void clear();

    // Accessors

    inline T&       operator* () const     { return *m_ptr; }
    inline T*       operator-> () const    { return m_ptr;  }
    inline T*       get() const            { return m_ptr; }
    inline explicit operator bool () const { return m_ptr != nullptr; }

    // Operators

    AML_MP_COMPARE_STRONG(==)
    AML_MP_COMPARE_STRONG(!=)
    AML_MP_COMPARE_STRONG_FUNCTIONAL(>, std::greater)
    AML_MP_COMPARE_STRONG_FUNCTIONAL(<, std::less)
    AML_MP_COMPARE_STRONG_FUNCTIONAL(<=, std::less_equal)
    AML_MP_COMPARE_STRONG_FUNCTIONAL(>=, std::greater_equal)

    // Punt these to the wp<> implementation.
    template<typename U>
    inline bool operator == (const wptr<U>& o) const {
        return o == *this;
    }

    template<typename U>
    inline bool operator != (const wptr<U>& o) const {
        return o != *this;
    }

private:
    template<typename Y> friend class sptr;
    template<typename Y> friend class wptr;
    void set_pointer(T* ptr);
    T* m_ptr;
};

// For code size reasons, we do not want this inlined or templated.
void sptr_report_race();

#undef COMPARE

// ---------------------------------------------------------------------------
// No user serviceable parts below here.

template<typename T>
sptr<T>::sptr(T* other)
        : m_ptr(other) {
    if (other)
        other->incStrong(this);
}

template<typename T>
sptr<T>::sptr(const sptr<T>& other)
        : m_ptr(other.m_ptr) {
    if (m_ptr)
        m_ptr->incStrong(this);
}

template <typename T>
sptr<T>::sptr(sptr<T>&& other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
}

template<typename T> template<typename U>
sptr<T>::sptr(U* other)
        : m_ptr(other) {
    if (other)
        (static_cast<T*>(other))->incStrong(this);
}

template<typename T> template<typename U>
sptr<T>::sptr(const sptr<U>& other)
        : m_ptr(other.m_ptr) {
    if (m_ptr)
        m_ptr->incStrong(this);
}

template<typename T> template<typename U>
sptr<T>::sptr(sptr<U>&& other)
        : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
}

template<typename T>
sptr<T>::~sptr() {
    if (m_ptr)
        m_ptr->decStrong(this);
}

template<typename T>
sptr<T>& sptr<T>::operator =(const sptr<T>& other) {
    // Force m_ptr to be read twice, to heuristically check for data races.
    T* oldPtr(*const_cast<T* volatile*>(&m_ptr));
    T* otherPtr(other.m_ptr);
    if (otherPtr) otherPtr->incStrong(this);
    if (oldPtr) oldPtr->decStrong(this);
    if (oldPtr != *const_cast<T* volatile*>(&m_ptr)) sptr_report_race();
    m_ptr = otherPtr;
    return *this;
}

template <typename T>
sptr<T>& sptr<T>::operator=(sptr<T>&& other) noexcept {
    T* oldPtr(*const_cast<T* volatile*>(&m_ptr));
    if (oldPtr) oldPtr->decStrong(this);
    if (oldPtr != *const_cast<T* volatile*>(&m_ptr)) sptr_report_race();
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
    return *this;
}

template<typename T>
sptr<T>& sptr<T>::operator =(T* other) {
    T* oldPtr(*const_cast<T* volatile*>(&m_ptr));
    if (other) other->incStrong(this);
    if (oldPtr) oldPtr->decStrong(this);
    if (oldPtr != *const_cast<T* volatile*>(&m_ptr)) sptr_report_race();
    m_ptr = other;
    return *this;
}

template<typename T> template<typename U>
sptr<T>& sptr<T>::operator =(const sptr<U>& other) {
    T* oldPtr(*const_cast<T* volatile*>(&m_ptr));
    T* otherPtr(other.m_ptr);
    if (otherPtr) otherPtr->incStrong(this);
    if (oldPtr) oldPtr->decStrong(this);
    if (oldPtr != *const_cast<T* volatile*>(&m_ptr)) sptr_report_race();
    m_ptr = otherPtr;
    return *this;
}

template<typename T> template<typename U>
sptr<T>& sptr<T>::operator =(sptr<U>&& other) {
    T* oldPtr(*const_cast<T* volatile*>(&m_ptr));
    if (m_ptr) m_ptr->decStrong(this);
    if (oldPtr != *const_cast<T* volatile*>(&m_ptr)) sptr_report_race();
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
    return *this;
}

template<typename T> template<typename U>
sptr<T>& sptr<T>::operator =(U* other) {
    T* oldPtr(*const_cast<T* volatile*>(&m_ptr));
    if (other) (static_cast<T*>(other))->incStrong(this);
    if (oldPtr) oldPtr->decStrong(this);
    if (oldPtr != *const_cast<T* volatile*>(&m_ptr)) sptr_report_race();
    m_ptr = other;
    return *this;
}

template<typename T>
void sptr<T>::force_set(T* other) {
    other->forceIncStrong(this);
    m_ptr = other;
}

template<typename T>
void sptr<T>::clear() {
    T* oldPtr(*const_cast<T* volatile*>(&m_ptr));
    if (oldPtr) {
        oldPtr->decStrong(this);
        if (oldPtr != *const_cast<T* volatile*>(&m_ptr)) sptr_report_race();
        m_ptr = nullptr;
    }
}

template<typename T>
void sptr<T>::set_pointer(T* ptr) {
    m_ptr = ptr;
}

}  // namespace android

// ---------------------------------------------------------------------------

#endif // ANDROID_STRONG_POINTER_H
