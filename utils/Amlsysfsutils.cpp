/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



#define LOG_TAG "AmlMpSysfsUtil"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <utils/AmlMpLog.h>
#include <sys/ioctl.h>
#include "Amlsysfsutils.h"
#include <unistd.h>
#include <cutils/properties.h>
#include <string.h>

static const char* mName = LOG_TAG;

namespace aml_mp {


#ifndef LOGD
#define LOGV MLOGV
#define LOGD MLOGD
#define LOGI MLOGI
#define LOGW MLOGW
#define LOGE MLOGE
#endif

//#define USE_SYSWRITE
#ifndef ANDROID_PLATFORM_SDK_VERSION
#define ANDROID_PLATFORM_SDK_VERSION 28
#endif

#ifndef USE_SYSWRITE
int amsysfs_set_sysfs_str(const char *path, const char *val) {
    int fd;
    int bytes;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        bytes = write(fd, val, strlen(val));
        close(fd);
        return 0;
    } else {
#if (ANDROID_PLATFORM_SDK_VERSION >= 21) && (ANDROID_PLATFORM_SDK_VERSION <= 25)
        return amSystemWriteWriteSysfs(path, (char *) val);
#else
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
        return -1;
#endif

    }
}
int amsysfs_get_sysfs_str(const char *path, char *valstr, unsigned size) {
    int fd;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(valstr, 0, size);
        read(fd, valstr, size - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
        return 0;
    } else {
#if (ANDROID_PLATFORM_SDK_VERSION >= 21) && (ANDROID_PLATFORM_SDK_VERSION <= 25)
        if (amSystemWriteReadNumSysfs(path, valstr, size) != -1) {
            return 0;
        }
#endif
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
        sprintf(valstr, "%s", "fail");
        return -1;
    }
}

int amsysfs_set_sysfs_int(const char *path, int val) {
    int fd;
    int bytes;
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
        sprintf(bcmd, "%d", val);
#if (ANDROID_PLATFORM_SDK_VERSION >= 21) && (ANDROID_PLATFORM_SDK_VERSION <= 25)
        return amSystemWriteWriteSysfs(path, bcmd);
#else
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
        return -1;
#endif

    }
}

int amsysfs_get_sysfs_int(const char *path) {
    int fd;
    int val = 0;
    char bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 10);
        close(fd);
    } else {
#if (ANDROID_PLATFORM_SDK_VERSION >= 21) && (ANDROID_PLATFORM_SDK_VERSION <= 25)
        if (amSystemWriteReadSysfs(path, bcmd) == 0) {
            val = strtol(bcmd, NULL, 10);
        }
#else
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
#endif
    }
    return val;
}

int amsysfs_set_sysfs_int16(const char *path, int val) {
    int fd;
    int bytes;
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "0x%x", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
#if (ANDROID_PLATFORM_SDK_VERSION >= 21) && (ANDROID_PLATFORM_SDK_VERSION <= 25)
        sprintf(bcmd, "0x%x", val);
        return amSystemWriteWriteSysfs(path, bcmd);
#else
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
        return -1;
#endif
    }
}

int amsysfs_get_sysfs_int16(const char *path) {
    int fd;
    int val = 0;
    char bcmd[16];
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 16);
        close(fd);
    } else {
#if (ANDROID_PLATFORM_SDK_VERSION >= 21) && (ANDROID_PLATFORM_SDK_VERSION <= 25)
        if (amSystemWriteReadSysfs(path, bcmd) == 0) {
            val = strtol(bcmd, NULL, 16);
        }
#else
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
#endif

    }
    return val;
}

unsigned long amsysfs_get_sysfs_ulong(const char *path) {
    int fd;
    char bcmd[24] = "";
    unsigned long num = 0;
    if ((fd = open(path, O_RDONLY)) >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        num = strtoul(bcmd, NULL, 0);
        close(fd);
    } else {
#if (ANDROID_PLATFORM_SDK_VERSION >= 21) && (ANDROID_PLATFORM_SDK_VERSION <= 25)
        if (amSystemWriteReadSysfs(path, bcmd) == 0) {
            num = strtoul(bcmd, NULL, 0);
        }
#else
        MLOGW("[%s] %s failed!",__FUNCTION__,path);
#endif

    }
    return num;
}
#else
int amsysfs_set_sysfs_str(const char *path, const char *val)
{
    return amSystemWriteWriteSysfs(path, (char *)val);
}

int amsysfs_get_sysfs_str(const char *path, char *valstr, unsigned size)
{
    if (amSystemWriteReadNumSysfs(path, valstr, size) != -1) {
        return 0;
    }
    sprintf(valstr, "%s", "fail");
    return -1;
}

int amsysfs_set_sysfs_int(const char *path, int val)
{
    char bcmd[16] = "";
    sprintf(bcmd,"%d",val);
    return amSystemWriteWriteSysfs(path, bcmd);
}

int amsysfs_get_sysfs_int(const char *path)
{
    char bcmd[16]= "";
    int val = 0;
    if (amSystemWriteReadSysfs(path, bcmd) == 0) {
        val = strtol(bcmd, NULL, 10);
    }
    return val;
}

int amsysfs_set_sysfs_int16(const char *path, int val)
{
    char bcmd[16]= "";
    sprintf(bcmd, "0x%x", val);
    return amSystemWriteWriteSysfs(path, bcmd);
}

int amsysfs_get_sysfs_int16(const char *path)
{
    char bcmd[16]= "";
    int val = 0;
    if (amSystemWriteReadSysfs(path, bcmd) == 0) {
        val = strtol(bcmd, NULL, 16);
    }
    return val;
}

unsigned long amsysfs_get_sysfs_ulong(const char *path)
{
    char bcmd[24]= "";
    int val = 0;
    if (amSystemWriteReadSysfs(path, bcmd) == 0) {
        val = strtoul(bcmd, NULL, 0);
    }
    return val;
}

#endif

}

