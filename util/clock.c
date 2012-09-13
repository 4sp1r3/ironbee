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
 * @brief IronBee --- Utility Functions
 *
 * @author William Metcalf <wmetcalf@qualys.com>
 * @author Brian Rectanus <brectanus@qualys.com>
 */

#include "ironbee_config_auto.h"

#include <ironbee/clock.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <sys/time.h>

#ifdef CLOCK_MONOTONIC_RAW
#define IB_CLOCK                  CLOCK_MONOTONIC_RAW
#else
#ifdef CLOCK_MONOTONIC
#define IB_CLOCK                  CLOCK_MONOTONIC
#endif /* CLOCK_MONOTONIC */
#endif /* CLOCK_MONOTONIC_RAW */

ib_clock_type_t ib_clock_type(void)
{
#ifdef IB_CLOCK
#ifdef CLOCK_MONOTONIC_RAW
    return IB_CLOCK_TYPE_MONOTONIC_RAW;
#else
    return IB_CLOCK_TYPE_MONOTONIC;
#endif /* CLOCK_MONOTONIC_RAW */
#else
    return IB_CLOCK_TYPE_NONMONOTONIC;
#endif /* IB_CLOCK */
}

ib_time_t ib_clock_get_time(void) {
    uint64_t usec;

#ifdef IB_CLOCK
    struct timespec ts;

    /* Ticks seem to be an undesirable due for many reasons.
     * IB_CLOCK is set to CLOCK_MONOTONIC which is vulnerable to slew or
     * if available set to CLOCK_MONOTONIC_RAW which does not suffer from
     * slew.
     *
     * timespec provides sec and nsec resolution so we have to convert to
     * msec.
     */
    clock_gettime(IB_CLOCK, &ts);

    /* There are 1 million microsecs in a sec.
     * There are 1000 nanosecs in a microsec
     */
    usec = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
#else
    struct timeval tv;

    gettimeofday(&tv, NULL);
    usec = ((tv.tv_sec * 1000000) + tv.tv_usec);
#endif

    return usec;
}

void ib_clock_gettimeofday(ib_timeval_t *tp) {
    assert(tp != NULL);

    struct timeval tv;

    gettimeofday(&tv, NULL);

    tp->tv_sec = (uint32_t)tv.tv_sec;
    tp->tv_usec = (uint32_t)tv.tv_usec;
}

void ib_clock_timestamp(char *buf, const ib_timeval_t *ptv)
{
    struct timeval tv;
    time_t t;
    struct tm *tm;

    if (ptv != NULL) {
        IB_CLOCK_ASSIGN_TIMEVAL(tv, *ptv);
    }
    else {
        gettimeofday(&tv, NULL);
    }
    t = tv.tv_sec;
    tm = localtime(&t);
    strftime(buf, 20, "%Y-%m-%dT%H:%M:%S", tm);
    snprintf(buf + 19, 8, ".%06lu", (unsigned long)tv.tv_usec);
    strftime(buf + 24, 6, "%z", tm);
}

void ib_clock_relative_timestamp(
    char               *buf,
    const ib_timeval_t *ptv,
    ib_time_t           offset
)
{
    ib_timeval_t adj_tv;

    if (ptv != NULL) {
        IB_CLOCK_ASSIGN_TIMEVAL(adj_tv, *ptv);
    }
    else {
        ib_clock_gettimeofday(&adj_tv);
    }
    IB_CLOCK_ADJUST_TIMEVAL(adj_tv, offset);

    ib_clock_timestamp(buf, &adj_tv);
}
