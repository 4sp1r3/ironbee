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

#ifndef _IB_RULE_DEFS_H_
#define _IB_RULE_DEFS_H_

/**
 * @file
 * @brief IronBee --- Rule engine definitions
 *
 * @author Nick LeRoy <nleroy@qualys.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup IronBeeRule
 * @{
 */

/**
 * Rule phase number.
 */
typedef enum {
    PHASE_INVALID = -1,        /**< Invalid; used to terminate list */
    PHASE_NONE,                /**< No phase */
    PHASE_REQUEST_HEADER,      /**< Request header available. */
    PHASE_REQUEST_BODY,        /**< Request body available. */
    PHASE_RESPONSE_HEADER,     /**< Response header available. */
    PHASE_RESPONSE_BODY,       /**< Response body available. */
    PHASE_POSTPROCESS,         /**< Post-processing phase. */
    PHASE_STR_REQUEST_HEADER,  /**< Stream: Req. header available. */
    PHASE_STR_REQUEST_BODY,    /**< Stream: Req. body available. */
    PHASE_STR_RESPONSE_HEADER, /**< Stream: Resp. header available. */
    PHASE_STR_RESPONSE_BODY,   /**< Stream: Resp. body available. */
    IB_RULE_PHASE_COUNT,       /**< Size of rule phase lists */
} ib_rule_phase_num_t;

/**
 * Rule flags
 *
 * If the external flag is set, the rule engine will always execute the
 * operator, passing NULL in as the field pointer.  The external rule is
 * expected to extract whatever fields, etc. it requires itself.
 */
#define IB_RULE_FLAG_NONE     (0x0)     /**< No flags */
#define IB_RULE_FLAG_VALID    (1 << 0)  /**< Rule is valid */
#define IB_RULE_FLAG_EXTERNAL (1 << 1)  /**< External rule */
#define IB_RULE_FLAG_STREAM   (1 << 2)  /**< External rule */
#define IB_RULE_FLAG_CHPARENT (1 << 3)  /**< Rule is parent in a chain */
#define IB_RULE_FLAG_CHCHILD  (1 << 4)  /**< Rule is child in a chain */
#define IB_RULE_FLAG_MAIN_CTX (1 << 5)  /**< Rule owned by main context */
#define IB_RULE_FLAG_MARK     (1 << 6)  /**< Mark used in list building */
#define IB_RULE_FLAG_CAPTURE  (1 << 7)  /**< Enable result capture */
#define IB_RULE_FLAG_CHAIN    (IB_RULE_FLAG_CHPARENT|IB_RULE_FLAG_CHCHILD)
#define IB_RULE_FLAG_FORCE_EN (1 << 8)  /**< Force enable (for action) */
#define IB_RULE_FLAG_NO_TGT   (1 << 9)  /**< Rule has no targets (for action) */
#define IB_RULE_FLAG_ACTION   (IB_RULE_FLAG_FORCE_EN|IB_RULE_FLAG_NO_TGT)

/**
 * Rule meta-data flags
 */
#define IB_RULEMD_FLAG_NONE        (0x0)     /**< No flags */
#define IB_RULEMD_FLAG_EXPAND_MSG  (1 << 0)  /**< Expand rule message */
#define IB_RULEMD_FLAG_EXPAND_DATA (1 << 1)  /**< Expand rule logdata */

/**
 * Rule context flags
 */
#define IB_RULECTX_FLAG_NONE          (0x0)  /**< No flags */
#define IB_RULECTX_FLAG_ENABLED    (1 << 0)  /**< Rule is enabled */

/**
 * Rule execution logging flags.
 **/
#define IB_RULE_LOG_FLAG_NONE           (0x0) /**< No logging */
#define IB_RULE_LOG_FLAG_TX         (1 <<  0) /**< Transaction start / end */
#define IB_RULE_LOG_FLAG_REQ_LINE   (1 <<  1) /**< Request line */
#define IB_RULE_LOG_FLAG_REQ_HEADER (1 <<  2) /**< Request header */
#define IB_RULE_LOG_FLAG_REQ_BODY   (1 <<  3) /**< Request body */
#define IB_RULE_LOG_FLAG_RSP_LINE   (1 <<  4) /**< Response line */
#define IB_RULE_LOG_FLAG_RSP_HEADER (1 <<  5) /**< Response header */
#define IB_RULE_LOG_FLAG_RSP_BODY   (1 <<  6) /**< Response body */
#define IB_RULE_LOG_FLAG_PHASE      (1 <<  7) /**< Rule engine phase */
#define IB_RULE_LOG_FLAG_RULE       (1 <<  8) /**< Rule start / end */
#define IB_RULE_LOG_FLAG_TARGET     (1 <<  9) /**< Target data */
#define IB_RULE_LOG_FLAG_TFN        (1 << 10) /**< Transformation */
#define IB_RULE_LOG_FLAG_OPERATOR   (1 << 11) /**< Operator */
#define IB_RULE_LOG_FLAG_ACTION     (1 << 12) /**< Action */
#define IB_RULE_LOG_FLAG_EVENT      (1 << 13) /**< Generated event */
#define IB_RULE_LOG_FLAG_AUDIT      (1 << 14) /**< Audit log */
#define IB_RULE_LOG_FLAG_TIMING     (1 << 15) /**< Timing information */
/* The following flags control which rules get logged */
#define IB_RULE_LOG_FILT_ALL        (1 << 16) /**< Log all rules */
#define IB_RULE_LOG_FILT_ACTIONABLE (1 << 17) /**< Rules that execute actions */
#define IB_RULE_LOG_FILT_OPEXEC     (1 << 18) /**< Rules with op executions */
#define IB_RULE_LOG_FILT_ERROR      (1 << 19) /**< Rules with errors */
#define IB_RULE_LOG_FILT_TRUE       (1 << 20) /**< Rules that return true */
#define IB_RULE_LOG_FILT_FALSE      (1 << 21) /**< Rules that return false */

/**
 * Mask of all of the enable bits of the rule logging flags
 */
#define IB_RULE_LOG_ENABLE_MASK                              \
    ( IB_RULE_LOG_FLAG_TX |                                  \
      IB_RULE_LOG_FLAG_REQ_LINE |                            \
      IB_RULE_LOG_FLAG_REQ_HEADER |                          \
      IB_RULE_LOG_FLAG_REQ_BODY |                            \
      IB_RULE_LOG_FLAG_RSP_LINE |                            \
      IB_RULE_LOG_FLAG_RSP_HEADER |                          \
      IB_RULE_LOG_FLAG_RSP_BODY |                            \
      IB_RULE_LOG_FLAG_PHASE |                               \
      IB_RULE_LOG_FLAG_RULE |                                \
      IB_RULE_LOG_FLAG_RULE_DATA |                           \
      IB_RULE_LOG_FLAG_TFN |                                 \
      IB_RULE_LOG_FLAG_OPERATOR |                            \
      IB_RULE_LOG_FLAG_ACTION |                              \
      IB_RULE_LOG_FLAG_EVENT |                               \
      IB_RULE_LOG_FLAG_AUDIT |                               \
      IB_RULE_LOG_FLAG_TIMING )

/**
 * Mask of all of the filter bits of the rule logging flags.
 * Note: This mask does *not* include FILT_ALL.
 */
#define IB_RULE_LOG_FILTER_MASK                      \
    ( IB_RULE_LOG_FILT_ACTIONABLE |                  \
      IB_RULE_LOG_FILT_OPEXEC |                      \
      IB_RULE_LOG_FILT_ERROR |                       \
      IB_RULE_LOG_FILT_TRUE |                        \
      IB_RULE_LOG_FILT_FALSE )

/**
 * Mask of all of the filter bits of the rule logging flags.
 * Note: This mask does not include FILT_ALL.
 */
#define IB_RULE_LOG_FILTER_ALLMASK                   \
    ( IB_RULE_LOG_FILT_ALL |                         \
      IB_RULE_LOG_FILTER_MASK )

/**
 * Rule log debugging level
 **/
typedef enum {
    IB_RULE_DLOG_ALWAYS,            /**< Always log this message */
    IB_RULE_DLOG_ERROR,             /**< Error in rule execution */
    IB_RULE_DLOG_WARNING,           /**< Warning in rule execution */
    IB_RULE_DLOG_NOTICE,            /**< Something unusual rule execution */
    IB_RULE_DLOG_INFO,              /**< Something usual in rule execution */
    IB_RULE_DLOG_DEBUG,             /**< Developer oriented information */
    IB_RULE_DLOG_TRACE,             /**< Reserved for future use */
} ib_rule_dlog_level_t;

/**
 * Rule engine: Basic rule type information
 */
typedef struct ib_rule_t ib_rule_t;
typedef struct ib_rule_exec_t ib_rule_exec_t;
typedef struct ib_rule_target_t ib_rule_target_t;

/**
 * Rule execution logging data
 */
typedef struct ib_rule_log_exec_t ib_rule_log_exec_t;
typedef struct ib_rule_log_tx_t ib_rule_log_tx_t;


/**
 * @} IronBeeRuleDefs
 */

#ifdef __cplusplus
}
#endif

#endif /* _IB_RULE_DEFS_H_ */
