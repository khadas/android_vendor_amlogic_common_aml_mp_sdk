/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_LOG_H_
#define _AML_MP_LOG_H_

#include <utils/Log.h>

#define AML_MP_LOG_TAG  "Aml_MP"

#if LOG_NDEBUG
#define MLOGV(fmt, ...)
#else
#define MLOGV(fmt, ...) ALOG(LOG_VERBOSE, AML_MP_LOG_TAG, "%s " fmt, mName, ##__VA_ARGS__)
#endif

#define MLOGD(fmt, ...) ALOG(LOG_DEBUG, AML_MP_LOG_TAG, "%s " fmt, mName, ##__VA_ARGS__)
#define MLOGI(fmt, ...) ALOG(LOG_INFO, AML_MP_LOG_TAG, "%s " fmt, mName, ##__VA_ARGS__)
#define MLOGW(fmt, ...) ALOG(LOG_WARN, AML_MP_LOG_TAG, "%s " fmt, mName, ##__VA_ARGS__)
#define MLOGE(fmt, ...) ALOG(LOG_ERROR, AML_MP_LOG_TAG, "%s " fmt, mName, ##__VA_ARGS__)

#ifndef MLOG
#define MLOG(fmt, ...) MLOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef KEEP_ALOGX

#ifdef ALOGV
#undef ALOGV
#define ALOGV MLOGV
#endif

#ifdef ALOGD
#undef ALOGD
#define ALOGD MLOGD
#endif

#ifdef ALOGI
#undef ALOGI
#define ALOGI MLOGI
#endif

#ifdef ALOGI_IF
#undef ALOGI_IF
#define ALOGI_IF(cond, fmt, ...) \
({do { \
    if ((cond)) \
        (void)MLOGI(fmt, ##__VA_ARGS__); \
} while(0);})
#endif


#ifdef ALOGW
#undef ALOGW
#define ALOGW MLOGW
#endif


#ifdef ALOGE
#undef ALOGE
#define ALOGE MLOGE
#endif

#endif //KEEP_ALOGX
#endif

