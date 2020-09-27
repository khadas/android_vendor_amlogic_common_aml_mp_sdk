/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _AML_MP_LOG_H_
#define _AML_MP_LOG_H_

#include <utils/Log.h>

#if LOG_NDEBUG
#define MLOGV(fmt, ...)
#else
#define MLOGV(fmt, ...) ALOG(LOG_VERBOSE, mName, fmt, ##__VA_ARGS__)
#endif

#define MLOGD(fmt, ...) ALOG(LOG_DEBUG, mName, fmt, ##__VA_ARGS__)
#define MLOGI(fmt, ...) ALOG(LOG_INFO, mName, fmt, ##__VA_ARGS__)
#define MLOGW(fmt, ...) ALOG(LOG_WARN, mName, fmt, ##__VA_ARGS__)
#define MLOGE(fmt, ...) ALOG(LOG_ERROR, mName, fmt, ##__VA_ARGS__)

#ifndef MLOG
#define MLOG(fmt, ...) ALOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
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
        (void)ALOG(LOG_INFO, mName, fmt, ##__VA_ARGS__); \
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

