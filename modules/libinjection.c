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
 * @brief IronBee --- SQLi Module based on libinjection.
 *
 * This module utilizes libinjection to implement SQLi detection. The
 * libinjection library is the work of Nick Galbreath.
 *
 * http://www.client9.com/projects/libinjection/
 *
 * Transformations:
 *   - normalizeSqli: Normalize SQL routine from libinjection.
 *
 * Operators:
 *   - is_sqli: Returns true if the data contains SQL injection.
 *
 * @author Brian Rectanus <brectanus@qualys.com>
 */

/* See `man 7 feature_test_macros` on certain Linux flavors. */
#define _POSIX_C_SOURCE 200809L

#include <ironbee/context.h>
#include <ironbee/module.h>
#include <ironbee/mpool.h>
#include <ironbee/rule_engine.h>
#include <ironbee/transformation.h>
#include <ironbee/util.h>

#include <libinjection.h>

#include <assert.h>
#include <stdio.h>

/* Define the module name as well as a string version of it. */
#define MODULE_NAME        sqli
#define MODULE_NAME_STR    IB_XSTRINGIFY(MODULE_NAME)

/* Declare the public module symbol. */
IB_MODULE_DECLARE();

/* Finger printer database. */
typedef struct sqli_pattern_set_t {
    const char **patterns;     /**< Sorted array of patterns. */
    size_t       num_patterns; /**< Size of @ref patterns. */
} sqli_pattern_set_t;

/* Module configuration. */
typedef struct sqli_module_config_t {
    /* For now, only support main context configuration. */
    /**
     * Hash of set name to sqli_pattern_set_t*.
     **/
    ib_hash_t *pattern_sets;
} sqli_module_config_t;
static sqli_module_config_t sqli_initial_config = { NULL };

/* Normalization function prototype. */
typedef int (*sqli_tokenize_fn_t)(sfilter * sf, stoken_t * sout);

/*********************************
 * Transformations
 *********************************/

static
ib_status_t sqli_normalize_tfn(ib_engine_t *ib,
                               ib_mpool_t *mp,
                               void *tfn_data,
                               const ib_field_t *field_in,
                               const ib_field_t **field_out)
{
    assert(ib != NULL);
    assert(mp != NULL);
    assert(field_in != NULL);
    assert(field_out != NULL);

    sfilter sf;
    ib_bytestr_t *bs_in;
    ib_bytestr_t *bs_out;
    const char *buf_in;
    char *buf_in_start;
    size_t buf_in_len;
    char *buf_out;
    char *buf_out_end;
    size_t buf_out_len;
    size_t lead_len = 0;
    char prev_token_type;
    ib_field_t *field_new;
    ib_status_t rc;
    size_t pat_len;

    /* Currently only bytestring types are supported.
     * Other types will just get passed through. */
    if (field_in->type != IB_FTYPE_BYTESTR) {
        *field_out = field_in;
        return IB_OK;
    }

    /* Extract the underlying incoming value. */
    rc = ib_field_value(field_in, ib_ftype_bytestr_mutable_out(&bs_in));
    if (rc != IB_OK) {
        return rc;
    }

    /* Create a buffer big enough (double) to allow for normalization. */
    buf_in = (const char *)ib_bytestr_const_ptr(bs_in);
    buf_out = buf_out_end = (char *)ib_mpool_calloc(mp, 2, ib_bytestr_length(bs_in));
    if (buf_out == NULL) {
        return IB_EALLOC;
    }

    /* As SQL can be injected into a string, the normalization
     * needs to start after the first quote character if one
     * exists.
     *
     * First try single quote, then double, then none.
     *
     * TODO: Handle returning multiple transformations:
     *       1) Straight normalization
     *       2) Normalization as if with single quotes (starting point
     *          should be based on straight normalization)
     *       3) Normalization as if with double quotes (starting point
     *          should be based on straight normalization)
     */
    buf_in_start = memchr(buf_in, CHAR_SINGLE, ib_bytestr_length(bs_in));
    if (buf_in_start == NULL) {
        buf_in_start = memchr(buf_in, CHAR_DOUBLE, ib_bytestr_length(bs_in));
    }
    if (buf_in_start == NULL) {
        buf_in_start = (char *)buf_in;
        buf_in_len = ib_bytestr_length(bs_in);
    }
    else {
        ++buf_in_start; /* After the quote. */
        buf_in_len = ib_bytestr_length(bs_in) - (buf_in_start - buf_in);
    }

    /* Copy the leading string if one exists. */
    if (buf_in_start != buf_in) {
        lead_len = buf_in_start - buf_in;
        memcpy(buf_out, buf_in, lead_len);
        buf_out_end += lead_len;
    }

    /* Copy the normalized tokens as a space separated list. Since
     * the tokenizer does not backtrack, and the normalized values
     * are always equal to or less than the original length, the
     * tokens are written back to the beginning of the original
     * buffer.
     */
    libinjection_is_sqli(&sf, buf_in_start, buf_in_len, NULL, NULL);
    buf_out_len = 0;
    prev_token_type = 0;
    pat_len = strlen(sf.pat);
    for (size_t i = 0; i < pat_len; ++i) {
        stoken_t current = sf.tokenvec[i];
        size_t token_len = strlen(current.val);
        ib_log_debug2(ib, "SQLi TOKEN: %c \"%s\"", current.type, current.val);

        /* Add in the space if required. */
        if ((buf_out_end != buf_out) &&
            (current.type != 'o') &&
            (prev_token_type != 'o') &&
            (current.type != ',') &&
            (*(buf_out_end - 1) != ','))
        {
            *buf_out_end = ' ';
            buf_out_end += 1;
            ++buf_out_len;
        }

        /* Copy the token value. */
        memcpy(buf_out_end, current.val, token_len);
        buf_out_end += token_len;
        buf_out_len += token_len;

        prev_token_type = current.type;
    }


    /* Create the output field wrapping bs_out. */
    buf_out_len += lead_len;
    rc = ib_bytestr_alias_mem(&bs_out, mp, (uint8_t *)buf_out, buf_out_len);
    if (rc != IB_OK) {
        return rc;
    }
    rc = ib_field_create(&field_new, mp,
                         field_in->name, field_in->nlen,
                         IB_FTYPE_BYTESTR,
                         ib_ftype_bytestr_mutable_in(bs_out));
    if (rc == IB_OK) {
        *field_out = field_new;
    }
    return rc;
}

/*********************************
 * Operators
 *********************************/

static
ib_status_t sqli_op_create(
    ib_context_t  *ctx,
    const char    *parameters,
    void         **instance_data,
    void          *cbdata
)
{
    ib_engine_t *ib = ib_context_get_engine(ctx);
    ib_status_t rc;
    ib_module_t *m = (ib_module_t *)cbdata;
    const char *set_name;
    size_t set_name_len;

    const sqli_module_config_t *cfg = NULL;
    const sqli_pattern_set_t    *ps  = NULL;

    if (parameters == NULL) {
        ib_log_error(ib, "Missing parameter for operator sqli");
        return IB_EINVAL;
    }

    set_name = parameters;
    set_name_len = strlen(parameters);
    if (set_name[0] == '\'') {
        ++set_name;
        --set_name_len;
    }
    if (set_name[set_name_len-1] == '\'') {
        --set_name_len;
    }

    if (strncmp("default", set_name, set_name_len) == 0) {
        *instance_data = NULL;
        return IB_OK;
    }

    rc = ib_context_module_config(ctx, m, &cfg);
    assert(rc == IB_OK);
    assert(cfg->pattern_sets != NULL);

    rc = ib_hash_get_ex(cfg->pattern_sets, &ps, set_name, set_name_len);
    if (rc == IB_ENOENT) {
        ib_log_error(ib, "No such pattern set: %s", parameters);
        return IB_EINVAL;
    }
    assert(rc == IB_OK);
    assert(ps != NULL);

    *instance_data = (void *)ps;

    return IB_OK;
}

static
int sqli_cmp(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

static
int sqli_is_sqli_pattern(const char *pattern, void *cbdata)
{
    const sqli_pattern_set_t *ps = (const sqli_pattern_set_t *)cbdata;

    void *result = bsearch(
        pattern,
        *ps->patterns, ps->num_patterns, sizeof(*ps->patterns),
        &sqli_cmp
    );
    return result != NULL;
}

static
ib_status_t sqli_op_execute(
    ib_tx_t *tx,
    void *instance_data,
    const ib_field_t *field,
    ib_field_t *capture,
    ib_num_t *result,
    void *cbdata
)
{
    assert(tx            != NULL);
    assert(field         != NULL);
    assert(result        != NULL);

    sfilter sf;
    ib_bytestr_t *bs;
    ib_status_t rc;
    const sqli_pattern_set_t *ps = (const sqli_pattern_set_t *)instance_data;

    *result = 0;

    /* Currently only bytestring types are supported.
     * Other types will just get passed through. */
    if (field->type != IB_FTYPE_BYTESTR) {
        return IB_OK;
    }

    rc = ib_field_value(field, ib_ftype_bytestr_mutable_out(&bs));
    if (rc != IB_OK) {
        return rc;
    }

    /* Run through libinjection. */
    // TODO: Support alternative SQLi pattern lookup
    if (
        libinjection_is_sqli(
            &sf,
            (const char *)ib_bytestr_const_ptr(bs), ib_bytestr_length(bs),
            (ps != NULL ? sqli_is_sqli_pattern : NULL),
            (void *)ps
        )
    ) {
        ib_log_debug_tx(tx, "Matched SQLi pattern: %s", sf.pat);
        *result = 1;
    }

    return IB_OK;
}



/*********************************
 * Helper Functions
 *********************************/
static
ib_status_t sqli_create_pattern_set_from_file(
    sqli_pattern_set_t **out_ps,
    const char         *path,
    ib_mpool_t         *mp
)
{
    assert(out_ps != NULL);
    assert(path   != NULL);
    assert(mp     != NULL);

    ib_status_t  rc;
    FILE               *fp          = NULL;
    char               *buffer      = NULL;
    size_t              buffer_size = 0;
    ib_list_t          *items       = NULL;
    ib_list_node_t     *n           = NULL;
    ib_mpool_t         *tmp         = NULL;
    sqli_pattern_set_t *ps          = NULL;
    size_t              i           = 0;

    /* Temporary memory pool for this function only. */
    rc = ib_mpool_create(&tmp, "sqli tmp", NULL);
    assert(rc == IB_OK);
    assert(tmp != NULL);

    fp = fopen(path, "r");
    if (fp == NULL) {
        goto fail;
    }

    rc = ib_list_create(&items, tmp);
    assert(rc    == IB_OK);
    assert(items != NULL);

    for (;;) {
        char *buffer_copy;
        int   read = getline(&buffer, &buffer_size, fp);

        if (read == -1) {
            if (! feof(fp)) {
                fclose(fp);
                goto fail;
            }
            else {
                break;
            }
        }

        buffer_copy = ib_mpool_memdup(mp, buffer, read);
        assert(buffer_copy != NULL);
        while (buffer_copy[read-1] == '\n' || buffer_copy[read-1] == '\r') {
            buffer_copy[read-1] = '\0';
            --read;
        }

        rc = ib_list_push(items, (void *)buffer_copy);
        assert(rc == IB_OK);
    }

    fclose(fp);

    ps = ib_mpool_alloc(mp, sizeof(*ps));
    assert(ps != NULL);

    ps->num_patterns = ib_list_elements(items);
    ps->patterns =
        ib_mpool_alloc(mp, ps->num_patterns * sizeof(*ps->patterns));
    assert(ps->patterns != NULL);

    i = 0;
    IB_LIST_LOOP(items, n) {
        ps->patterns[i] = ib_list_node_data(n);
        ++i;
    }
    assert(i == ps->num_patterns);

    ib_mpool_destroy(tmp);

    qsort(
        ps->patterns, ps->num_patterns,
        sizeof(*ps->patterns),
        &sqli_cmp
    );

    *out_ps = ps;

    return IB_OK;

fail:
    ib_mpool_destroy(tmp);
    return IB_EINVAL;
}

/*********************************
 * Directive Functions
 *********************************/

static
ib_status_t sqli_dir_pattern_set(
    ib_cfgparser_t  *cp,
    const char      *directive_name,
    const char      *set_name,
    const char      *set_path,
    void            *cbdata
)
{
    assert(cp             != NULL);
    assert(directive_name != NULL);
    assert(set_name       != NULL);
    assert(set_path       != NULL);

    ib_status_t           rc;
    ib_context_t         *ctx = NULL;
    ib_module_t          *m   = NULL;
    sqli_module_config_t *cfg = NULL;
    sqli_pattern_set_t   *ps  = NULL;
    ib_mpool_t           *mp  = NULL;

    rc = ib_cfgparser_context_current(cp, &ctx);
    assert(rc  == IB_OK);
    assert(ctx != NULL);

    if (ctx != ib_context_main(cp->ib)) {
        ib_cfg_log_error(cp,
            "%s: Only valid at main context.", directive_name
        );
        return IB_EINVAL;
    }

    if (strcmp("default", set_name) == 0) {
        ib_cfg_log_error(cp,
            "%s: default is a reserved set name.", directive_name
        );
        return IB_EINVAL;
    }

    mp = ib_engine_pool_main_get(cp->ib);
    assert(mp != NULL);

    rc = ib_engine_module_get(
        ib_context_get_engine(ctx),
        MODULE_NAME_STR,
        &m
    );
    assert(rc == IB_OK);

    rc = ib_context_module_config(ctx, m, &cfg);
    assert(rc == IB_OK);

    if (cfg->pattern_sets == NULL) {
        rc = ib_hash_create(&cfg->pattern_sets, mp);
        assert(rc == IB_OK);
    }
    assert(cfg->pattern_sets != NULL);

    rc = ib_hash_get(cfg->pattern_sets, NULL, set_name);
    if (rc == IB_OK) {
        ib_cfg_log_error(cp,
            "%s: Duplicate pattern set definition: %s",
            directive_name, set_name
        );
        return IB_EINVAL;
    }
    assert(rc == IB_ENOENT);

    rc = sqli_create_pattern_set_from_file(&ps, set_path, mp);
    if (rc != IB_OK) {
        ib_cfg_log_error(cp,
            "%s: Failure to load pattern set from file: %s",
            directive_name, set_path
        );
        return IB_EINVAL;
    }
    assert(ps != NULL);

    rc = ib_hash_set(cfg->pattern_sets, ib_mpool_strdup(mp, set_name), ps);
    assert(rc == IB_OK);

    return IB_OK;
}

/*********************************
 * Module Functions
 *********************************/

/* Called to initialize a module (on load). */
static ib_status_t sqli_init(ib_engine_t *ib, ib_module_t *m, void *cbdata)
{
    assert(ib != NULL);
    assert(m != NULL);

    ib_status_t rc;

    ib_log_debug(ib, "Initializing %s module.", MODULE_NAME_STR);

    /* Register normalizeSqli transformation. */
    rc = ib_tfn_register(ib, "normalizeSqli", sqli_normalize_tfn,
                         IB_TFN_FLAG_NONE, NULL);
    if (rc != IB_OK) {
        return rc;
    }

    /* Register is_sqli operator. */
    rc = ib_operator_create_and_register(
        NULL,
        ib,
        "is_sqli",
        IB_OP_CAPABILITY_NON_STREAM,
        sqli_op_create, m,
        NULL, NULL,
        sqli_op_execute, NULL
    );
    if (rc != IB_OK) {
        return rc;
    }

    return IB_OK;
}

/* Called to finish a module (on unload). */
static ib_status_t sqli_fini(ib_engine_t *ib, ib_module_t *m, void *cbdata)
{
    ib_log_debug(ib, "Finish %s module.", MODULE_NAME_STR);

    return IB_OK;
}

static IB_DIRMAP_INIT_STRUCTURE(sqli_directive_map) = {
    IB_DIRMAP_INIT_PARAM2(
        "SQLiPatternSet",
        sqli_dir_pattern_set, NULL
    ),
    IB_DIRMAP_INIT_LAST
};

/* Initialize the module structure. */
IB_MODULE_INIT(
    IB_MODULE_HEADER_DEFAULTS,           /* Default metadata */
    MODULE_NAME_STR,                     /* Module name */
    IB_MODULE_CONFIG(&sqli_initial_config), /* Global config data */
    NULL,                                /* Configuration field map */
    sqli_directive_map,                  /* Config directive map */
    sqli_init,                           /* Initialize function */
    NULL,                                /* Callback data */
    sqli_fini,                           /* Finish function */
    NULL,                                /* Callback data */
);
