/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo"
#include <utils/Log.h>
#include <AmlMpTestSupporter.h>
#include <unistd.h>
#include <getopt.h>
#include "TestUtils.h"

using namespace aml_mp;

struct Argument
{
    std::string url;
    int x = 0;
    int y = 0;
    int viewWidth = -1;
    int viewHeight = -1;
    int playMode = 0;
    int videoMode = 0;
    int zorder = 0;
    int record = 0;
    int crypto = 0;
};

static int parseCommandArgs(int argc, char* argv[], Argument* argument)
{
    static const struct option longopts[] = {
        {"help",        no_argument,        nullptr, 'h'},
        {"size",        required_argument,  nullptr, 's'},
        {"pos",         required_argument,  nullptr, 'p'},
        {"playmode",    required_argument,  nullptr, 'm'},
        {"zorder",      required_argument,  nullptr, 'z'},
        {"videomode",   required_argument,  nullptr, 'v'},
        {"record",      no_argument,        nullptr, 'r'},
        {"crypto",      no_argument,        nullptr, 'c'},
        {nullptr,       no_argument,        nullptr, 0},
    };

    int opt, longindex;
    while ((opt = getopt_long(argc, argv, "", longopts, &longindex)) != -1) {
        //printf("opt = %d\n", opt);
        switch (opt) {
        case 's':
            {
                int width, height;
                if (sscanf(optarg, "%dx%d", &width, &height) != 2) {
                    printf("parse %s failed! %s\n", longopts[longindex].name, optarg);
                } else {
                    argument->viewWidth = width;
                    argument->viewHeight = height;
                }
            }
            break;

        case 'p':
            {
                int x, y;
                //printf("optarg:%s\n", optarg);
                if (sscanf(optarg, "%dx%d", &x, &y) != 2) {
                    printf("parse %s failed! %s\n", longopts[longindex].name, optarg);
                } else {
                    argument->x = x;
                    argument->y = y;
                    printf("pos:%dx%d\n", x, y);
                }
            }
            break;

        case 'm':
            {
                int playMode = strtol(optarg, nullptr, 0);
                if (playMode < 0 || playMode >= AmlMpTestSupporter::PLAY_MODE_MAX) {
                    playMode = 0;
                }
                printf("playMode:%d\n", playMode);
                argument->playMode = playMode;
            }
            break;

        case 'z':
        {
            int zorder = strtol(optarg, nullptr, 0);
            printf("zorder:%d\n", zorder);
            argument->zorder = zorder;
        }
        break;

        case 'v':
        {
            int videoMode = strtol(optarg, nullptr, 0);
            printf("videoMode:%d\n", videoMode);
            argument->videoMode = videoMode;
        }
        break;

        case 'r':
        {
            printf("record mode!\n");
            argument->record = true;
        }
        break;

        case 'c':
        {
            printf("crypto mode!\n");
            argument->crypto = true;
        }
        break;

        case 'h':
        default:
            return -1;
        }
    }

    //printf("optind:%d, argc:%d\n", optind, argc);
    if (optind < argc) {
        argument->url = argv[argc-1];
        printf("url:%s\n", argument->url.c_str());
    }

    return 0;
}

static void showUsage()
{
    printf("Usage: amlMpPlayerDemo <options> <url>\n"
            "options:\n"
            "    --size:      eg: 1920x1080\n"
            "    --pos:       eg: 0x0\n"
            "    --zorder:    eg: 0\n"
            "    --videomode: 0: set ANativeWindow\n"
            "                 1: set video window\n"
            "    --playmode:  0: set audio param, set video param, start --> stop\n"
            "                 1: set audio param, set video param, start --> stop audio, stop video\n"
            "                 2: set audio param, set video param, start audio, start video --> stop\n"
            "                 3: set audio param, set video param, start audio, start video --> stop audio, stop video\n"
            "                 4: set audio param, set video param, start video, start audio --> stop audio, stop video\n"
            "                 5: set audio param, start audio, set video param, start video --> stop audio, stop video\n"
            "                 6: set video param, start video, set audio param, start audio --> stop video, stop audio\n"
            "   --record:     record mode, record file name is \"" AML_MP_TEST_SUPPORTER_RECORD_FILE "\"\n"
            "   --crypto      crypto mode\n"
            "\n"
            "url format: url?program=xx&demuxid=xx\n"
            "    DVB-T dvbt://<freq>, eg: dvbt://474\n"
            "    local file, eg: /data/a.ts\n"
            "    dvr replay, eg: dvr:/" AML_MP_TEST_SUPPORTER_RECORD_FILE "\n"
            "    udp source, eg: udp://224.0.0.1:8000\n"
            "\n"
            );
}


int main(int argc, char *argv[])
{
    Argument argument{};

    if (parseCommandArgs(argc, argv, &argument) < 0 || argument.url.empty()) {
        showUsage();
        return 0;
    }

    sptr<AmlMpTestSupporter> mpTestSupporter = new AmlMpTestSupporter;
    mpTestSupporter->installSignalHandler();

    mpTestSupporter->setDataSource(argument.url);
    int ret = mpTestSupporter->prepare(argument.crypto);
    if (ret < 0) {
        printf("prepare failed!\n");
        return -1;
    }

    if (!argument.record) {
        AmlMpTestSupporter::DisplayParam displayParam;
        displayParam.x = argument.x;
        displayParam.y = argument.y;
        displayParam.width = argument.viewWidth;
        displayParam.height = argument.viewHeight;
        displayParam.zorder = argument.zorder;
        displayParam.videoMode = argument.videoMode;
        mpTestSupporter->setDisplayParam(displayParam);

        mpTestSupporter->startPlay((AmlMpTestSupporter::PlayMode)argument.playMode);

    } else {
        mpTestSupporter->startRecord();
    }

    mpTestSupporter->fetchAndProcessCommands();
    mpTestSupporter->stop();
    mpTestSupporter.clear();

    ALOGI("exited!");
    return 0;
}
