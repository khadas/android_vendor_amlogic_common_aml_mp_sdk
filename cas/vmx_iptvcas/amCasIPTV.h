/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef AMCAS_IPTV_H
#define AMCAS_IPTV_H

#include <stdint.h>

typedef struct iptvseverinfo {
    char    *storepath;
    char    *serveraddr;
    char    *serverport;
    int     enablelog;
} iptvseverinfo_t;

typedef int AmCasCode_t;

class  AmCasIPTV {

public:
    AmCasIPTV() {}
    ~AmCasIPTV() {}
    AmCasCode_t setPrivateData(void *iDate, int iSize) {return 0;}
    AmCasCode_t provision() {return 0;}
    AmCasCode_t openSession(uint8_t *sessionId) {return 0;}
    AmCasCode_t closeSession(uint8_t *sessionId) {return 0;}
    AmCasCode_t setPids(int vPid, int aPid) {return 0;}
    AmCasCode_t processEcm(int isSection, int iPid, uint8_t*pBuffer, int iBufferLength) {return 0;}
    AmCasCode_t processEmm(int isSection, int iPid, uint8_t *pBuffer, int iBufferLength) {return 0;}
    //CasStreamInfo mCasStreamInfo;

private:
    unsigned int    mCaSystemId;
    unsigned int    *mEcmPid;
    unsigned int    *mEmmPid;
    char            *mLaUrl; //cas server url
    char            *mPort;//cas server port
    char            *mLicStore;//cas license store
    int             mvPid;
    int             maPid;
};

#endif
