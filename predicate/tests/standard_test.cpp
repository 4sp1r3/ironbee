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
 * @brief Predicate --- Standard Test Implementation
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 **/

#include "predicate/standard.hpp"
#include "predicate/merge_graph.hpp"

#include "standard_test.hpp"

using namespace std;
using namespace IronBee::Predicate;

void StandardTest::SetUp()
{
    Standard::load(m_factory);
    m_factory.add("A", &create);
    m_factory.add("B", &create);
}

node_p StandardTest::parse(const std::string& text) const
{
    size_t i = 0;
    return parse_call(text, i, m_factory);
}

ValueList StandardTest::eval(const node_p& n)
{
    Reporter r;
    NodeReporter nr(r, n);
    n->pre_transform(nr);
    if (r.num_errors() > 0 || r.num_warnings() > 0) {
        r.write_report(cout);
        BOOST_THROW_EXCEPTION(
            IronBee::einval() << IronBee::errinfo_what(
                "pre_transform() failed."
            )
        );
    }
    n->pre_eval(m_engine, nr);
    if (r.num_errors() > 0 || r.num_warnings() > 0) {
        r.write_report(cout);
        BOOST_THROW_EXCEPTION(
            IronBee::einval() << IronBee::errinfo_what(
                "pre_eval() failed."
            )
        );
    }
    return n->eval(m_transaction);
}

bool StandardTest::eval_bool(const string& text)
{
    node_p n = parse(text);
    return ! eval(n).empty();
}

string StandardTest::eval_s(const string& text)
{
    node_p n = parse(text);
    ValueList vals = eval(n);
    if (vals.empty()) {
        throw runtime_error("eval_s called on null value.");
    }
    if (vals.size() != 1) {
        throw runtime_error("eval_s called on non-simple value.");
    }
    IronBee::ConstByteString bs = vals.front().value_as_byte_string();
    return bs.to_s();
}

int64_t StandardTest::eval_n(const string& text)
{
    node_p n = parse(text);
    ValueList vals = eval(n);
    if (vals.size() != 1) {
        throw runtime_error("eval_s called on invalid value.");
    }
    return vals.front().value_as_number();
}

node_cp StandardTest::transform(node_p n) const
{
    MergeGraph G;
    Reporter r;
    size_t i = G.add_root(n);
    n->transform(G, m_factory, NodeReporter(r, n));
    if (r.num_warnings() || r.num_errors()) {
        throw runtime_error("Warnings/Errors during transform.");
    }
    return G.root(i);
}

string StandardTest::transform(const string& s) const
{
    return transform(parse(s))->to_s();
}
