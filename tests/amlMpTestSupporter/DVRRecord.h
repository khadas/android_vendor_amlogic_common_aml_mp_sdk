/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "Parser.h"
#include "TestModule.h"
#include <Aml_MP/Aml_MP.h>

namespace aml_mp {
class DVRRecord : public TestModule, public ISourceReceiver
{
public:
    DVRRecord();
    ~DVRRecord();

    int start();
    int stop();

protected:
    virtual const Command* getCommandTable() const override;
    virtual void* getCommandHandle() const override;

private:
    DVRRecord(const DVRRecord&) = delete;
    DVRRecord& operator=(const DVRRecord&) = delete;
};

}
