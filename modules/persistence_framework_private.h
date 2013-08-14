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

#ifndef _MODULE__PERSISTENCE_FRAMEWORK_PRIVATE_H_
#define _MODULE__PERSISTENCE_FRAMEWORK_PRIVATE_H_

/**
 * @file
 * @brief IronBee Engine --- Persistence Framework
 *
 * @author Sam Baskinger <sbaskinger@qualys.com>
 */

#define PERSISTENCE_FRAMEWORK_MODULE_NAME persistence_framework
#define PERSISTENCE_FRAMEWORK_MODULE_NAME_STR IB_XSTRINGIFY(PERSISTENCE_FRAMEWORK_MODULE_NAME)


#include "persistence_framework.h"

#include <ironbee/build.h>
#include <ironbee/engine_types.h>
#include <ironbee/field.h>
#include <ironbee/hash.h>
#include <ironbee/module.h>

#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure contains handlers for a particular type.
 *
 * A handler cannot store data, though. A handler must first be used
 * to create an implementation instance. The implementation instance
 * plus its associated handler structure is a @ref ib_persist_fw_store_t.
 */
struct ib_persist_fw_handler_t {
    const char                 *type;         /**< The type this handles. */
    ib_persist_fw_create_fn_t   create_fn;    /**< Create an instance. */
    void                       *create_data;  /**< Callback data. */
    ib_persist_fw_destroy_fn_t  destroy_fn;   /**< Destroy an instance. */
    void                       *destroy_data; /**< Callback data. */
    ib_persist_fw_load_fn_t     load_fn;      /**< Load data from an instance. */
    void                       *load_data;    /**< Callback data. */
    ib_persist_fw_store_fn_t    store_fn;     /**< Store data in an instance. */
    void                       *store_data;   /**< Callback data. */
};
typedef struct ib_persist_fw_handler_t ib_persist_fw_handler_t;

/**
 * A store is an instances of a @ref ib_persist_fw_handler_t.
 *
 * A @ref ib_persist_fw_handler_t plus the implementation data
 * created by ib_persist_fw_handler_t::create_fn is a @ref ib_persist_fw_store_t.
 */
struct ib_persist_fw_store_t {
    const char           *name;    /**< The name this is hashed under. */

    /**
     * The handler.
     *
     * When a store is destroyed this is set to NULL.
     */
    ib_persist_fw_handler_t *handler;
    void                 *impl;    /**< User implementation data. */
};
typedef struct ib_persist_fw_store_t ib_persist_fw_store_t;

/**
 * A mapping is a key, a collection name, and a @ref ib_persist_fw_store_t.
 *
 * This represents a mapping of a collection to persisted data via key.
 *
 * The key may be string constant or may be a variable to be expanded
 * when storing or loading data.
 */
struct ib_persist_fw_mapping_t {
    const char *name;          /**< Collection Name. */
    const char *key;           /**< The key the collection is stored under. */
    ib_persist_fw_store_t *store; /**< The store the data is in. */
};
typedef struct ib_persist_fw_mapping_t ib_persist_fw_mapping_t;

/**
 * Persistence framework structure given to a user module.
 *
 * The user's module keeps this handle and returns it to the
 * persistence module API to fetch the configuration information.
 */
struct ib_persist_fw_t {
    ib_engine_t *ib;             /**< The engine registered to. */
    ib_module_t *persist_fw_module; /**< The persistence framework's module. */
    ib_module_t *user_module;    /**< User's module structure. */
};

/**
 * The persistence framework module configuration type.
 */
struct ib_persist_fw_cfg_t {
    /**
     * Map of type to ib_persist_fw_handler_t.
     *
     * This is copied in new configuration contexts.
     */
    ib_hash_t   *handlers;

    /**
     * A map of named @ref ib_persist_fw_store_t.
     *
     * Names stores are looked up here and linked to named collections
     * in ib_persist_fw_t::coll_list.
     *
     * This is copied in new configuration contexts.
     */
    ib_hash_t   *stores;

    /**
     * A list of ib_persist_fw_mapping_t.
     *
     * This is copied in new configuration contexts.
     */
    ib_list_t   *coll_list;
};
typedef struct ib_persist_fw_cfg_t ib_persist_fw_cfg_t;

/**
 * An array of @ref ib_persist_fw_cfg_t indexed by the user module number.
 */
struct ib_persist_fw_modlist_t {
    /**
     * Array of NULL or @ref ib_persist_fw_cfg_t.
     *
     * The index is the client module's index value. The stored
     * @ref ib_persist_fw_cfg_t is the configuration for that
     * context and that module managed by the persistence module
     * on behalf of the user module.
     *
     * If an entry is NULL, then it means that no module has registered.
     */
    ib_array_t *configs;
};
typedef struct ib_persist_fw_modlist_t ib_persist_fw_modlist_t;

/**
 * Initialize @a persist_fw.
 *
 * @param[in] mp Allocate out of this memory pool.
 * @param[out] persist_fw Initialize this.
 *
 * @returns
 * - IB_OK
 * - IB_EALLOC
 */
ib_status_t ib_persist_fw_cfg_create(
    ib_mpool_t           *mp,
    ib_persist_fw_cfg_t **persist_fw
);

#ifdef __cplusplus
}
#endif

#endif /* _MODULE__PERSISTENCE_FRAMEWORK_PRIVATE_H_ */
