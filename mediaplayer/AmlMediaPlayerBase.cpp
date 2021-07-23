/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMediaPlayerBase"
#include "utils/AmlMpLog.h"
#include <utils/AmlMpUtils.h>
#include "AmlMediaPlayerBase.h"
#include "Aml_MP_MediaPlayerImpl.h"
#include <dlfcn.h>

namespace aml_mp {
///////////////////////////////////////////////////////
AmlMediaPlayer_Ops::AmlMediaPlayer_Ops() {
    //initAmlMediaPlayerLib();

    MLOGI("ctor AmlMediaPlayer_Ops\n");
};

AmlMediaPlayer_Ops::~AmlMediaPlayer_Ops() {
    releaseAmlMediaPlayerLib();

    MLOGI("~AmlMediaPlayer_Ops leave\n");
};

int AmlMediaPlayer_Ops::initAmlMediaPlayerLib(Aml_MP_MediaPlayerCreateParams* createParams)
{
    int err = -1;

    if (isInit) {
        MLOGI("AmlMediaPlayer_Ops has inited\n");
        return 0;
    }

    if (libHandle == NULL) {
        libHandle = dlopen("libdrmp.so", RTLD_NOW);
        if (libHandle == NULL) {
            //libHandle = dlopen("libdrmp.so", RTLD_NOW);//AmUMedaplayer
            if (libHandle == NULL) {
                    MLOGE("unable to dlopen libdrmp.so: %s", dlerror());
                    return err;
            }
        }
    }

    typedef AmlMediaPlayerBase *(*create)(Aml_MP_MediaPlayerCreateParams* createParams, int instanceId);
    createAmlMediaPlayerFunc =
        (create)dlsym(libHandle, "createAmlMediaplayer");
    if (createAmlMediaPlayerFunc == NULL) {
        MLOGE(" dlsym createAmlMediaPlayerFunc failed, err=%s \n", dlerror());
        return err;
    }

    MLOGI("initAmlMediaPlayerLib ok\n");

    isInit = true;
    err = 0;
    return err;
}

int AmlMediaPlayer_Ops::releaseAmlMediaPlayerLib()
{
    if (libHandle) {
        dlclose(libHandle);
        libHandle = NULL;
    }

    MLOGI("AmlMediaPlayerLibRelease\n");

    return 0;
}

static struct AmlMediaPlayer_Ops g_AmlMediaPlayer_ops;

///////////////////////////////////////////////////////
AmlMediaPlayerBase* AmlMediaPlayerBase::create(Aml_MP_MediaPlayerCreateParams* createParams, int instanceId)
{
    const char* mName = LOG_TAG;

    AmlMediaPlayerBase *player = nullptr;
    if (0 != g_AmlMediaPlayer_ops.initAmlMediaPlayerLib(createParams)) {
        return player;
    }

    player = (*g_AmlMediaPlayer_ops.createAmlMediaPlayerFunc)(createParams, instanceId);
    MLOGI("create player ok, %p, id:%d", player, instanceId);

    return player;
}

AmlMediaPlayerBase::AmlMediaPlayerBase(Aml_MP_MediaPlayerCreateParams* createParams, int instanceId)
{
    ;
}

AmlMediaPlayerBase::~AmlMediaPlayerBase()
{
    ;
}

int AmlMediaPlayerBase::registerEventCallBack(Aml_MP_MediaPlayerEventCallback cb, void* userData)
{
    mEventCb = cb;
    mUserData = userData;

    return 0;
}

int AmlMediaPlayerBase::setSubtitleWindow(int x, int y, int width, int height)
{
    MLOGI("setSubtitleWindow");
    MLOGI("param x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    MLOGI("TODO!!!");

    return -1;
}

int AmlMediaPlayerBase::showSubtitle()
{
    MLOGI("showSubtitle");
    MLOGI("TODO!!!");

    return -1;
}

int AmlMediaPlayerBase::hideSubtitle()
{
    MLOGI("showSubtitle");
    MLOGI("TODO!!!");

    return -1;
}

///////////////////////////////////////////////////////////////////////////////
void AmlMediaPlayerBase::notifyListener(Aml_MP_MediaPlayerEventType eventType, int64_t param)
{
    if (mEventCb) {
        mEventCb(mUserData, eventType, param);
    } else {
        MLOGE("mEventCb is NULL! %s", mpPlayerEventType2Str(eventType));
    }
}
///////////////////////////////////////////////////////////////////////////////
}

