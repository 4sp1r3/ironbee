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
 * @brief IronBee --- Lock Utilities
 * @author Sam Baskinger <sbaskinger@qualys.com>
 */

#include "ironbee_config_auto.h"

#include <ironbee/lock.h>

ib_status_t ib_lock_init(ib_lock_t *lock)
{
    int rc = pthread_mutex_init(lock, NULL);
    if (rc != 0) {
        return IB_EALLOC;
    }


    return IB_OK;
}

ib_status_t ib_lock_lock(ib_lock_t *lock)
{
    int rc = pthread_mutex_lock(lock);
    if (rc != 0) {
        return IB_EUNKNOWN;
    }

    return IB_OK;
}

ib_status_t ib_lock_unlock(ib_lock_t *lock)
{
    int rc = pthread_mutex_unlock(lock);
    if (rc != 0) {
        return IB_EUNKNOWN;
    }

    return IB_OK;
}

ib_status_t ib_lock_destroy(ib_lock_t *lock)
{
    int rc = pthread_mutex_destroy(lock);
    if (rc != 0) {
        return IB_EUNKNOWN;
    }

    return IB_OK;
}
