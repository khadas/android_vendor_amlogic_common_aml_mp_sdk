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
#include <map>
#include <string>

namespace aml_mp {
class TestUrlList
{
public:
    static TestUrlList& instance() {
        static TestUrlList urlList;
        return urlList;
    }

    ~TestUrlList() = default;
    void initSourceDir(const std::string& source);
    bool getUrls(const std::string& testName, std::list<std::string>* results);

private:
    TestUrlList();
    std::string mapToDirectoryName(const std::string& testName);
    bool collectFileList(const std::string& dir, std::list<std::string>* fileList);


    std::string mSourceDir;
    std::map<std::string, std::list<std::string>> mFileListCache;

private:
    TestUrlList(const TestUrlList&) = delete;
    TestUrlList& operator=(const TestUrlList&) = delete;
};


}








#endif
