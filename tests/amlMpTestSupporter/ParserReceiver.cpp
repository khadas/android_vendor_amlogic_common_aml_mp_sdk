/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo_ParserReceiver"
#include "ParserReceiver.h"

namespace aml_mp {

ParserReceiver::ParserReceiver(const sptr<Parser>& amlTsParser)
: mAmlTsParser(amlTsParser)
{
}
ParserReceiver::~ParserReceiver()
{
}

int ParserReceiver::writeData(const uint8_t* buffer, size_t size)
{
    if (mAmlTsParser) {
        mAmlTsParser->writeData(buffer, size);
    }
    int ret = ISourceReceiver::writeData(buffer, size);
    return ret;
}


}