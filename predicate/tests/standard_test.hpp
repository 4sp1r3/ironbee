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
 * @brief Predicate --- Standard Fixture
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 **/

#ifndef __PREDICATE__TESTS__STANDARD_TEST__
#define __PREDICATE__TESTS__STANDARD_TEST__

#include "predicate/dag.hpp"

#include "parse_fixture.hpp"
#include "../../ironbeepp/tests/fixture.hpp"

#include "gtest/gtest.h"

// XXX documentation

class StandardTest :
    public ::testing::Test,
    public IBPPTestFixture,
    public ParseFixture
{
protected:
    void SetUp();

    IronBee::Predicate::node_p parse(
        const std::string& text
    ) const;

    IronBee::Predicate::ValueList eval(
        const IronBee::Predicate::node_p& n
    );

    // The following copy the value out and thus are safe to use text
    // as there is no need to keep the expression tree around.
    bool eval_bool(
        const std::string& text
    );

    std::string eval_s(
        const std::string& text
    );

    int64_t eval_n(
        const std::string& text
    );

    IronBee::Predicate::node_cp transform(
        IronBee::Predicate::node_p n
    ) const;

    std::string transform(
        const std::string& s
    ) const;
};


#endif /* __PREDICATE__TESTS__STANDARD_TEST__ */