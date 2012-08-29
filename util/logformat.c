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
 * @brief IronBee &mdash; Utility Log Format helper functions
 * @author Nick LeRoy <nleroy@qualys.com>
 */

/**
 * Logformat helper for custom index file formats
 */

#include "ironbee_config_auto.h"

#include <ironbee/logformat.h>
#include <ironbee/debug.h>

#include <assert.h>
#include <stdlib.h>

typedef enum {
    STATE_NORMAL,
    STATE_FORMAT,
    STATE_BACKSLASH
} logformat_state_t;

ib_status_t ib_logformat_create(ib_mpool_t *mp,
                                ib_logformat_t **lf)
{
    IB_FTRACE_INIT();

    assert(mp != NULL);
    assert(lf != NULL);

    ib_logformat_t *new;
    ib_status_t rc;

    new = (ib_logformat_t *)ib_mpool_calloc(mp, 1, sizeof(*new));
    if (new == NULL) {
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }
    new->mp = mp;
    rc = ib_list_create(&new->items, mp);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    *lf = new;
    IB_FTRACE_RET_STATUS(IB_OK);
}

static ib_status_t create_item(ib_logformat_t *lf,
                               ib_logformat_itype_t itype,
                               ib_logformat_item_t **iptr,
                               bool push)
{
    IB_FTRACE_INIT();
    assert(lf != NULL);
    assert(lf->mp != NULL);
    assert(iptr != NULL);
    ib_status_t rc = IB_OK;

    ib_logformat_item_t *item =
        (ib_logformat_item_t *)ib_mpool_alloc(lf->mp, sizeof(*item));
    if (item == NULL) {
        *iptr = NULL;
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }
    item->itype = itype;
    *iptr = item;

    if (push) {
        rc = ib_list_push(lf->items, item);
    }

    IB_FTRACE_RET_STATUS(rc);
}

static ib_status_t create_item_literal(ib_logformat_t *lf,
                                       const char *start,
                                       const char *end)
{
    IB_FTRACE_INIT();
    assert(lf != NULL);
    assert(lf->mp != NULL);
    assert(start != NULL);
    assert(end != NULL);
    assert(end >= start);

    ib_logformat_item_t *item;
    size_t len = end - start;
    ib_status_t rc = IB_OK;

    /* Special case: Zero length string, do nothing */
    if (len == 0) {
        IB_FTRACE_RET_STATUS(IB_OK);
    }

    if (len <= IB_LOGFORMAT_MAX_SHORT_LITERAL) {
        rc = create_item(lf, item_type_literal, &item, true);
        if (rc != IB_OK) {
            IB_FTRACE_RET_STATUS(rc);
        }
        memcpy(item->item.literal.buf.short_str, start, len);
        item->item.literal.buf.short_str[len] = '\0';
        item->item.literal.len = len;
        IB_FTRACE_RET_STATUS(IB_OK);
    }

    /* Long string; we'll manually push the item after we're done */
    rc = create_item(lf, item_type_literal, &item, false);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }
    
    item->item.literal.buf.str = (char *)ib_mpool_alloc(lf->mp, len+1);
    if (item->item.literal.buf.str == NULL) {
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }
    memcpy(item->item.literal.buf.str, start, len);
    item->item.literal.buf.str[len] = '\0';
    item->item.literal.len = len;

    rc = ib_list_push(lf->items, item);
    IB_FTRACE_RET_STATUS(rc);
}

static ib_status_t create_item_field(ib_logformat_t *lf,
                                     char c)
{
    IB_FTRACE_INIT();
    assert(lf != NULL);
    assert(lf->mp != NULL);
    
    ib_status_t rc;
    ib_logformat_item_t *item;

    rc = create_item(lf, item_type_format, &item, true);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }
    item->item.field.fchar = c;
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_logformat_parse(ib_logformat_t *lf,
                               const char *format)
{
    IB_FTRACE_INIT();
    assert(lf != NULL);
    assert(lf->mp != NULL);
    assert(format != NULL);

    ib_status_t rc;
    char *literal_buf;
    char *literal_cur;
    size_t len = strlen(format);
    logformat_state_t state = STATE_NORMAL;
    size_t i;

    literal_buf = malloc(len + 1);
    if (literal_buf == NULL) {
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }
    literal_cur = literal_buf;

    /* Store the original format (right now its just for debugging) */
    lf->format = ib_mpool_strdup(lf->mp, format);
    if (lf->format == NULL) {
        rc = IB_EALLOC;
        goto cleanup;
    }

    for (i = 0; i < len; ++i) {
        switch (state) {
        case STATE_FORMAT:

            /* Which field? */
            switch (format[i]) {
            case IB_LOG_FIELD_REMOTE_ADDR :
            case IB_LOG_FIELD_LOCAL_ADDR :
            case IB_LOG_FIELD_HOSTNAME :
            case IB_LOG_FIELD_SITE_ID :
            case IB_LOG_FIELD_SENSOR_ID :
            case IB_LOG_FIELD_TRANSACTION_ID :
            case IB_LOG_FIELD_TIMESTAMP :
            case IB_LOG_FIELD_LOG_FILE :
            {
                rc = create_item_literal(lf, literal_buf, literal_cur);
                if (rc != IB_OK) {
                    goto cleanup;
                }

                rc = create_item_field(lf, format[i]);
                if (rc != IB_OK) {
                    goto cleanup;
                }
                literal_cur = literal_buf; /* Reset current literal pointer */
                break;
            }

            case '%':
                *literal_cur = '%';
                ++literal_cur;
                break;

            default:
                /* Not understood - ignore it */
                break;
            }
            state = STATE_NORMAL;
            break;

        case STATE_BACKSLASH:
            /* Avoid '\b', '\n', '\r' */
            switch (format[i]) {
            case 't':
                *literal_cur = '\t';
                ++literal_cur;
                break;

            case 'b':
            case 'n':
            case 'r':
                /* Just add a space (for now at least) */
                *literal_cur = ' ';
                ++literal_cur;
                break;

            default:
                /* Just use the character directly */
                *literal_cur = format[i];
                ++literal_cur;
                break;
            }
            state = STATE_NORMAL;
            break;

        case STATE_NORMAL:
            switch (format[i]) {
            case '\\':
                state = STATE_BACKSLASH;
                break;

            case '%':
                state = STATE_FORMAT;
                break;

            default:
                /* literal string character */
                *literal_cur = format[i];
                ++literal_cur;
            }
            break;
        } /* switch(state) */
    } /* for (i...) */

    if (state != STATE_NORMAL) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }

    /* Add any literal string we might be in the middle of */
    rc = create_item_literal(lf, literal_buf, literal_cur);

cleanup:
    if (literal_buf != NULL) {
        free(literal_buf);
    }
    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t ib_logformat_format(const ib_logformat_t *lf,
                                char *line,
                                size_t line_size,
                                size_t *line_len,
                                ib_logformat_fn_t fn,
                                void *fndata)
{
    IB_FTRACE_INIT();
    assert(lf != NULL);
    assert(line != NULL);
    assert(line_len != NULL);
    assert(fn != NULL);

    ib_status_t rc;
    const ib_list_node_t *node;
    size_t line_remain = line_size - 1;
    char *line_cur = line;
    bool truncated;

    IB_LIST_LOOP_CONST(lf->items, node) {
        const ib_logformat_item_t *item =
            (const ib_logformat_item_t *)ib_list_node_data_const(node);
        const char *str;
        size_t len;

        switch (item->itype) {
        case item_type_literal:
            len = item->item.literal.len;
            if (len <= IB_LOGFORMAT_MAX_SHORT_LITERAL) {
                str = item->item.literal.buf.short_str;
            }
            else {
                str = item->item.literal.buf.str;
            }
            break;

        case item_type_format:
            rc = fn(lf, &(item->item.field), fndata, &str);
            if (rc != IB_OK) {
                IB_FTRACE_RET_STATUS(rc);
            }
            len = strlen(str);
        }

        /* Copy into buffer */
        truncated = false;
        strncpy(line_cur, str, line_remain);
        if (len > line_remain) {
            len = line_remain;
            truncated = true;
        }
        line_cur += len;
        line_remain -= len;

        if (truncated) {
            IB_FTRACE_RET_STATUS(IB_ETRUNC);
        }
    }

    if (line_remain) {
        *line_cur = '\0';
        --line_remain;
        truncated = false;
    }
    else {
        truncated = true;
    }
    *line_len = (line_cur - line);

    IB_FTRACE_RET_STATUS(truncated ? IB_ETRUNC : IB_OK);
}
