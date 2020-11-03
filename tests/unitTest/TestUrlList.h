/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_TEST_URL_LIST_H_
#define _AML_MP_TEST_URL_LIST_H_

#include <list>
#include <set>
#include <string>

namespace Json {
 class Value;
};

namespace aml_mp {
class TestUrlList
{
public:
    static TestUrlList& instance() {
        static TestUrlList urlList;
        return urlList;
    }

    ~TestUrlList() = default;
    bool getUrls(const std::string& testName, std::list<std::string>* results);
    int genConfig(const std::string& source);
    int loadConfig(const std::string& sourceDir);

private:
    struct UrlInfo {
        std::string url;
        std::set<std::string> testNames;
    };

    TestUrlList();
    void initDefaultConfig();
    void initSourceDir(const std::string& source);
    void initUrlInfo(const Json::Value& root);


    std::list<UrlInfo> mUrlInfos;
    std::string mSourceDir;

    TestUrlList(const TestUrlList&) = delete;
    TestUrlList& operator=(const TestUrlList&) = delete;
};


}








#endif
