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
 * @brief IronBee --- String Assembly Implementation
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#include "ironbee_config_auto.h"

#include <ironbee/string_assembly.h>

#include <assert.h>

/**
 * Chunk of data to build string out of.
 **/
typedef struct ib_sa_chunk_t ib_sa_chunk_t;
struct ib_sa_chunk_t
{
    /** Data */
    const char *data;
    /** Length of data */
    size_t length;
    /** Next chunk */
    ib_sa_chunk_t *next;
};

struct ib_sa_t
{
    /** Memory pool to use for chunks. */
    ib_mpool_t *mp;
    /** First chunk */
    ib_sa_chunk_t *first_chunk;
    /** Last chunk so far */
    ib_sa_chunk_t *current_chunk;
    /** Length of all chunks */
    size_t length;
};

ib_status_t ib_sa_begin(
    ib_sa_t    **sa,
    ib_mpool_t  *parent_mp
)
{
    assert(sa != NULL);

    ib_status_t rc;
    ib_mpool_t *sa_mp;
    rc = ib_mpool_create(&sa_mp, "sa", parent_mp);
    if (rc != IB_OK) {
        assert(rc == IB_EALLOC);
        return rc;
    }

    ib_sa_t *local_sa = ib_mpool_alloc(sa_mp, sizeof(*local_sa));
    if (local_sa == NULL) {
        return IB_EALLOC;
    }

    local_sa->mp            = sa_mp;
    local_sa->first_chunk   = NULL;
    local_sa->current_chunk = NULL;
    local_sa->length        = 0;

    *sa = local_sa;

    return IB_OK;
}

ib_status_t ib_sa_append(
    ib_sa_t    *sa,
    const char *data,
    size_t      data_length
)
{
    assert(sa   != NULL);
    assert(data != NULL);

    ib_sa_chunk_t *chunk = ib_mpool_alloc(sa->mp, sizeof(*chunk));
    if (chunk == NULL) {
        return IB_EALLOC;
    }

    if (sa->first_chunk == NULL) {
        sa->first_chunk = sa->current_chunk = chunk;
    }
    else {
        sa->current_chunk = sa->current_chunk->next = chunk;
    }
    sa->length += data_length;

    chunk->next   = NULL;
    chunk->data   = data;
    chunk->length = data_length;

    return IB_OK;
}

ib_status_t ib_sa_finish(
    ib_sa_t    **sa,
    const char **dst,
    size_t      *dst_length,
    ib_mpool_t  *mp
)
{
    assert(sa         != NULL);
    assert(*sa        != NULL);
    assert(dst        != NULL);
    assert(dst_length != NULL);
    assert(mp         != NULL);

    char *buffer;
    char *cur;

    buffer = ib_mpool_alloc(mp, (*sa)->length);
    if (buffer == NULL) {
        return IB_EALLOC;
    }

    cur = buffer;
    for (
        const ib_sa_chunk_t *chunk = (*sa)->first_chunk;
        chunk != NULL;
        chunk = chunk->next
    )
    {
        assert(cur + chunk->length <= buffer + (*sa)->length);
        memcpy(cur, chunk->data, chunk->length);
        cur += chunk->length;
    }

    *dst        = buffer;
    *dst_length = (*sa)->length;

    ib_mpool_destroy((*sa)->mp);
    *sa         = NULL;

    return IB_OK;
}
