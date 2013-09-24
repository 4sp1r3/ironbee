/*****************************************************************************
 * Licensed to Qualys, Inc. (QUALYS) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * QUALYS licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/**
 * @file
 * @brief IronBee++ Internals --- ParsedResponseLine Tests
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 **/

#include <ironbeepp/parsed_response_line.hpp>
#include <ironbeepp/transaction.hpp>
#include <ironbeepp/byte_string.hpp>
#include <ironbeepp/memory_pool.hpp>

#include <ironbeepp/test_fixture.hpp>

#include "gtest/gtest.h"

class TestParsedResponseLine :
    public ::testing::Test, public IronBee::TestFixture
{
};

using namespace IronBee;

TEST_F(TestParsedResponseLine, basic)
{
    MemoryPool mp = MemoryPool::create();

    ib_parsed_resp_line_t ib_prl;

    ParsedResponseLine prl(&ib_prl);

    ASSERT_TRUE(prl);

    ib_prl.raw = ByteString::create(mp, "raw").ib();
    EXPECT_EQ(ib_prl.raw, prl.raw().ib());

    ib_prl.protocol = ByteString::create(mp, "foo").ib();
    EXPECT_EQ(ib_prl.protocol, prl.protocol().ib());

    ib_prl.status = ByteString::create(mp, "foo").ib();
    EXPECT_EQ(ib_prl.status, prl.status().ib());

    ib_prl.msg = ByteString::create(mp, "bar").ib();
    EXPECT_EQ(ib_prl.msg, prl.message().ib());
}

TEST_F(TestParsedResponseLine, create)
{
    const char* raw      = "raw";
    const char* protocol = "foo";
    const char* status   = "bar";
    const char* message  = "baz";

    ConstParsedResponseLine prl = ParsedResponseLine::create_alias(
        m_transaction,
        raw,      3,
        protocol, 3,
        status,   3,
        message,  3
    );

    ASSERT_TRUE(prl);

    EXPECT_EQ(raw,      prl.raw().to_s());
    EXPECT_EQ(protocol, prl.protocol().to_s());
    EXPECT_EQ(status,   prl.status().to_s());
    EXPECT_EQ(message,  prl.message().to_s());
}
