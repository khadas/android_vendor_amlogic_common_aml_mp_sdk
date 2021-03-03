/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _PARSER_RECEIVER_H_
#define _PARSER_RECEIVER_H_

#include "source/Source.h"
#include <demux/AmlTsParser.h>

namespace aml_mp {

class ParserReceiver : public ISourceReceiver
{
public:
    ParserReceiver(const sptr<Parser>& amlTsParser);
    ~ParserReceiver();
    virtual int writeData(const uint8_t* buffer, size_t size);
private:
    sptr<Parser> mAmlTsParser;
    ParserReceiver(const ParserReceiver&) = delete;
    ParserReceiver& operator=(const ParserReceiver&) = delete;
};

}

#endif
