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

using namespace android;
using namespace aml_mp;

struct Argument
{
    std::string url;
    int x;
    int y;
    int viewWidth;
    int viewHeight;
    int playMode;
};

static int parseCommandArgs(int argc, char* argv[], Argument* argument)
{
    static const struct option longopts[] = {
        {"help",        no_argument,        nullptr, 'h'},
        {"size",        required_argument,  nullptr, 's'},
        {"pos",         required_argument,  nullptr, 'p'},
        {"playmode",    required_argument,  nullptr, 'm'},
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

        case 'h':
        default:
            return -1;
        }
    }

    printf("optind:%d, argc:%d\n", optind, argc);
    if (optind < argc) {
        argument->url = argv[argc-1];
        printf("url:%s\n", argument->url.c_str());
    }

    return 0;
}

static void showUsage()
{
    printf("Usage: AmlMpPlayerDemo <options> <url>\n"
            "options:\n"
            "    size: eg: 1920x1080\n"
            "    pos:  eg: 0x0 \n"
            "    playmode: [0, 4] set all params before start\n"
            "              5: set audio param, start audio decoding; set video param, start video decoding\n"
            "              6: set video param, start video decoding; set audio param, start audio decoding\n"
            "\n"
            "url format:\n"
            "    local filepath?<program number>\n"
            "    udp://ip:port?<program number>\n"
            "    dvbc://demux id>?<program number>\n"
            "    dvbt://demux id>?<program number>\n"
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

    sp<AmlMpTestSupporter> mpPlayer = new AmlMpTestSupporter;
    mpPlayer->installSignalHandler();

    mpPlayer->setDataSource(argument.url);
    int ret = mpPlayer->prepare();
    if (ret < 0) {
        printf("prepare failed!\n");
        return -1;
    }

    mpPlayer->startPlay((AmlMpTestSupporter::PlayMode)argument.playMode);
    mpPlayer->fetchAndProcessCommands();
    mpPlayer->stop();
    mpPlayer.clear();

    ALOGI("exited!");
    return 0;
}

