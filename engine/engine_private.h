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

#ifndef _IB_ENGINE_PRIVATE_H_
#define _IB_ENGINE_PRIVATE_H_

/**
 * @file
 * @brief IronBee --- Engine Private Declarations
 *
 * @author Brian Rectanus <brectanus@qualys.com>
 */

#include "state_notify_private.h"

#include <ironbee/array.h>
#include <ironbee/lock.h>

#include <stdio.h>

/**
 * Per-context audit log configuration.
 *
 * This struct is associated with an owning context by the ib_context_t*
 * member named "owner."
 * Only the owner context may destroy or edit the logging context.
 * Child contexts that copy from the parent context may have a copy of
 * the pointer to this struct, but may not edit its context.
 *
 * Child contexts may, though, lock the index_fp_lock field and write to
 * the index_fp.
 *
 * The owning context should lock index_fp_lock before updating lock_fp and
 * index.
 */
typedef struct ib_auditlog_cfg_t ib_auditlog_cfg_t;
struct ib_auditlog_cfg_t {
    bool          index_enabled; /**< Index file enabled? */
    bool          index_default; /**< Index file is default? */
    char         *index;         /**< Index file name. */
    FILE         *index_fp;      /**< Index file pointer. */
    ib_lock_t     index_fp_lock; /**< Lock to protect index_fp. */
    ib_context_t *owner;         /**< Owning context. Only owner should edit. */
};

/**
 * Rule engine data
 */
typedef struct ib_rule_engine_t ib_rule_engine_t;

/**
 * Rule engine per-context data
 */
typedef struct ib_rule_context_t ib_rule_context_t;

/**
 * Engine configuration state
 */
typedef enum {
    CFG_NOT_STARTED,
    CFG_STARTED,
    CFG_FINISHED
} ib_engine_cfg_state_t;

/**
 * Configuration Context Selection Data
 */
struct ib_context_select_t {
    const ib_module_t            *module;
    ib_context_select_fn_t        select_fn;
    ib_context_get_site_fn_t      get_site_fn;
    ib_context_get_location_fn_t  get_location_fn;
};
typedef struct ib_context_select_t ib_context_select_t;

/**
 * Engine handle.
 */
struct ib_engine_t {
    ib_mpool_t         *mp;               /**< Primary memory pool */
    ib_mpool_t         *config_mp;        /**< Config memory pool */
    ib_mpool_t         *temp_mp;          /**< Temp memory pool for config */
    ib_provider_inst_t *dpi;              /**< Data provider instance */
    ib_context_t       *ectx;             /**< Engine configuration context */
    ib_context_t       *ctx;              /**< Main configuration context */
    ib_engine_cfg_state_t cfg_state;      /**< Engine configuration state */
    ib_uuid_t           sensor_id;        /**< Sensor UUID */
    uint32_t            sensor_id_hash;   /**< Sensor UUID hash (4 bytes) */
    const char         *sensor_id_str;    /**< ascii format, for logging */
    const char         *sensor_name;      /**< Sensor name */
    const char         *sensor_version;   /**< Sensor version string */
    const char         *sensor_hostname;  /**< Sensor hostname */

    /// @todo Only these should be private
    ib_server_t        *server;           /**< Info about the server */
    ib_array_t         *modules;          /**< Array tracking modules */
    ib_array_t         *filters;          /**< Array tracking filters */
    ib_list_t          *contexts;         /**< Configuration contexts */
    ib_hash_t          *dirmap;           /**< Hash tracking directive map */
    ib_hash_t          *apis;             /**< Hash tracking provider APIs */
    ib_hash_t          *providers;        /**< Hash tracking providers */
    ib_hash_t          *tfns;             /**< Hash tracking transforms */
    ib_hash_t          *operators;        /**< Hash tracking operators */
    ib_hash_t          *actions;          /**< Hash tracking rules */
    ib_rule_engine_t   *rule_engine;      /**< Rule engine data */

    /* Hooks */
    ib_hook_t *hook[IB_STATE_EVENT_NUM + 1]; /**< Registered hook callbacks */

    /* Context selection functions */
    ib_context_select_t act_ctx_select;   /**< Active context selection */
    ib_context_select_t core_ctx_select;  /**< Core context selection */
};

/**
 * Configuration context data.
 */
typedef struct ib_context_data_t ib_context_data_t;
struct ib_context_data_t {
    ib_module_t          *module;      /**< Module handle */
    void                 *data;        /**< Module config structure */
};

/**
 * Configuration context.
 */
struct ib_context_t {
    ib_engine_t          *ib;          /**< Engine */
    ib_mpool_t           *mp;          /**< Memory pool */
    ib_cfgmap_t          *cfg;         /**< Config map */
    ib_array_t           *cfgdata;     /**< Config data */
    ib_context_t         *parent;      /**< Parent context */
    ib_list_t            *children;    /**< Child contexts */
    ib_ctype_t            ctype;       /**< Context type */
    const char           *ctx_type;    /**< Type identifier string. */
    const char           *ctx_name;    /**< Name identifier string. */
    const char           *ctx_full;    /**< Full name of context */
    const char           *ctx_cwd;     /**< Context's current directory */
    ib_auditlog_cfg_t    *auditlog;    /**< Per-context audit log cfgs. */
    const ib_cfgparser_t *cfgparser;   /**< Our configuration parser */
    bool                  is_open;     /**< True when context is open */

    /* Data specific to the selector for this context */
    void                 *selection_data; /**< Selector specific data */

    /* Filters */
    ib_list_t            *filters;     /**< Context enabled filters */

    /* Rules associated with this context */
    ib_rule_context_t    *rules;       /**< Rule context data */
};

#endif /* _IB_ENGINE_PRIVATE_H_ */
