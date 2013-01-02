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
 *
 * IronBee - Unit testing utility functions implementation.
 *
 * @author Brian Rectanus <brectanus@qualys.com>
 */

#include "ibtest_util.hpp"

ib_server_t ibt_ibserver = {
   IB_SERVER_HEADER_DEFAULTS,
   "unit_tests",
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};

bool ibtest_memeq(const void *v1, const void *v2, size_t n)
{
    return memcmp(v1, v2, n) ? false : true;
}
