//////////////////////////////////////////////////////////////////////////////
// Licensed to Qualys, Inc. (QUALYS) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.
// QUALYS licenses this file to You under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance with
// the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief IronBee - Test string util lower case functions
///
/// @author Nick LeRoy <nleroy@qualys.com>
//////////////////////////////////////////////////////////////////////////////

#include "ironbee_config_auto.h"

#include <ironbee/types.h>
#include <ironbee/string.h>

#include "ibtest_strbase.hh"

#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"

#include <stdexcept>


class TestStringLower : public TestSimpleStringManipulation
{
public:
    const char *TestName(ib_strop_t op, test_type_t tt)
    {
        return TestNameImpl("string_lower", op, tt);
    }

    // Inplace operators
    ib_status_t ExecInplaceNul(char *buf, ib_flags_t &result)
    {
        char *out;
        return ib_strlower(IB_STROP_INPLACE, m_mpool,
                           buf, &out, &result);
    }
    ib_status_t ExecInplaceEx(uint8_t *data_in,
                              size_t dlen_in,
                              size_t &dlen_out,
                              ib_flags_t &result)
    {
        uint8_t *data_out;
        return ib_strlower_ex(IB_STROP_INPLACE, m_mpool,
                              data_in, dlen_in,
                              &data_out, &dlen_out,
                              &result);
    }

    // Copy operators
    ib_status_t ExecCopyNul(const char *data_in,
                            char **data_out,
                            ib_flags_t &result)
    {
        return ib_strlower(IB_STROP_COPY, m_mpool,
                           (char *)data_in, data_out, &result);
    }
    ib_status_t ExecCopyEx(const uint8_t *data_in,
                           size_t dlen_in,
                           uint8_t **data_out,
                           size_t &dlen_out,
                           ib_flags_t &result)
    {
        return ib_strlower_ex(IB_STROP_COPY, m_mpool,
                              (uint8_t *)data_in, dlen_in,
                              data_out, &dlen_out,
                              &result);
    }

    // Copy-on-write operators
    ib_status_t ExecCowNul(const char *data_in,
                           char **data_out,
                           ib_flags_t &result)
    {
        return ib_strlower(IB_STROP_COW, m_mpool,
                           (char *)data_in, data_out, &result);
    }
    ib_status_t ExecCowEx(const uint8_t *data_in,
                          size_t dlen_in,
                          uint8_t **data_out,
                          size_t &dlen_out,
                          ib_flags_t &result)
    {
        return ib_strlower_ex(IB_STROP_COW, m_mpool,
                              (uint8_t *)data_in, dlen_in,
                              data_out, &dlen_out,
                              &result);
    }
};

TEST_F(TestStringLower, Basic)
{
    {
        SCOPED_TRACE("Empty");
        RunTest("");
    }
    {
        SCOPED_TRACE("Basic #1");
        RunTest("test case");
    }
    {
        SCOPED_TRACE("Basic #2");
        RunTest("Test Case", "test case");
    }
    {
        SCOPED_TRACE("Basic #3");
        uint8_t in[] = "Test\0Case";
        uint8_t out[] = "test\0case";
        RunTest(in, sizeof(in)-1, out, sizeof(out)-1);
    }
    {
        SCOPED_TRACE("Basic #4");
        RunTest("ABC def GHI", "abc def ghi");
    }
}
