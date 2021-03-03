/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _TEST_MODULE_H_
#define _TEST_MODULE_H_

#include "demux/AmlTsParser.h"


namespace aml_mp {
class TestModule : virtual public AmlMpRefBase
{
public:
    struct Command {
        const char* name;
        size_t argCount;
        const char* help;
        int (*fn)(void* handle, const std::vector<std::string>& args);
    };

    TestModule();
    virtual ~TestModule();

    void processCommand(const std::vector<std::string>& args);
    static void printCommands(const Command* pCommands, bool printHeader);

protected:
    virtual const Command* getCommandTable() const {return nullptr;}
    virtual void* getCommandHandle() const {return nullptr;}

private:
    const Command* findCommand(const std::string& cmdName, const TestModule::Command* commandTable);

    TestModule(const TestModule&) = delete;
    TestModule& operator=(const TestModule&) = delete;
};

}


#endif
