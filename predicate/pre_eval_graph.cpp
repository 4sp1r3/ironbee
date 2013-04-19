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
 * @brief Predicate --- Pre-Eval Graph Implementation
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#include <predicate/pre_eval_graph.hpp>

#include <predicate/bfs.hpp>
#include <predicate/merge_graph.hpp>

#include <boost/bind.hpp>
#include <boost/function_output_iterator.hpp>

using namespace std;

namespace IronBee {
namespace Predicate {

namespace  {

//! Call pre_eval on node.
class pre_eval_graph_helper
{
public:
    pre_eval_graph_helper(reporter_t reporter, Environment environment) :
        m_reporter(reporter),
        m_environment(environment)
    {
        // nop
    }

    void operator()(const node_p& n)
    {
        n->pre_eval(m_environment, NodeReporter(m_reporter, n));
    }

private:
    reporter_t  m_reporter;
    Environment m_environment;
};

}

void pre_eval_graph(
    reporter_t  reporter,
    MergeGraph& graph,
    Environment environment
)
{
    bfs_down(
        graph.roots().first, graph.roots().second,
        boost::make_function_output_iterator(
            pre_eval_graph_helper(reporter, environment)
        )
    );
}

} // Predicate
} // IronBee
