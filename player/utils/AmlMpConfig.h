/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _AML_MP_CONFIG_H_
#define _AML_MP_CONFIG_H_

#include <utils/RefBase.h>

namespace aml_mp {
using android::RefBase;


class AmlMpConfig final : public RefBase
{
public:
    explicit AmlMpConfig(int instanceId);
    ~AmlMpConfig();

public:
    int mInstanceId;
    int mLogDebug;

private:
    void init();
    void reset();

    template <typename T>
    void initProperty(const char* propertyName, T& value);

private:
    AmlMpConfig(const AmlMpConfig&) = delete;
    AmlMpConfig& operator= (const AmlMpConfig&) = delete;
};
}


#endif
