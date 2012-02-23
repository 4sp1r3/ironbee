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
 *****************************************************************************/

#ifndef _IB_CORE_PRIVATE_H_
#define _IB_CORE_PRIVATE_H_

/**
 * @file
 * @brief IronBee - Definitions private to the IronBee core module
 *
 * @author Nick LeRoy <nleroy@qualys.com>
 */

#include <ironbee/build.h>
#include <ironbee/release.h>

#include <ironbee/types.h>
#include <ironbee/list.h>
#include <ironbee/operator.h>
#include <ironbee/action.h>
#include <ironbee/engine.h>
#include <ironbee/rule_defs.h>

#include "ironbee_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Field operator function type.
 *
 * @param[in] ib Ironbee engine.
 * @param[in] mp Memory pool to use.
 * @param[in] field The field to operate on.
 * @param[out] result The result of the operator 1=true 0=false.
 *
 * @returns IB_OK if successful.
 */
typedef ib_status_t (* ib_field_op_fn_t)(ib_engine_t *ib,
                                         ib_mpool_t *mp,
                                         ib_field_t *field,
                                         ib_field_t **result);

/**
 * Rule engine: Rule meta data
 */
typedef struct {
    const char            *id;            /**< Rule ID */
    const char            *msg;           /**< Rule message */
    ib_list_t             *tags;          /**< Rule tags */
    ib_rule_phase_t        phase;         /**< Rule execution phase */
    uint8_t                severity;      /**< Rule severity */
    uint8_t                confidence;    /**< Rule confidence */
} ib_rule_meta_t;

/**
 * Rule engine: Rule list
 */
typedef struct {
    ib_list_t             *rule_list;     /**< List of rules */
} ib_rulelist_t;

/**
 * Rule engine: Target fields
 */
typedef struct {
    const char            *field_name;    /**< The field name */
    ib_list_t             *field_ops;     /**< List of field operators */
} ib_rule_target_t;

/**
 * Rule engine: Rule
 *
 * The typedef of ib_rule_t is done in ironbee/rule_engine.h
 */
struct ib_rule_t {
    ib_rule_meta_t         meta;          /**< Rule meta data */
    ib_operator_inst_t    *opinst;        /**< Rule operator */
    ib_list_t             *target_fields; /**< List of target fields */
    ib_list_t             *true_actions;  /**< Actions if condition True */
    ib_list_t             *false_actions; /**< Actions if condition False */
    ib_rulelist_t         *parent_rlist;  /**< Parent rule list */
    ib_rule_t             *chained_rule;  /**< Next rule in the chain */
    ib_flags_t             flags;         /**< External, etc. */
};

/**
 * Rule engine: List of rules to execute during a phase
 */
typedef struct {
    ib_rule_phase_t        phase;         /**< Phase number */
    ib_rulelist_t          rules;         /**< Rules to execute in phase */
} ib_rule_phase_data_t;

/**
 * Rule engine: Set of rules for all phases
 */
typedef struct {
    ib_rule_phase_data_t  phases[IB_RULE_PHASE_COUNT];
} ib_ruleset_t;


/**
 * Rule engine parser data
 */
typedef struct {
    ib_rule_t         *previous;     /**< Previous rule parsed */
} ib_rule_parser_data_t;

/**
 * Rule engine data; typedef in ironbee_private.h
 */
struct ib_rule_engine_t {
    ib_ruleset_t          ruleset;     /**< Rules to exec */
    ib_rulelist_t         rule_list;   /**< All rules owned by this context */
    ib_rule_parser_data_t parser_data; /**< Rule parser specific data */
};

/**
 * @internal
 * Initialize the rule engine.
 *
 * Called when the rule engine is loaded, registers event handlers.
 *
 * @param[in,out] ib IronBee object
 * @param[in] mod Module object
 */
ib_status_t ib_rule_engine_init(ib_engine_t *ib,
                                ib_module_t *mod);

/**
 * @internal
 * Initialize a context the rule engine.
 *
 * Called when a context is initialized, performs rule engine initialization.
 *
 * @param[in,out] ib IronBee object
 * @param[in] mod Module object
 * @param[in,out] ctx IronBee context
 */
ib_status_t ib_rule_engine_ctx_init(ib_engine_t *ib,
                                    ib_module_t *mod,
                                    ib_context_t *ctx);

/**
 * @internal
 * Initialize the core operators.
 *
 * Called when the rule engine is loaded, registers the core operators.
 *
 * @param[in,out] ib IronBee object
 * @param[in] mod Module object
 */
ib_status_t ib_core_operators_init(ib_engine_t *ib,
                                   ib_module_t *mod);

/**
 * @internal
 * Initialize the core actions.
 *
 * Called when the rule engine is loaded, registers the core actions.
 *
 * @param[in,out] ib IronBee object
 * @param[in] mod Module object
 */
ib_status_t ib_core_actions_init(ib_engine_t *ib,
                                 ib_module_t *mod);


#ifdef __cplusplus
}
#endif

#endif /* _IB_CORE_PRIVATE_H_ */
