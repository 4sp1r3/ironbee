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
 * @brief IronBee++ Internals --- Configuration Directives Tests
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 **/

#include <ironbeepp/configuration_parser.hpp>
#include <ironbeepp/context.hpp>
#include <ironbeepp/site.hpp>
#include "fixture.hpp"

#include "gtest/gtest.h"

#include "engine_private.h"

#include <ironbee/engine.h>
#include <ironbee/config.h>

using namespace std;
using namespace IronBee;

class TestConfigurationParser :
    public ::testing::Test, public IBPPTestFixture
{
};

TEST_F(TestConfigurationParser, ConfigurationParser)
{
    ib_cfgparser_t parser;

    ConfigurationParser P(&parser);

    ASSERT_TRUE(P);
    ASSERT_EQ(&parser, P.ib());

    parser.ib = m_engine.ib();
    parser.mp = m_engine.main_memory_pool().ib();

    ib_context_t ctx;
    parser.cur_ctx = &ctx;
    parser.cur_file = "testfile";
    parser.cur_blkname = "foobar";

    EXPECT_EQ(parser.ib, P.engine().ib());
    EXPECT_EQ(parser.mp, P.memory_pool().ib());
    EXPECT_EQ(parser.cur_ctx, P.current_context().ib());
    EXPECT_EQ(parser.cur_file, P.current_file());
    EXPECT_EQ(parser.cur_blkname, P.current_block_name());

    // Parse routines tested in test_configuration_directives.
}

TEST_F(TestConfigurationParser, create_destroy)
{
    ConfigurationParser P = ConfigurationParser::create(m_engine);
    ASSERT_TRUE(P);
    P.destroy();
}
