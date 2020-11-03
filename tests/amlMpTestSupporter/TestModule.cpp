/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpPlayerDemo_TestModule"
#include <utils/Log.h>
#include <vector>
#include "TestModule.h"

namespace aml_mp {
TestModule::TestModule()
{

}

TestModule::~TestModule()
{

}

void TestModule::processCommand(const std::vector<std::string>& args)
{
    std::string cmdName = *args.begin();
    const struct Command* command = nullptr;
    const struct Command* commandTable = getCommandTable();
    if (!commandTable) {
        return;
    }

    command = findCommand(cmdName, commandTable);
    if (command == nullptr) {
        printf("Unknown command: %s\n", cmdName.c_str());
        return;
    }

    if (args.size() <= command->argCount) {
        printf("missing arguments, expect %d args\n", command->argCount);
        return;
    }

    command->fn(getCommandHandle(), args);
}

const TestModule::Command* TestModule::findCommand(const std::string& cmdName, const TestModule::Command* commandTable)
{
    if (commandTable == nullptr)
        return nullptr;

    const struct TestModule::Command* command = commandTable;
    const struct TestModule::Command* result  = nullptr;

    while (command && command->name) {
        if (command->name == cmdName) {
            result = command;
            break;
        }

        ++command;
    }

    return result;
}

void TestModule::printCommands(const Command* pCommands, bool printHeader)
{
    if (pCommands == nullptr) return;

    const struct Command* command = pCommands;

    if (printHeader && command && command->name) {
        printf("commands help:\n%20s | %s | %-s\n", "Command Name", "Args count", "Help");
    }

    while (command && command->name) {
        const char* help = command->help ? command->help : "";
        printf("%20s | %10u | %s\n", command->name, command->argCount, help);

        ++command;
    }
}


}
