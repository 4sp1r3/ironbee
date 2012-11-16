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
 * @brief IronBee
 *
 * @author Brian Rectanus <brectanus@qualys.com>
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#include "ironbee_config_auto.h"

#include <ironbee/engine.h>
#include "engine_private.h"

#include "state_notify_private.h"

#include <ironbee/cfgmap.h>
#include <ironbee/core.h>
#include <ironbee/debug.h>
#include <ironbee/string.h>
#include <ironbee/ip.h>
#include <ironbee/hash.h>
#include <ironbee/module.h>
#include <ironbee/mpool.h>
#include <ironbee/server.h>
#include <ironbee/state_notify.h>
#include <ironbee/context_selection.h>

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h> /* htonl */
#include <sys/types.h> /* getpid */

/* -- Constants -- */

/** Constant max path used in calls to getcwd() */
const size_t maxpath = 512;

/** Constant String Values */
const ib_default_string_t ib_default_string = {
    "",          /* empty */
    "unknown",   /* unknown */
    "core",      /* core */
    "/",         /* root_path */
    "/",         /* uri_root_path */
};

const char *default_auditlog_index = "ironbee-index.log";

/* -- Internal Structures -- */

#if 0
typedef struct ib_field_callback_data_t ib_field_callback_data_t;
#endif

/**
 * List of callback data types for event id to type lookups.
 */
static const ib_state_hook_type_t ib_state_event_hook_types[] = {
    /* Engine States */
    IB_STATE_HOOK_CONN,     /**< conn_started_event */
    IB_STATE_HOOK_CONN,     /**< conn_finished_event */
    IB_STATE_HOOK_TX,       /**< tx_started_event */
    IB_STATE_HOOK_TX,       /**< tx_process_event */
    IB_STATE_HOOK_TX,       /**< tx_finished_event */

    /* Handler States */
    IB_STATE_HOOK_CONN,     /**< handle_context_conn_event */
    IB_STATE_HOOK_CONN,     /**< handle_connect_event */
    IB_STATE_HOOK_TX,       /**< handle_context_tx_event */
    IB_STATE_HOOK_TX,       /**< handle_request_header_event */
    IB_STATE_HOOK_TX,       /**< handle_request_event */
    IB_STATE_HOOK_TX,       /**< handle_response_header_event */
    IB_STATE_HOOK_TX,       /**< handle_response_event */
    IB_STATE_HOOK_CONN,     /**< handle_disconnect_event */
    IB_STATE_HOOK_TX,       /**< handle_postprocess_event */

    /* Server States */
    IB_STATE_HOOK_NULL,     /**< cfg_started_event */
    IB_STATE_HOOK_NULL,     /**< cfg_finished_event */
    IB_STATE_HOOK_CONN,     /**< conn_opened_event */
    IB_STATE_HOOK_CONNDATA, /**< conn_data_in_event */
    IB_STATE_HOOK_CONNDATA, /**< conn_data_out_event */
    IB_STATE_HOOK_CONN,     /**< conn_closed_event */

    /* Parser States */
    IB_STATE_HOOK_REQLINE,  /**< request_started_event */
    IB_STATE_HOOK_HEADER,   /**< request_header_data_event */
    IB_STATE_HOOK_TX,       /**< request_header_finished_event */
    IB_STATE_HOOK_TXDATA,   /**< request_body_data_event */
    IB_STATE_HOOK_TX,       /**< request_finished_event */
    IB_STATE_HOOK_RESPLINE, /**< response_started_event */
    IB_STATE_HOOK_HEADER,   /**< response_header_data_event */
    IB_STATE_HOOK_TX,       /**< response_header_finished_event */
    IB_STATE_HOOK_TXDATA,   /**< response_body_data_event */
    IB_STATE_HOOK_TX        /**< response_finished_event */
};

/* -- Internal Routines -- */

ib_status_t ib_check_hook(
    ib_engine_t* ib,
    ib_state_event_type_t event,
    ib_state_hook_type_t hook_type
) {
    IB_FTRACE_INIT();
    static const size_t num_events =
        sizeof(ib_state_event_hook_types) / sizeof(ib_state_hook_type_t);
    ib_state_hook_type_t expected_hook_type;

    if (event > num_events) {
        ib_log_error( ib,
            "Event/hook mismatch: Unknown event type: %d", event
        );
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }

    expected_hook_type = ib_state_event_hook_types[event];
    if ( expected_hook_type != hook_type ) {
        ib_log_debug(ib,
                     "Event/hook mismatch: "
                     "Event type %s expected %d but received %d",
                     ib_state_event_name(event),
                     expected_hook_type, hook_type);
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }

    IB_FTRACE_RET_STATUS(IB_OK);
}

static ib_status_t ib_register_hook(
    ib_engine_t* ib,
    ib_state_event_type_t event,
    ib_hook_t* hook
) {
    IB_FTRACE_INIT();

    ib_hook_t *last = ib->hook[event];

    /* Insert the hook at the end of the list */
    if (last == NULL) {
        ib_log_debug3(ib, "Registering %s hook: %p",
                      ib_state_event_name(event),
                      hook->callback.as_void);

        ib->hook[event] = hook;

        IB_FTRACE_RET_STATUS(IB_OK);
    }
    while (last->next != NULL) {
        last = last->next;
    }

    last->next = hook;

    ib_log_debug3(ib, "Registering %s hook after %p: %p",
                  ib_state_event_name(event), last->callback.as_void,
                  hook->callback.as_void);

    IB_FTRACE_RET_STATUS(IB_OK);
}

static ib_status_t ib_unregister_hook(
    ib_engine_t* ib,
    ib_state_event_type_t event,
    ib_void_fn_t cb
) {
    IB_FTRACE_INIT();
    ib_hook_t *prev = NULL;
    ib_hook_t *hook = ib->hook[event];

    /* Remove the first matching hook */
    while (hook != NULL) {
        if (hook->callback.as_void == cb) {
            if (prev == NULL) {
                ib->hook[event] = hook->next;
            }
            else {
                prev->next = hook->next;
            }
            IB_FTRACE_RET_STATUS(IB_OK);
        }
        prev = hook;
        hook = hook->next;
    }

    IB_FTRACE_RET_STATUS(IB_ENOENT);
}

/* -- Main Engine Routines -- */

ib_status_t ib_engine_create(ib_engine_t **pib, ib_server_t *server)
{
    IB_FTRACE_INIT();
    ib_mpool_t *pool;
    ib_status_t rc;

    /* Create primary memory pool */
    rc = ib_mpool_create(&pool, "engine", NULL);
    if (rc != IB_OK) {
        rc = IB_EALLOC;
        goto failed;
    }

    /* Create the main structure in the primary memory pool */
    *pib = (ib_engine_t *)ib_mpool_calloc(pool, 1, sizeof(**pib));
    if (*pib == NULL) {
        rc = IB_EALLOC;
        goto failed;
    }
    (*pib)->mp = pool;

    /* Create temporary memory pool */
    rc = ib_mpool_create(&((*pib)->temp_mp),
                         "temp",
                         (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create the config memory pool */
    rc = ib_mpool_create(&((*pib)->config_mp),
                         "config",
                         (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a list to hold config contexts */
    rc = ib_list_create(&((*pib)->contexts), (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create an engine config context and use it as the
     * main context until the engine can be configured.
     */
    rc = ib_context_create(*pib, NULL, IB_CTYPE_ENGINE,
                           "engine", "engine", &((*pib)->ectx));
    if (rc != IB_OK) {
        goto failed;
    }

    /* Set the context's CWD */
    rc = ib_context_set_cwd((*pib)->ectx, NULL);
    if (rc != IB_OK) {
        goto failed;
    }

    (*pib)->ctx = (*pib)->ectx;
    (*pib)->cfg_state = CFG_NOT_STARTED;

    /* Check server for ABI compatibility with this engine */
    if (server == NULL) {
        ib_log_error(*pib,  "Error in ib_create: server info required");
        rc = IB_EINVAL;
        goto failed;
    }
    if (server->vernum > IB_VERNUM) {
        ib_log_alert(*pib,
                     "Server %s (built against engine version %s) is not "
                     "compatible with this engine (version %s): "
                     "ABI %d > %d",
                     server->filename, server->version, IB_VERSION,
                     server->abinum, IB_ABINUM);
        rc = IB_EINCOMPAT;
        goto failed;
    }
    (*pib)->server = server;

    /* Sensor info. */
    (*pib)->sensor_name = IB_DSTR_UNKNOWN;
    (*pib)->sensor_version = IB_PRODUCT_VERSION_NAME;
    (*pib)->sensor_hostname = IB_DSTR_UNKNOWN;

    /* Create an array to hold loaded modules */
    /// @todo Need good defaults here
    rc = ib_array_create(&((*pib)->modules), (*pib)->mp, 16, 8);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create an array to hold filters */
    /// @todo Need good defaults here
    rc = ib_array_create(&((*pib)->filters), (*pib)->mp, 16, 8);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a hash to hold configuration directive mappings by name */
    rc = ib_hash_create_nocase(&((*pib)->dirmap), (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a hash to hold provider apis by name */
    rc = ib_hash_create_nocase(&((*pib)->apis), (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a hash to hold providers by name */
    rc = ib_hash_create_nocase(&((*pib)->providers), (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a hash to hold transformations by name */
    rc = ib_hash_create_nocase(&((*pib)->tfns), (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a hash to hold operators by name */
    rc = ib_hash_create_nocase(&((*pib)->operators), (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a hash to hold actions by name */
    rc = ib_hash_create_nocase(&((*pib)->actions), (*pib)->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Initialize the core static module. */
    /// @todo Probably want to do this in a less hard-coded manner.
    rc = ib_module_init(ib_core_module(), *pib);
    if (rc != IB_OK) {
        ib_log_alert(*pib,  "Error in ib_module_init");
        goto failed;
    }

    IB_FTRACE_RET_STATUS(rc);

failed:
    /* Make sure everything is cleaned up on failure */
    if (*pib != NULL) {
        ib_engine_pool_destroy(*pib, (*pib)->mp);
    }
    *pib = NULL;

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t ib_engine_init(ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_context_open(ib->ectx);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_context_close(ib->ectx);
    IB_FTRACE_RET_STATUS(rc);
}

/* Create a main context to operate in. */
ib_status_t ib_engine_context_create_main(ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    ib_context_t *ctx;
    ib_status_t rc;

    rc = ib_context_create(ib, ib->ectx, IB_CTYPE_MAIN, "main", "main", &ctx);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    /* Set the context's CWD */
    rc = ib_context_set_cwd(ctx, NULL);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib->ctx = ctx;

    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_engine_module_get(ib_engine_t *ib,
                                 const char * name,
                                 ib_module_t **pm)
{
    IB_FTRACE_INIT();
    size_t n;
    size_t i;
    ib_module_t *m;

    /* Return the first module matching the name. */
    IB_ARRAY_LOOP(ib->modules, n, i, m) {
        if (strcmp(name, m->name) == 0) {
            *pm = m;
            IB_FTRACE_RET_STATUS(IB_OK);
        }
    }

    *pm = NULL;

    IB_FTRACE_RET_STATUS(IB_ENOENT);
}

ib_mpool_t *ib_engine_pool_main_get(ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    IB_FTRACE_RET_PTR(ib_mpool_t, ib->mp);
}

ib_mpool_t *ib_engine_pool_config_get(ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    IB_FTRACE_RET_PTR(ib_mpool_t, ib->mp);
}

ib_mpool_t *ib_engine_pool_temp_get(ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    IB_FTRACE_RET_PTR(ib_mpool_t, ib->temp_mp);
}

void ib_engine_pool_temp_destroy(ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    ib_engine_pool_destroy(ib, ib->temp_mp);
    ib->temp_mp = NULL;
    IB_FTRACE_RET_VOID();
}

void ib_engine_pool_destroy(ib_engine_t *ib, ib_mpool_t *mp)
{
    IB_FTRACE_INIT();

    assert(ib != NULL);


    if (mp == NULL) {
        IB_FTRACE_RET_VOID();
    }

#ifdef IB_DEBUG_MEMORY
    {
        ib_status_t rc;
        char *message = NULL;
        char *path = ib_mpool_path(mp);

        if (path == NULL) {
            /* This will probably also fail... */
            ib_log_emergency(ib, "Allocation error.");
            goto finish;
        }

        rc = ib_mpool_validate(mp, &message);
        if (rc != IB_OK) {
            ib_log_error(ib, "Memory pool %s failed to validate: %s",
                path, (message ? message : "no message")
            );
        }

        if (message != NULL) {
            free(message);
            message = NULL;
        }

        message = ib_mpool_analyze(mp);
        if (message == NULL) {
            ib_log_emergency(ib, "Allocation error.");
            goto finish;
        }

        /* We use printf to coincide with the final memory debug which
         * can't be logged because it occurs too late in engine destruction.
         */
        printf("Memory Pool Analysis of %s:\n%s", path, message);

    finish:
        if (path != NULL) {
            free(path);
        }
        if (message != NULL) {
            free(message);
        }
    }
#endif

    ib_mpool_release(mp);

    IB_FTRACE_RET_VOID();
}


void ib_engine_destroy(ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    if (ib) {
        size_t ne;
        size_t idx;
        ib_list_node_t *node;
        ib_module_t *cm = ib_core_module();
        ib_module_t *m;

        /// @todo Destroy filters

        ib_log_debug3(ib, "Destroying configuration contexts...");
        IB_LIST_LOOP_REVERSE(ib->contexts, node) {
            ib_context_t *ctx = (ib_context_t *)node->data;
            if ( (ctx != ib->ctx) && (ctx != ib->ectx) ) {
                ib_context_destroy(ctx);
            }
        }
        if (ib->ctx != ib->ectx) {
            ib_log_debug3(ib, "Destroying main configuration context...");
            ib_context_destroy(ib->ctx);
            ib->ctx = NULL;
        }
        ib_log_debug3(ib, "Destroying engine configuration context...");
        ib_context_destroy(ib->ectx);
        ib->ectx = ib->ctx = NULL;

        ib_log_debug3(ib, "Unloading modules...");
        IB_ARRAY_LOOP_REVERSE(ib->modules, ne, idx, m) {
            if (m != cm) {
                ib_module_unload(m);
            }
        }

        ib_log_debug3(ib, "Destroy IB handle (%d, %d, %s, %s): %p",
                      ib->server->vernum, ib->server->abinum,
                      ib->server->filename, ib->server->name, ib);

#ifdef IB_DEBUG_MEMORY
        /* We can't use ib_engine_pool_destroy here as too little of the
         * the engine is left.
         *
         * But always output memory usage stats.
         *
         * Also can't use logging anymore.
         */
        {
            char *report = ib_mpool_analyze(ib->mp);
            if (report != NULL) {
                printf("Engine Memory Use:\n%s\n", report);
                free(report);
            }
        }
#endif
        ib_mpool_destroy(ib->mp);
    }
    IB_FTRACE_RET_VOID();
}

ib_status_t ib_conn_create(ib_engine_t *ib,
                           ib_conn_t **pconn, void *server_ctx)
{
    IB_FTRACE_INIT();
    ib_mpool_t *pool;
    ib_status_t rc;
    char namebuf[64];

    /* Create a sub-pool for each connection and allocate from it */
    /// @todo Need to tune the pool size
    rc = ib_mpool_create(&pool, "conn", ib->mp);
    if (rc != IB_OK) {
        ib_log_alert(ib,
            "Failed to create connection memory pool: %s",
            ib_status_to_string(rc)
        );
        rc = IB_EALLOC;
        goto failed;
    }
    *pconn = (ib_conn_t *)ib_mpool_calloc(pool, 1, sizeof(**pconn));
    if (*pconn == NULL) {
        ib_log_alert(ib, "Failed to allocate memory for connection");
        rc = IB_EALLOC;
        goto failed;
    }

    /* Mark time. */
    ib_clock_gettimeofday(&(*pconn)->tv_created);
    (*pconn)->t.started = ib_clock_get_time();

    /* Name the connection pool */
    snprintf(namebuf, sizeof(namebuf), "conn[%p]", (void *)(*pconn));
    ib_mpool_setname(pool, namebuf);

    (*pconn)->ib = ib;
    (*pconn)->mp = pool;
    (*pconn)->ctx = ib->ctx;
    (*pconn)->server_ctx = server_ctx;

    rc = ib_hash_create_nocase(&((*pconn)->data), (*pconn)->mp);
    if (rc != IB_OK) {
        rc = IB_EALLOC;
        goto failed;
    }

    IB_FTRACE_RET_STATUS(IB_OK);

failed:
    /* Make sure everything is cleaned up on failure */
    if (*pconn != NULL) {
        ib_engine_pool_destroy(ib, (*pconn)->mp);
    }
    *pconn = NULL;

    IB_FTRACE_RET_STATUS(rc);
}

void ib_conn_parser_context_set(ib_conn_t *conn,
                                void *parser_ctx)
{
    IB_FTRACE_INIT();
    assert(conn != NULL);

    conn->parser_ctx = parser_ctx;

    IB_FTRACE_RET_VOID();
}

void *ib_conn_parser_context_get(ib_conn_t *conn)
{
    IB_FTRACE_INIT();
    assert(conn != NULL);

    IB_FTRACE_RET_PTR(void, conn->parser_ctx);
}


ib_status_t ib_conn_data_create(ib_conn_t *conn,
                                ib_conndata_t **pconndata,
                                size_t dalloc)
{
    IB_FTRACE_INIT();
    ib_engine_t *ib = conn->ib;
    ib_mpool_t *pool;
    ib_status_t rc;

    /* Create a sub-pool for data buffers */
    rc = ib_mpool_create(&pool, "conn_data", conn->mp);
    if (rc != IB_OK) {
        ib_log_alert(ib,
            "Failed to create connection data memory pool: %s",
            ib_status_to_string(rc)
        );
        rc = IB_EALLOC;
        goto failed;
    }
    *pconndata =
        (ib_conndata_t *)ib_mpool_calloc(pool, 1, sizeof(**pconndata));
    if (*pconndata == NULL) {
        ib_log_alert(ib, "Failed to allocate memory for connection data");
        rc = IB_EALLOC;
        goto failed;
    }

    (*pconndata)->conn = conn;

    (*pconndata)->dlen = 0;
    (*pconndata)->data = (uint8_t *)ib_mpool_calloc(pool, 1, dalloc);
    if ((*pconndata)->data == NULL) {
        ib_log_alert(ib,
            "Failed to allocate memory for connection data buffer"
        );
        rc = IB_EALLOC;
        goto failed;
    }

    IB_FTRACE_RET_STATUS(IB_OK);

failed:
    /* Make sure everything is cleaned up on failure */
    *pconndata = NULL;

    IB_FTRACE_RET_STATUS(rc);
}

void ib_conn_destroy(ib_conn_t *conn)
{
    /// @todo Probably need to update state???
    if ( conn != NULL && conn->mp != NULL ) {
        ib_engine_pool_destroy(conn->ib, conn->mp);
        /* Don't do this: conn->mp = NULL; conn is now freed memory! */
    }
}

ib_status_t ib_tx_generate_id(ib_tx_t *tx,
                              ib_mpool_t *mp)
{
    IB_FTRACE_INIT();

    ib_uuid_t uuid;
    ib_status_t rc;
    char *str;

    rc = ib_uuid_create_v4(&uuid);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    /* Convert to a hex-string representation */
    str = (char *)ib_mpool_alloc(mp, IB_UUID_HEX_SIZE);
    if (str == NULL) {
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }
    tx->id = str;

    rc = ib_uuid_bin_to_ascii(str, &uuid);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_tx_create(ib_tx_t **ptx,
                         ib_conn_t *conn,
                         void *sctx)
{
    IB_FTRACE_INIT();
    ib_mpool_t *pool;
    ib_status_t rc;
    char namebuf[64];
    ib_tx_t *tx = NULL;
    ib_core_cfg_t *corecfg;

    ib_engine_t *ib = conn->ib;

    rc = ib_context_module_config(
        ib->ctx,
        ib_core_module(),
        (void *)&corecfg
    );

    if (rc != IB_OK) {
        ib_log_alert(ib, "Failed to retrieve core module configuration.");
    }

    assert(corecfg != NULL);

    /* Create a sub-pool from the connection memory pool for each
     * transaction and allocate from it
     */
    rc = ib_mpool_create(&pool, "tx", conn->mp);
    if (rc != IB_OK) {
        ib_log_alert(ib,
            "Failed to create transaction memory pool: %s",
            ib_status_to_string(rc)
        );
        rc = IB_EALLOC;
        goto failed;
    }
    tx = (ib_tx_t *)ib_mpool_calloc(pool, 1, sizeof(*tx));
    if (tx == NULL) {
        ib_log_alert(ib, "Failed to allocate memory for transaction");
        rc = IB_EALLOC;
        goto failed;
    }

    /* Name the transaction pool */
    snprintf(namebuf, sizeof(namebuf), "tx[%p]", (void *)tx);
    ib_mpool_setname(pool, namebuf);

    /* Mark time. */
    ib_clock_gettimeofday(&tx->tv_created);
    tx->t.started = ib_clock_get_time();

    tx->ib = ib;
    tx->mp = pool;
    tx->ctx = ib->ctx;
    tx->sctx = sctx;
    tx->conn = conn;
    tx->er_ipstr = conn->remote_ipstr;
    tx->hostname = IB_DSTR_EMPTY;
    tx->path = IB_DSTR_URI_ROOT_PATH;
    tx->block_status = corecfg->block_status;

    ++conn->tx_count;
    ib_tx_generate_id(tx, tx->mp);

    /* Create the generic data store. */
    rc = ib_hash_create_nocase(&(tx->data), tx->mp);
    if (rc != IB_OK) {
        rc = IB_EALLOC;
        goto failed;
    }

    /* Create a filter controller. */
    rc = ib_fctl_tx_create(&(tx->fctl), tx, tx->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create the body buffers. */
    rc = ib_stream_create(&tx->request_body, tx->mp);
    if (rc != IB_OK) {
        goto failed;
    }
    rc = ib_stream_create(&tx->response_body, tx->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /**
     * After this, we have generally succeeded and are now outputting
     * the transaction to the conn object and the ptx pointer.
     */

    /* Add transaction to the connection list */
    if (conn->tx_first == NULL) {
        conn->tx_first = tx;
        conn->tx = tx;
        conn->tx_last = tx;
        ib_log_debug3_tx(tx, "First transaction: %p", tx);
    }
    else {
        conn->tx = tx;
        conn->tx_last->next = tx;
        conn->tx_last = tx;

        /* If there are more than one transactions, then this is a pipeline
         * request and needs to be marked as such.
         */
        if (conn->tx_first->next == tx) {
            ib_tx_flags_set(conn->tx_first, IB_TX_FPIPELINED);
        }
        ib_tx_flags_set(tx, IB_TX_FPIPELINED);

        ib_log_debug3_tx(tx, "Found a pipelined transaction: %p", tx);
    }

    /* Only when we are successful, commit changes to output variable. */
    *ptx = tx;
    ib_log_debug3_tx(tx, "TX CREATE p=%p id=%s", tx, tx->id);

    IB_FTRACE_RET_STATUS(IB_OK);

failed:
    /* Make sure everything is cleaned up on failure */
    if (tx != NULL) {
        ib_engine_pool_destroy(ib, tx->mp);
    }
    tx = NULL;

    IB_FTRACE_RET_STATUS(rc);
}

void ib_tx_destroy(ib_tx_t *tx)
{
    /// @todo It should always be the first one in the list,
    ///       so this should not be needed and should cause an error
    ///       or maybe for us to throw a flag???
    assert(tx != NULL);
    assert(tx->ib != NULL);
    assert(tx->conn != NULL);
    assert(tx->conn->tx_first == tx);

    ib_engine_t *ib = tx->ib;
    ib_tx_t *curr;

    ib_log_debug3_tx(tx, "TX DESTROY p=%p id=%s", tx, tx->id);

    /* Make sure that the post processing state was notified. */
    if (! ib_tx_flags_isset(tx, IB_TX_FPOSTPROCESS)) {
        ib_log_info_tx(tx,
                       "Forcing engine to run post processing "
                       "prior to destroying transaction.");
        ib_state_notify_postprocess(ib, tx);
    }

    /* Keep track of the first/current tx. */
    tx->conn->tx_first = tx->next;
    tx->conn->tx = tx->next;

    for (curr = tx->conn->tx_first; curr != NULL; curr = curr->next) {
        if (curr == tx) {
            curr->next = curr->next ? curr->next->next : NULL;
            break;
        }
    }

    /* Keep track of the last tx. */
    if (tx->conn->tx_last == tx) {
        tx->conn->tx_last = NULL;
    }

    /// @todo Probably need to update state???
    ib_engine_pool_destroy(tx->ib, tx->mp);
}

/* -- State Routines -- */

/**
 * List of state names for id to name lookups.
 *
 * @warning Remember to update ib_state_event_type_t.
 *
 */
static const char *ib_state_event_name_list[] = {
    /* Engine States */
    IB_STRINGIFY(conn_started_event),
    IB_STRINGIFY(conn_finished_event),
    IB_STRINGIFY(tx_started_event),
    IB_STRINGIFY(tx_process_event),
    IB_STRINGIFY(tx_finished_event),

    /* Handler States */
    IB_STRINGIFY(handle_context_conn_event),
    IB_STRINGIFY(handle_connect_event),
    IB_STRINGIFY(handle_context_tx_event),
    IB_STRINGIFY(handle_request_header_event),
    IB_STRINGIFY(handle_request_event),
    IB_STRINGIFY(handle_response_header_event),
    IB_STRINGIFY(handle_response_event),
    IB_STRINGIFY(handle_disconnect_event),
    IB_STRINGIFY(handle_postprocess_event),

    /* Server States */
    IB_STRINGIFY(cfg_started_event),
    IB_STRINGIFY(cfg_finished_event),
    IB_STRINGIFY(conn_opened_event),
    IB_STRINGIFY(conn_data_in_event),
    IB_STRINGIFY(conn_data_out_event),
    IB_STRINGIFY(conn_closed_event),

    /* Parser States */
    IB_STRINGIFY(request_started_event),
    IB_STRINGIFY(request_header_data_event),
    IB_STRINGIFY(request_header_finished_event),
    IB_STRINGIFY(request_body_data_event),
    IB_STRINGIFY(request_finished_event),
    IB_STRINGIFY(response_started_event),
    IB_STRINGIFY(response_header_data_event),
    IB_STRINGIFY(response_header_finished_event),
    IB_STRINGIFY(response_body_data_event),
    IB_STRINGIFY(response_finished_event),

    NULL
};

const char *ib_state_event_name(ib_state_event_type_t event)
{
    IB_FTRACE_INIT();
    IB_FTRACE_RET_CONSTSTR(ib_state_event_name_list[event]);
}
/* -- Hook Routines -- */

ib_state_hook_type_t ib_state_hook_type(ib_state_event_type_t event)
{
    static const size_t num_events =
        sizeof(ib_state_event_hook_types) / sizeof(ib_state_hook_type_t);

    if (event > num_events) {
        return IB_STATE_HOOK_INVALID;
    }

    return ib_state_event_hook_types[event];
}

ib_status_t DLL_PUBLIC ib_hook_null_register(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_null_hook_fn_t cb,
    void *cdata
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_NULL);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib_hook_t *hook = (ib_hook_t *)ib_mpool_alloc(ib->mp, sizeof(*hook));
    if (hook == NULL) {
        ib_log_emergency(ib, "Error in ib_mpool_calloc");
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }

    hook->callback.null = cb;
    hook->cdata = cdata;
    hook->next = NULL;

    rc = ib_register_hook(ib, event, hook);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_null_hook_unregister(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_null_hook_fn_t cb
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_NULL);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_unregister_hook(ib, event, (ib_void_fn_t)cb);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_conn_register(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_conn_hook_fn_t cb,
    void *cdata
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_CONN);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib_hook_t *hook = (ib_hook_t *)ib_mpool_alloc(ib->mp, sizeof(*hook));
    if (hook == NULL) {
        ib_log_emergency(ib, "Error in ib_mpool_calloc");
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }

    hook->callback.conn = cb;
    hook->cdata = cdata;
    hook->next = NULL;

    rc = ib_register_hook(ib, event, hook);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_conn_hook_unregister(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_conn_hook_fn_t cb
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_CONN);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_unregister_hook(ib, event, (ib_void_fn_t)cb);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_conndata_register(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_conndata_hook_fn_t cb,
    void *cdata
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_CONNDATA);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib_hook_t *hook = (ib_hook_t *)ib_mpool_alloc(ib->mp, sizeof(*hook));
    if (hook == NULL) {
        ib_log_emergency(ib, "Error in ib_mpool_calloc");
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }

    hook->callback.conndata = cb;
    hook->cdata = cdata;
    hook->next = NULL;

    rc = ib_register_hook(ib, event, hook);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_conndata_hook_unregister(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_conndata_hook_fn_t cb
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_CONNDATA);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_unregister_hook(ib, event, (ib_void_fn_t)cb);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_tx_register(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_tx_hook_fn_t cb,
    void *cdata
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_TX);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib_hook_t *hook = (ib_hook_t *)ib_mpool_alloc(ib->mp, sizeof(*hook));
    if (hook == NULL) {
        ib_log_emergency(ib, "Error in ib_mpool_calloc");
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }

    hook->callback.tx = cb;
    hook->cdata = cdata;
    hook->next = NULL;

    rc = ib_register_hook(ib, event, hook);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_tx_hook_unregister(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_tx_hook_fn_t cb
) {
    IB_FTRACE_INIT();

    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_TX);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_unregister_hook(ib, event, (ib_void_fn_t)cb);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_txdata_register(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_txdata_hook_fn_t cb,
    void *cdata
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_TXDATA);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib_hook_t *hook = (ib_hook_t *)ib_mpool_alloc(ib->mp, sizeof(*hook));
    if (hook == NULL) {
        ib_log_emergency(ib, "Error in ib_mpool_calloc");
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }

    hook->callback.txdata = cb;
    hook->cdata = cdata;
    hook->next = NULL;

    rc = ib_register_hook(ib, event, hook);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_txdata_hook_unregister(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_txdata_hook_fn_t cb
) {
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_TXDATA);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_unregister_hook(ib, event, (ib_void_fn_t)cb);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_parsed_header_data_register(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_header_data_fn_t cb,
    void *cdata)
{
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_HEADER);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib_hook_t *hook = (ib_hook_t *)ib_mpool_alloc(ib->mp, sizeof(*hook));
    if (hook == NULL) {
        ib_log_emergency(ib, "Error in ib_mpool_calloc");
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }

    hook->callback.headerdata = cb;
    hook->cdata = cdata;
    hook->next = NULL;

    rc = ib_register_hook(ib, event, hook);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_parsed_header_data_unregister(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_header_data_fn_t cb)
{
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_HEADER);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_unregister_hook(ib, event, (ib_void_fn_t)cb);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_parsed_req_line_register(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_request_line_fn_t cb,
    void *cdata)
{
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_REQLINE);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib_hook_t *hook = (ib_hook_t *)ib_mpool_alloc(ib->mp, sizeof(*hook));
    if (hook == NULL) {
        ib_log_emergency(ib, "Error in ib_mpool_calloc");
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }

    hook->callback.requestline = cb;
    hook->cdata = cdata;
    hook->next = NULL;

    rc = ib_register_hook(ib, event, hook);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_parsed_req_line_unregister(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_request_line_fn_t cb)
{
    IB_FTRACE_INIT();
    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_REQLINE);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_unregister_hook(ib, event, (ib_void_fn_t)cb);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_parsed_resp_line_register(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_response_line_fn_t cb,
    void *cdata)
{
    IB_FTRACE_INIT();

    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_RESPLINE);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    ib_hook_t *hook = (ib_hook_t *)ib_mpool_alloc(ib->mp, sizeof(*hook));
    if (hook == NULL) {
        ib_log_emergency(ib, "Error in ib_mpool_calloc");
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }

    hook->callback.responseline = cb;
    hook->cdata = cdata;
    hook->next = NULL;

    rc = ib_register_hook(ib, event, hook);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t DLL_PUBLIC ib_hook_parsed_resp_line_unregister(
    ib_engine_t *ib,
    ib_state_event_type_t event,
    ib_state_response_line_fn_t cb)
{
    IB_FTRACE_INIT();

    ib_status_t rc;

    rc = ib_check_hook(ib, event, IB_STATE_HOOK_RESPLINE);
    if (rc != IB_OK) {
        IB_FTRACE_RET_STATUS(rc);
    }

    rc = ib_unregister_hook(ib, event, (ib_void_fn_t)cb);

    IB_FTRACE_RET_STATUS(rc);
}


/* -- Connection Handling -- */

/* -- Transaction Handling -- */

/* -- Configuration Contexts -- */

const ib_list_t *ib_context_get_all(const ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    assert(ib != NULL);

    IB_FTRACE_RET_PTR(const ib_list_t, ib->contexts);
}

ib_status_t ib_context_create(ib_engine_t *ib,
                              ib_context_t *parent,
                              ib_ctype_t ctype,
                              const char *ctx_type,
                              const char *ctx_name,
                              ib_context_t **pctx)
{
    IB_FTRACE_INIT();
    ib_mpool_t *ppool;
    ib_mpool_t *pool;
    ib_status_t rc;
    ib_context_t *ctx = NULL;
    char *full;
    size_t full_len;

    /* Create memory subpool */
    ppool = (parent == NULL) ? ib->mp : parent->mp;
    rc = ib_mpool_create(&pool, "context", ppool);
    if (rc != IB_OK) {
        rc = IB_EALLOC;
        goto failed;
    }

    /* Create the main structure */
    ctx = (ib_context_t *)ib_mpool_calloc(pool, 1, sizeof(*ctx));
    if (ctx == NULL) {
        rc = IB_EALLOC;
        goto failed;
    }

    ctx->ib = ib;
    ctx->mp = pool;
    ctx->parent = parent;
    ctx->ctype = ctype;
    ctx->ctx_type = ctx_type;
    ctx->ctx_name = ctx_name;
    ctx->state = CTX_CREATED;

    /* Generate the full name of the context */
    full_len = 2;
    if (parent != NULL) {
        full_len += strlen(parent->ctx_name) + 1;
    }
    if (ctx_type != NULL) {
        full_len += strlen(ctx_type);
    }
    if (ctx_name != NULL) {
        full_len += strlen(ctx_name);
    }
    full = (char *)ib_mpool_alloc(pool, full_len);
    if (full == NULL) {
        rc = IB_EALLOC;
        goto failed;
    }
    *full = '\0';
    if (parent != NULL) {
        strcat(full, parent->ctx_name);
        strcat(full, ":");
    }
    if (ctx_type != NULL) {
        strcat(full, ctx_type);
    }
    strcat(full, ":");
    if (ctx_name != NULL) {
        strcat(full, ctx_name);
    }
    ctx->ctx_full = full;

    /* Create a cfgmap to hold the configuration */
    rc = ib_cfgmap_create(&(ctx->cfg), ctx->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create an array to hold the module config data */
    rc = ib_array_create(&(ctx->cfgdata), ctx->mp, 16, 8);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a list to hold the enabled filters */
    rc = ib_list_create(&(ctx->filters), ctx->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a list to hold child contexts */
    rc = ib_list_create(&(ctx->children), ctx->mp);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Create a hash to hold the module-specific data */
    rc = ib_array_create(&(ctx->cfgdata), ctx->mp, 16, 8);
    if (rc != IB_OK) {
        goto failed;
    }

    /* Add myself to my parent's child list */
    if (parent != NULL) {
        rc = ib_list_push(parent->children, ctx);
        if (rc != IB_OK) {
            goto failed;
        }
    }

    /* Add to the engine's list of all contexts */
    rc = ib_list_push(ib->contexts, ctx);
    if (rc != IB_OK) {
        goto failed;
    }

    if (parent != NULL) {
        rc = ib_context_set_auditlog_index(
            ctx,
            parent->auditlog->index_enabled,
            parent->auditlog->index_default ? NULL : parent->auditlog->index);
    }
    else {
        rc = ib_context_set_auditlog_index(ctx, true, NULL);
    }
    if (rc != IB_OK) {
        goto failed;
    }

    /* Register the modules */
    /// @todo Later on this needs to be triggered by ActivateModule or similar
    if (ib->modules) {
        ib_module_t *m;
        size_t n;
        size_t i;
        IB_ARRAY_LOOP(ib->modules, n, i, m) {
            ib_log_debug3(ib, "Registering module=\"%s\" idx=%zd",
                          m->name, m->idx);
            rc = ib_module_register_context(m, ctx);
            if (rc != IB_OK) {
                goto failed;
            }
        }
    }
    else {
        /* Register the core module by default. */
        rc = ib_module_register_context(ib_core_module(), ctx);
        if (rc != IB_OK) {
            goto failed;
        }
    }

    /* Commit the new ctx to pctx. */
    *pctx = ctx;

    IB_FTRACE_RET_STATUS(IB_OK);


failed:
    /* Make sure everything is cleaned up on failure */
    if (ctx != NULL) {
        ib_engine_pool_destroy(ib, ctx->mp);
    }

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t ib_context_open(ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    ib_engine_t *ib = ctx->ib;
    ib_context_data_t *cfgdata;
    ib_status_t rc;
    size_t ncfgdata;
    size_t i;

    if (ctx->state != CTX_CREATED) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }
    ib_log_debug3(ib, "Opening context ctx=%p '%s'", ctx, ctx->ctx_full);

    IB_ARRAY_LOOP(ctx->cfgdata, ncfgdata, i, cfgdata) {
        if (cfgdata == NULL) {
            continue;
        }
        ib_module_t *m = cfgdata->module;

        if (m->fn_ctx_open != NULL) {
            rc = m->fn_ctx_open(ib, m, ctx, m->cbdata_ctx_open);
            if (rc != IB_OK) {
                /// @todo Log the error???  Fail???
                ib_log_error(ib, "Failed to call context open: %s",
                             ib_status_to_string(rc));
                IB_FTRACE_RET_STATUS(rc);
            }
        }
    }

    ctx->state = CTX_OPEN;
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_context_set_cwd(ib_context_t *ctx, const char *dir)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    /* For special cases (i.e. tests), allow handle NULL directory */
    if (dir == NULL) {
        char *buf = (char *)ib_mpool_alloc(ctx->mp, maxpath);
        if (buf == NULL) {
            IB_FTRACE_RET_STATUS(IB_EALLOC);
        }
        ctx->ctx_cwd = getcwd(buf, maxpath);
        if (ctx->ctx_cwd == NULL) {
            IB_FTRACE_RET_STATUS(IB_EALLOC);
        }
        IB_FTRACE_RET_STATUS(IB_OK);
    }

    /* Copy it */
    ctx->ctx_cwd = ib_mpool_strdup(ctx->mp, dir);
    if (ctx->ctx_cwd == NULL) {
        IB_FTRACE_RET_STATUS(IB_EALLOC);
    }
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_context_config_set_parser(ib_context_t *ctx,
                                         const ib_cfgparser_t *parser)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    ctx->cfgparser = parser;
    if (parser == NULL) {
        IB_FTRACE_RET_STATUS(IB_OK);
    }
    IB_FTRACE_RET_STATUS(ib_context_set_cwd(ctx, parser->cur_cwd));
}

ib_status_t ib_context_config_get_parser(const ib_context_t *ctx,
                                         const ib_cfgparser_t **pparser)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);
    assert(pparser != NULL);

    *pparser = ctx->cfgparser;
    IB_FTRACE_RET_STATUS(IB_OK);
}

const char *ib_context_config_cwd(const ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    if (ctx->cfgparser == NULL) {
        IB_FTRACE_RET_CONSTSTR(ctx->ctx_cwd);
    }
    else {
        IB_FTRACE_RET_CONSTSTR(ctx->cfgparser->cur_cwd);
    }
}

ib_status_t ib_context_set_auditlog_index(ib_context_t *ctx,
                                          bool enable,
                                          const char *idx)
{
    IB_FTRACE_INIT();

    ib_status_t rc;

    assert(ctx != NULL);
    assert(ctx->ib != NULL);
    assert(ctx->mp != NULL);

    /* Check if a new audit log structure must be allocated:
     *   1. if auditlog == NULL or
     *   2. if the allocated audit log belongs to another context we may
     *      not change its auditlog->index value (or auditlog->index_fp).
     *      We must make a new auditlog that the passed in ib_context_t
     *      ctx owns.  */
    if (ctx->auditlog == NULL || ctx->auditlog->owner != ctx) {

        ctx->auditlog = (ib_auditlog_cfg_t *)
            ib_mpool_calloc(ctx->mp, 1, sizeof(*ctx->auditlog));

        if (ctx->auditlog == NULL) {
            IB_FTRACE_RET_STATUS(IB_EALLOC);
        }

        /* Set owner. */
        ctx->auditlog->owner = ctx;

        /* Set index_fp_lock. */
        if (enable == true) {
            rc = ib_lock_init(&ctx->auditlog->index_fp_lock);
            if (rc != IB_OK) {
                ib_log_debug2(ctx->ib,
                              "Failed to initialize lock "
                              "for audit index %s: %s",
                              idx, ib_status_to_string(rc));
                IB_FTRACE_RET_STATUS(rc);
            }

            /* Set index. */
            if (idx == NULL) {
                ctx->auditlog->index_default = true;
                idx = default_auditlog_index;
            }
            else {
                ctx->auditlog->index_default = false;
            }
            ctx->auditlog->index = ib_mpool_strdup(ctx->mp, idx);
            if (ctx->auditlog->index == NULL) {
                IB_FTRACE_RET_STATUS(IB_EALLOC);
            }
            ctx->auditlog->index_enabled = true;
        }
        else {
            ctx->auditlog->index = NULL;
            ctx->auditlog->index_fp = NULL;
        }
    }
    /* Else the auditlog struct is initialized and owned by this ctx. */
    else {
        bool unlock = false;
        if (ctx->auditlog->index_enabled == true) {
            const char *cidx = ctx->auditlog->index; /* Current index */

            rc = ib_lock_lock(&ctx->auditlog->index_fp_lock);
            if (rc != IB_OK) {
                ib_log_debug2(ctx->ib, "Failed lock to audit index %s",
                              ctx->auditlog->index);
                IB_FTRACE_RET_STATUS(rc);
            }
            unlock = true;

            /* Check that we aren't re-setting a value in the same context. */
            if ( (enable && ctx->auditlog->index_enabled) &&
                 ( ((idx == NULL) && ctx->auditlog->index_default) ||
                   ((idx != NULL) && (cidx != NULL) &&
                    (strcmp(idx, cidx) == 0)) ) )
            {
                if (unlock) {
                    ib_lock_unlock(&ctx->auditlog->index_fp_lock);
                }
                ib_log_debug2(ctx->ib,
                              "Re-setting log same value. No action: %s",
                              idx);

                IB_FTRACE_RET_STATUS(IB_OK);
            }
        }

        /* Replace the old index value with the new index value. */
        if (enable == false) {
            ctx->auditlog->index_enabled = false;
            ctx->auditlog->index_default = false;
            ctx->auditlog->index = NULL;
        }
        else {
            if (idx == NULL) {
                idx = default_auditlog_index;
                ctx->auditlog->index_default = true;
            }
            else {
                ctx->auditlog->index_default = false;
            }
            ctx->auditlog->index = ib_mpool_strdup(ctx->mp, idx);
            if (ctx->auditlog->index == NULL) {
                if (unlock) {
                    ib_lock_unlock(&ctx->auditlog->index_fp_lock);
                }
                IB_FTRACE_RET_STATUS(IB_EALLOC);
            }
            ctx->auditlog->index_enabled = true;
        }

        /* Close the audit log file if it is open. */
        if (ctx->auditlog->index_fp != NULL) {
            fclose(ctx->auditlog->index_fp);
            ctx->auditlog->index_fp = NULL;
        }

        if (unlock) {
            ib_lock_unlock(&ctx->auditlog->index_fp_lock);
        }
    }

    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_engine_cfg_finished(ib_engine_t *ib)
{
    IB_FTRACE_INIT();
    ib_status_t rc;
    ib_list_node_t *node;

    /* Clear the configuration parsers for all contexts */
    IB_LIST_LOOP(ib->contexts, node) {
        ib_context_t *ctx = (ib_context_t *)node->data;
        assert(ctx != NULL);

        rc = ib_context_config_set_parser(ctx, NULL);
        if (rc != IB_OK) {
            IB_FTRACE_RET_STATUS(rc);
        }
    }
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_context_close(ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    ib_engine_t *ib = ctx->ib;
    ib_context_data_t *cfgdata;
    ib_status_t rc;
    size_t ncfgdata;
    size_t i;

    if (ctx->state != CTX_OPEN) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }
    ib_log_debug3(ib, "Closing context ctx=%p '%s'", ctx, ctx->ctx_full);

    IB_ARRAY_LOOP(ctx->cfgdata, ncfgdata, i, cfgdata) {
        if (cfgdata == NULL) {
            continue;
        }
        ib_module_t *m = cfgdata->module;

        if (m->fn_ctx_close != NULL) {
            rc = m->fn_ctx_close(ib, m, ctx, m->cbdata_ctx_close);
            if (rc != IB_OK) {
                /// @todo Log the error???  Fail???
                ib_log_error(ib,
                             "Failed to call context close: %s",
                             ib_status_to_string(rc)
                );
                IB_FTRACE_RET_STATUS(rc);
            }
        }
    }

    ctx->state = CTX_CLOSED;
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_context_site_set(ib_context_t *ctx,
                                const ib_site_t *site)
{
    IB_FTRACE_INIT();

    assert(ctx != NULL);

    if (ctx->state == CTX_CLOSED) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }
    if ( (ctx->ctype != IB_CTYPE_SITE) && (ctx->ctype != IB_CTYPE_LOCATION) ) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }

    ctx->site = site;
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_context_site_get(const ib_context_t *ctx,
                                const ib_site_t **psite)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);
    assert(psite != NULL);

    if ( (ctx->ctype != IB_CTYPE_SITE) && (ctx->ctype != IB_CTYPE_LOCATION) ) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }

    *psite = ctx->site;
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_context_location_set(ib_context_t *ctx,
                                    const ib_site_location_t *location)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    if (ctx->state == CTX_CLOSED) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }
    if (ctx->ctype != IB_CTYPE_LOCATION) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }

    ctx->location = location;
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_context_location_get(const ib_context_t *ctx,
                                    const ib_site_location_t **plocation)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);
    assert(plocation != NULL);

    if (ctx->ctype != IB_CTYPE_LOCATION) {
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }

    *plocation = ctx->location;
    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_context_t *ib_context_parent_get(const ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    IB_FTRACE_RET_PTR(ib_context_t, ctx->parent);
}

void ib_context_parent_set(ib_context_t *ctx,
                           ib_context_t *parent)
{
    IB_FTRACE_INIT();
    ctx->parent = parent;
    IB_FTRACE_RET_VOID();
}

ib_ctype_t ib_context_type(const ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    IB_FTRACE_RET_UINT(ctx->ctype);
}

bool ib_context_type_check(const ib_context_t *ctx, ib_ctype_t ctype)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    IB_FTRACE_RET_BOOL(ctx->ctype == ctype);
}

const char *ib_context_type_get(const ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    if (ctx->ctx_type == NULL) {
        IB_FTRACE_RET_CONSTSTR("");
    }
    else {
        IB_FTRACE_RET_CONSTSTR(ctx->ctx_type);
    }
}

const char *ib_context_name_get(const ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    if (ctx->ctx_name == NULL) {
        IB_FTRACE_RET_CONSTSTR("");
    }
    else {
        IB_FTRACE_RET_CONSTSTR(ctx->ctx_name);
    }
}

const char *ib_context_full_get(const ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    assert(ctx != NULL);

    IB_FTRACE_RET_CONSTSTR(ctx->ctx_full);
}

void ib_context_destroy(ib_context_t *ctx)
{
    IB_FTRACE_INIT();
    ib_engine_t *ib;
    ib_context_data_t *cfgdata;
    ib_status_t rc;
    size_t ncfgdata, i;

    if (ctx == NULL) {
        IB_FTRACE_RET_VOID();
    }

    ib = ctx->ib;

    ib_log_debug3(ib, "Destroying context ctx=%p '%s'", ctx, ctx->ctx_full);

    /* Run through the context modules to call any ctx_fini functions. */
    /// @todo Not sure this is needed anymore
    IB_ARRAY_LOOP(ctx->cfgdata, ncfgdata, i, cfgdata) {
        if (cfgdata == NULL) {
            continue;
        }
        ib_module_t *m = cfgdata->module;

        if (m->fn_ctx_destroy != NULL) {
            ib_log_debug3(ib,
                          "Finishing context ctx=%p '%s' for module=%s (%p)",
                          ctx, ctx->ctx_full, m->name, m);
            rc = m->fn_ctx_destroy(ib, m, ctx, m->cbdata_ctx_destroy);
            if (rc != IB_OK) {
                /// @todo Log the error???  Fail???
                ib_log_error(ib,
                    "Failed to call context fini: %s",
                    ib_status_to_string(rc)
                );
            }
        }
    }

    ib_engine_pool_destroy(ib, ctx->mp);

    IB_FTRACE_RET_VOID();
}

ib_context_t *ib_context_engine(const ib_engine_t *ib)
{
    return ib->ectx;
}

ib_context_t *ib_context_main(const ib_engine_t *ib)
{
    return ib->ctx;
}

ib_engine_t *ib_context_get_engine(const ib_context_t *ctx)
{
    return ctx->ib;
}

ib_status_t ib_context_init_cfg(ib_context_t *ctx,
                                void *base,
                                const ib_cfgmap_init_t *init)
{
    IB_FTRACE_INIT();
    ib_status_t rc;

    ib_log_debug3(ctx->ib, "Initializing context %s base=%p",
                  ib_context_full_get(ctx), base);

    if (init == NULL) {
        IB_FTRACE_RET_STATUS(IB_OK);
    }

    rc = ib_cfgmap_init(ctx->cfg, base, init);

    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t ib_context_module_config(ib_context_t *ctx,
                                     ib_module_t *m,
                                     void *pcfg)
{
    IB_FTRACE_INIT();
    ib_context_data_t *cfgdata;
    ib_status_t rc;

    rc = ib_array_get(ctx->cfgdata, m->idx, (void *)&cfgdata);
    if (rc != IB_OK) {
        *(void **)pcfg = NULL;
        IB_FTRACE_RET_STATUS(rc);
    }

    if (cfgdata == NULL) {
        *(void **)pcfg = NULL;
        IB_FTRACE_RET_STATUS(IB_EINVAL);
    }

    *(void **)pcfg = cfgdata->data;

    IB_FTRACE_RET_STATUS(IB_OK);
}

ib_status_t ib_context_set(ib_context_t *ctx,
                           const char *name,
                           void *val)
{
    IB_FTRACE_INIT();
    ib_status_t rc = ib_cfgmap_set(ctx->cfg, name, val);
    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t ib_context_set_num(ib_context_t *ctx,
                               const char *name,
                               ib_num_t val)
{
    IB_FTRACE_INIT();
    ib_status_t rc = ib_cfgmap_set(ctx->cfg, name, ib_ftype_num_in(&val));
    IB_FTRACE_RET_STATUS(rc);
}

ib_status_t ib_context_set_string(ib_context_t *ctx,
                                  const char *name,
                                  const char *val)
{
    IB_FTRACE_INIT();
    ib_status_t rc = ib_cfgmap_set(ctx->cfg, name, ib_ftype_nulstr_in(val));
    IB_FTRACE_RET_STATUS(rc);
}


ib_status_t ib_context_get(ib_context_t *ctx,
                           const char *name,
                           void *pval, ib_ftype_t *ptype)
{
    IB_FTRACE_INIT();
    ib_status_t rc = ib_cfgmap_get(ctx->cfg, name, pval, ptype);
    IB_FTRACE_RET_STATUS(rc);
}
