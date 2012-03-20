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
 * @brief IronBee++ Internals -- Configuration Map Tests
 * @internal
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 **/

#include <ironbeepp/configuration_map.hpp>
#include <ironbeepp/module.hpp>
#include "fixture.hpp"

#include "gtest/gtest.h"

#include <ironbee/debug.h>

#include <boost/bind.hpp>

using namespace std;

class TestConfigurationMap : public ::testing::Test, public IBPPTestFixture
{
protected:
    ib_cfgmap_t* setup_cfgmap(const ib_cfgmap_init_t* init, void* data)
    {
        ib_cfgmap_t* cm = NULL;
        ib_status_t rc;
        rc = ib_cfgmap_create(&cm, ib_engine_pool_main_get(m_ib_engine));
        EXPECT_EQ(IB_OK, rc);
        rc = ib_cfgmap_init(cm, data, init);
        EXPECT_EQ(IB_OK, rc);

        return cm;
    }
};

template <typename T>
T cfgmap_get(
    const ib_cfgmap_t* cm,
    const char*        name,
    ib_ftype_t         expected_type
)
{
    T v;
    ib_ftype_t actual_type;
    ib_status_t rc = ib_cfgmap_get(cm, name, &v, &actual_type);
    EXPECT_EQ(IB_OK, rc);
    EXPECT_EQ(expected_type, actual_type);

    return v;
}

template <typename T>
void cfgmap_set(
    ib_cfgmap_t* cm,
    const char* name,
    T value
)
{
    ib_status_t rc = ib_cfgmap_set(cm, name, &value);
    EXPECT_EQ(IB_OK, rc);
}

struct test_data_t
{
    int                 s;
    unsigned int        u;
    const char*         n;
    IronBee::ByteString b;
    std::string         ss;
};

TEST_F(TestConfigurationMap, DataMember)
{
    ib_module_t ib_module;
    ib_module.ib = m_ib_engine;
    IronBee::Module m(&ib_module);
    IronBee::MemoryPool mpool(ib_engine_pool_main_get(m_ib_engine));

    IronBee::ConfigurationMapInit<test_data_t> cmi(m.ib()->cm_init, mpool);

    cmi.number("s", &test_data_t::s);
    cmi.unsigned_number("u", &test_data_t::u);
    cmi.null_string("n", &test_data_t::n);
    cmi.byte_string("b", &test_data_t::b);
    cmi.byte_string_s("ss", &test_data_t::ss);
    cmi.finish();

    test_data_t data;
    ib_cfgmap_t* cm = setup_cfgmap(ib_module.cm_init, &data);
    ASSERT_TRUE(cm);

    data.s = 13;
    EXPECT_EQ(data.s, cfgmap_get<ib_num_t>(cm, "s", IB_FTYPE_NUM));
    cfgmap_set<ib_num_t>(cm, "s", 19);
    EXPECT_EQ(19, data.s);

    data.u = -13;
    EXPECT_EQ(data.u, cfgmap_get<ib_unum_t>(cm, "u", IB_FTYPE_UNUM));
    cfgmap_set<ib_unum_t>(cm, "u", -19);
    EXPECT_EQ(-19, data.u);

    const char* s1 = "Hello World";
    const char* s2 = "Foobar";
    data.n = s1;
    EXPECT_EQ(data.n, cfgmap_get<const char*>(cm, "n", IB_FTYPE_NULSTR));
    cfgmap_set<const char*>(cm, "n", s2);
    EXPECT_EQ(string(s2), data.n);

    IronBee::ByteString bs1
        = IronBee::ByteString::create(mpool, "Hello World");
    IronBee::ByteString bs2
        = IronBee::ByteString::create(mpool, "Foobar");
    data.b = bs1;
    EXPECT_EQ(
        data.b,
        cfgmap_get<IronBee::ConstByteString>(cm, "b", IB_FTYPE_BYTESTR)
    );
    cfgmap_set<IronBee::ConstByteString>(cm, "b", bs2);
    EXPECT_EQ(bs2.to_s(), data.b.to_s());

    data.ss = std::string("abc");
    EXPECT_EQ(
        data.ss,
        cfgmap_get<IronBee::ConstByteString>(
            cm, "ss", IB_FTYPE_BYTESTR
        ).to_s()
    );
    cfgmap_set<IronBee::ConstByteString>(cm, "ss", bs2);
    EXPECT_EQ(bs2.to_s(), data.ss);
}

struct test_data2_t
{
    static int         s_which;
    static std::string s_name;
    static test_data_t s_data;

    static void reset()
    {
        s_which = 0;
        s_name.clear();
    }

    int64_t get_number(const std::string& name) const
    {
        s_which = 1;
        s_name = name;
        return s_data.s;
    }

    void set_number(const std::string& name, int64_t v) const
    {
        s_which = 1;
        s_name = name;
        s_data.s = v;
    }

    uint64_t get_unsigned_number(const std::string& name) const
    {
        s_which = 2;
        s_name = name;
        return s_data.u;
    }

    void set_unsigned_number(const std::string& name, uint64_t v) const
    {
        s_which = 2;
        s_name = name;
        s_data.u = v;
    }

    const char* get_null_string(const std::string& name) const
    {
        s_which = 3;
        s_name = name;
        return s_data.n;
    }

    void set_null_string(const std::string& name, const char* v) const
    {
        s_which = 3;
        s_name = name;
        s_data.n = v;
    }

    IronBee::ConstByteString get_byte_string(const std::string& name) const
    {
        s_which = 4;
        s_name = name;
        return s_data.b;
    }

    void set_byte_string(
        const std::string& name,
        IronBee::ConstByteString v
    ) const
    {
        s_which = 4;
        s_name = name;
        s_data.b.clear();
        s_data.b.append(v);
    }

    std::string get_string(const std::string& name) const
    {
        s_which = 5;
        s_name = name;
        return s_data.ss;
    }

    void set_string(
        const std::string& name,
        const std::string& v
    ) const
    {
        s_which = 5;
        s_name = name;
        s_data.b.clear();
        s_data.b.append(v);
    }
};
int         test_data2_t::s_which;
std::string test_data2_t::s_name;
test_data_t test_data2_t::s_data;

TEST_F(TestConfigurationMap, FunctionMember)
{
    ib_module_t ib_module;
    ib_module.ib = m_ib_engine;
    IronBee::Module m(&ib_module);
    IronBee::MemoryPool mpool(ib_engine_pool_main_get(m_ib_engine));

    IronBee::ConfigurationMapInit<test_data2_t> cmi(m.ib()->cm_init, mpool);

    cmi.number(
        "s",
        &test_data2_t::get_number,
        &test_data2_t::set_number
    );
    cmi.unsigned_number(
        "u",
        &test_data2_t::get_unsigned_number,
        &test_data2_t::set_unsigned_number
    );
    cmi.null_string(
        "n",
        &test_data2_t::get_null_string,
        &test_data2_t::set_null_string
    );
    cmi.byte_string(
        "b",
        &test_data2_t::get_byte_string,
        &test_data2_t::set_byte_string
    );
    cmi.byte_string_s(
        "ss",
        &test_data2_t::get_string,
        &test_data2_t::set_string
    );
    cmi.finish();

    test_data2_t data;
    ib_cfgmap_t* cm = setup_cfgmap(ib_module.cm_init, &data);
    ASSERT_TRUE(cm);

    test_data2_t::s_data.s = 13;
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.s,
        cfgmap_get<ib_num_t>(cm, "s", IB_FTYPE_NUM)
    );
    EXPECT_EQ(1,   test_data2_t::s_which);
    EXPECT_EQ("s", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<ib_num_t>(cm, "s", 19);
    EXPECT_EQ(19,  test_data2_t::s_data.s);
    EXPECT_EQ(1,   test_data2_t::s_which);
    EXPECT_EQ("s", test_data2_t::s_name);

    test_data2_t::s_data.u = -13;
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.u,
        cfgmap_get<ib_unum_t>(cm, "u", IB_FTYPE_UNUM)
    );
    EXPECT_EQ(2,   test_data2_t::s_which);
    EXPECT_EQ("u", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<ib_unum_t>(cm, "u", -19);
    EXPECT_EQ(-19, test_data2_t::s_data.u);
    EXPECT_EQ(2,   test_data2_t::s_which);
    EXPECT_EQ("u", test_data2_t::s_name);

    const char* s1 = "Hello World";
    const char* s2 = "Foobar";
    test_data2_t::s_data.n = s1;
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.n,
        cfgmap_get<const char*>(cm, "n", IB_FTYPE_NULSTR)
    );
    EXPECT_EQ(3,   test_data2_t::s_which);
    EXPECT_EQ("n", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<const char*>(cm, "n", s2);
    EXPECT_EQ(s2,  test_data2_t::s_data.n);
    EXPECT_EQ(3,   test_data2_t::s_which);
    EXPECT_EQ("n", test_data2_t::s_name);

    IronBee::ByteString bs1
        = IronBee::ByteString::create(mpool, "Hello World");
    IronBee::ByteString bs2
        = IronBee::ByteString::create(mpool, "Foobar");
    test_data2_t::s_data.b = bs1;
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.b.to_s(),
        cfgmap_get<IronBee::ConstByteString>(cm, "b", IB_FTYPE_BYTESTR).to_s()
    );
    EXPECT_EQ(4,   test_data2_t::s_which);
    EXPECT_EQ("b", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<IronBee::ConstByteString>(cm, "b", bs2);
    EXPECT_EQ(bs2.to_s(), test_data2_t::s_data.b.to_s());
    EXPECT_EQ(4,          test_data2_t::s_which);
    EXPECT_EQ("b",        test_data2_t::s_name);

    test_data2_t::s_data.ss = bs1.to_s();
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.ss,
        cfgmap_get<IronBee::ConstByteString>(
            cm, "ss", IB_FTYPE_BYTESTR
        ).to_s()
    );
    EXPECT_EQ(5,    test_data2_t::s_which);
    EXPECT_EQ("ss", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<IronBee::ConstByteString>(cm, "ss", bs2);
    EXPECT_EQ(bs2.to_s(), test_data2_t::s_data.ss);
    EXPECT_EQ(5,          test_data2_t::s_which);
    EXPECT_EQ("ss",       test_data2_t::s_name);
}

TEST_F(TestConfigurationMap, Functional)
{
    ib_module_t ib_module;
    ib_module.ib = m_ib_engine;
    IronBee::Module m(&ib_module);
    IronBee::MemoryPool mpool(ib_engine_pool_main_get(m_ib_engine));

    IronBee::ConfigurationMapInit<test_data2_t> cmi(m.ib()->cm_init, mpool);
    test_data2_t data;

    cmi.number(
        "s",
        boost::bind(&test_data2_t::get_number, _1, _2),
        boost::bind(&test_data2_t::set_number, _1, _2, _3)
    );
    cmi.unsigned_number(
        "u",
        boost::bind(&test_data2_t::get_unsigned_number, _1, _2),
        boost::bind(&test_data2_t::set_unsigned_number, _1, _2, _3)
    );
    cmi.null_string(
        "n",
        boost::bind(&test_data2_t::get_null_string, _1, _2),
        boost::bind(&test_data2_t::set_null_string, _1, _2, _3)
    );
    cmi.byte_string(
        "b",
        boost::bind(&test_data2_t::get_byte_string, _1, _2),
        boost::bind(&test_data2_t::set_byte_string, _1, _2, _3)
    );
    cmi.byte_string_s(
        "ss",
        boost::bind(&test_data2_t::get_string, _1, _2),
        boost::bind(&test_data2_t::set_string, _1, _2, _3)
    );
    cmi.finish();

    ib_cfgmap_t* cm = setup_cfgmap(ib_module.cm_init, &data);
    ASSERT_TRUE(cm);

    test_data2_t::s_data.s = 13;
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.s,
        cfgmap_get<ib_num_t>(cm, "s", IB_FTYPE_NUM)
    );
    EXPECT_EQ(1,   test_data2_t::s_which);
    EXPECT_EQ("s", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<ib_num_t>(cm, "s", 19);
    EXPECT_EQ(19,  test_data2_t::s_data.s);
    EXPECT_EQ(1,   test_data2_t::s_which);
    EXPECT_EQ("s", test_data2_t::s_name);

    test_data2_t::s_data.u = -13;
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.u,
        cfgmap_get<ib_unum_t>(cm, "u", IB_FTYPE_UNUM)
    );
    EXPECT_EQ(2,   test_data2_t::s_which);
    EXPECT_EQ("u", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<ib_unum_t>(cm, "u", -19);
    EXPECT_EQ(-19, test_data2_t::s_data.u);
    EXPECT_EQ(2,   test_data2_t::s_which);
    EXPECT_EQ("u", test_data2_t::s_name);

    const char* s1 = "Hello World";
    const char* s2 = "Foobar";
    test_data2_t::s_data.n = s1;
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.n,
        cfgmap_get<const char*>(cm, "n", IB_FTYPE_NULSTR)
    );
    EXPECT_EQ(3,   test_data2_t::s_which);
    EXPECT_EQ("n", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<const char*>(cm, "n", s2);
    EXPECT_EQ(s2,  test_data2_t::s_data.n);
    EXPECT_EQ(3,   test_data2_t::s_which);
    EXPECT_EQ("n", test_data2_t::s_name);

    IronBee::ByteString bs1
        = IronBee::ByteString::create(mpool, "Hello World");
    IronBee::ByteString bs2
        = IronBee::ByteString::create(mpool, "Foobar");
    test_data2_t::s_data.b = bs1;
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.b.to_s(),
        cfgmap_get<IronBee::ConstByteString>(cm, "b", IB_FTYPE_BYTESTR).to_s()
    );
    EXPECT_EQ(4,   test_data2_t::s_which);
    EXPECT_EQ("b", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<IronBee::ConstByteString>(cm, "b", bs2);
    EXPECT_EQ(bs2.to_s(), test_data2_t::s_data.b.to_s());
    EXPECT_EQ(4,          test_data2_t::s_which);
    EXPECT_EQ("b",        test_data2_t::s_name);

    test_data2_t::s_data.ss = bs1.to_s();
    test_data2_t::reset();
    EXPECT_EQ(
        test_data2_t::s_data.ss,
        cfgmap_get<IronBee::ConstByteString>(
            cm, "ss", IB_FTYPE_BYTESTR
        ).to_s()
    );
    EXPECT_EQ(5,    test_data2_t::s_which);
    EXPECT_EQ("ss", test_data2_t::s_name);
    test_data2_t::reset();
    cfgmap_set<IronBee::ConstByteString>(cm, "ss", bs2);
    EXPECT_EQ(bs2.to_s(), test_data2_t::s_data.ss);
    EXPECT_EQ(5,          test_data2_t::s_which);
    EXPECT_EQ("ss",       test_data2_t::s_name);
}
