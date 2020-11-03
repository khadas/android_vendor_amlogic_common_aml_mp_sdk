/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "TestUrlList"
#define KEEP_ALOGX
#include <utils/Log.h>
#include "TestUrlList.h"
#include <dirent.h>
#include <json/json.h>
#include <fstream>

#define MLOG(fmt, ...) ALOGI("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

namespace aml_mp {
TestUrlList::TestUrlList()
{
    initDefaultConfig();
}

bool TestUrlList::getUrls(const std::string& testName, std::list<std::string>* results)
{
    if (results == nullptr) return false;

    ALOGI("mUrlInfos size = %d, mSourceDir:%s\n", mUrlInfos.size(), mSourceDir.c_str());
    bool found = false;
    for (auto& p : mUrlInfos) {
        if (p.testNames.empty() || (p.testNames.find(testName) != p.testNames.end())) {
            results->push_back(mSourceDir + p.url);
            found = true;
        }
    }

    return found;
}

int TestUrlList::genConfig(const std::string& sourceDir)
{
    MLOG("sourceDir:%s", sourceDir.c_str());

    if (access(sourceDir.c_str(), F_OK) < 0) {
        printf("cann't access %s\n", sourceDir.c_str());
        return -1;
    }

    struct DIR* dir;
    dir = opendir(sourceDir.c_str());
    if (dir == nullptr) {
        printf("open %s failed!\n", sourceDir.c_str());
    }

    Json::Value root, item;

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string file = ent->d_name;
        if (file == "." || file == "..") {
            continue;
        }

        if (ent->d_type != DT_REG) {
            continue;
        }

        auto suffix = file.find_last_of(".");
        if (suffix == std::string::npos) {
            continue;
        }

        if (file.substr(suffix+1) != "ts") {
            continue;
        }

        //printf("file:%s\n", file.c_str());

        item["url"] = file;
        item["testName"] = "AmlMpPlayerTest";

        root.append(item);
    }

    closedir(dir);


    if (!root.empty()) {
        std::string configFile = sourceDir + "/amlMpUnitTestConfig.json";
        std::ofstream of(configFile);
        if (!of.is_open()) {
            printf("open %s failed!\n", configFile.c_str());
        } else {
            Json::StyledStreamWriter().write(of, root);
        }

        initSourceDir(sourceDir);
        initUrlInfo(root);
    }

    return 0;
}

int TestUrlList::loadConfig(const std::string& sourceDir)
{
    if (sourceDir.empty()) {
        printf("invalid sourceDir!\n");
        return -1;
    }

    std::string configFile = mSourceDir + "/amlMpUnitTestConfig.json";
    std::ifstream cf(configFile);
    if (!cf.is_open()) {
        printf("oepn %s failed!\n", configFile.c_str());
        return -1;
    }

    Json::Value root;
    if (!Json::Reader().parse(cf, root)) {
        printf("parse %s failed!\n", configFile.c_str());
        return -1;
    }

    if (!root.isArray()) {
        printf("config is invalied!\n");
        return -1;
    }

    initSourceDir(sourceDir);
    initUrlInfo(root);

    return 0;
}

void TestUrlList::initSourceDir(const std::string& sourceDir)
{
    mSourceDir = sourceDir;
    if (*(mSourceDir.end()-1) != '/') {
        mSourceDir.append("/");
    }
}

void TestUrlList::initUrlInfo(const Json::Value& root)
{
    mUrlInfos.clear();

    for (auto& p : root) {
        if (!p.isObject() && !p.isString()) {
            continue;
        }

        MLOG();

        UrlInfo info;
        if (p.isObject()) {
            Json::Value url = p.get("url", Json::nullValue);
            if (url.isString()) {
                info.url = url.asString();
                printf("url:%s\n", info.url.c_str());
            }

            Json::Value testNames = p.get("testName", Json::nullValue);
            if (testNames.isArray()) {
                for (auto& t : testNames) {
                    info.testNames.emplace(t.asString());
                }
            } else if (testNames.isString()) {
                info.testNames.emplace(testNames.asString());
            }
        }

        mUrlInfos.push_back(std::move(info));
    }
}

void TestUrlList::initDefaultConfig()
{
    UrlInfo info;
    info.url = "/storage/5E82-06C8/a.ts";

    mUrlInfos.emplace_back(info);
}


}
