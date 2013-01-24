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
 * @brief IronBee --- User Agent Extraction Module
 *
 * This module extracts the user agent information
 *
 * @author Nick LeRoy <nleroy@qualys.com>
 */

#include "user_agent_private.h"

#include <ironbee/bytestr.h>
#include <ironbee/engine.h>
#include <ironbee/field.h>
#include <ironbee/hash.h>
#include <ironbee/ip.h>
#include <ironbee/module.h>
#include <ironbee/mpool.h>
#include <ironbee/string.h>
#include <ironbee/types.h>
#include <ironbee/util.h>

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>

/* Define the module name as well as a string version of it. */
#define MODULE_NAME        user_agent
#define MODULE_NAME_STR    IB_XSTRINGIFY(MODULE_NAME)

/* Declare the public module symbol. */
IB_MODULE_DECLARE();

static const modua_match_ruleset_t *modua_match_ruleset = NULL;

/**
 * Skip spaces, return pointer to first non-space.
 *
 * Skips spaces in the passed in string.
 *
 * @param[in] str String to skip
 *
 * @returns Pointer to first non-space character in the string.
 */
static char *skip_space(char *str)
{
    while (*str == ' ') {
        ++str;
    }
    return (*str == '\0') ? NULL : str;
}

/**
 * Parse the user agent header.
 *
 * Attempt to tokenize the user agent string passed in, splitting up
 * the passed in string into component parts.
 *
 * @param[in,out] str User agent string to parse
 * @param[out] p_product Pointer to product string
 * @param[out] p_platform Pointer to platform string
 * @param[out] p_extra Pointer to "extra" string
 *
 * @returns Status code
 */
static ib_status_t modua_parse_uastring(char *str,
                                        char **p_product,
                                        char **p_platform,
                                        char **p_extra)
{
    char *lp = NULL;            /* lp: Left parent */
    char *extra = NULL;

    /* Initialize these to known values */
    *p_platform = NULL;
    *p_extra = NULL;

    /* Skip any leading space */
    str = skip_space(str);

    /* Simple validation */
    if ( (str == NULL) || (isalnum(*str) == 0) ) {
        *p_product = NULL;
        *p_extra = str;
        return  (str == NULL) ? IB_EUNKNOWN : IB_OK ;
    }

    /* The product is the first field. */
    *p_product = str;

    /* Search for a left parent followed by a right paren */
    lp = strstr(str, " (");
    if (lp != NULL) {
        char *rp;

        /* Find the matching right paren (if it exists).
         *  First try ") ", then ")" to catch a paren at end of line. */
        rp = strstr(lp, ") ");
        if (rp == NULL) {
            rp = strchr(str, ')');
        }

        /* If no matching right paren, ignore the left paren */
        if (rp == NULL) {
            lp = NULL;
        }
        else {
            /* Terminate the string after the right paren */
            ++rp;
            if ( (*rp == ' ') || (*rp == ',') || (*rp == ';') ) {
                extra = rp;
                *extra++ = '\0';
            }
            else if (*rp != '\0') {
                lp = NULL;
            }
        }
    }

    /* No parens?  'Extra' starts after the first space */
    if (lp == NULL) {
        char *cur = strchr(str, ' ');
        if (cur != NULL) {
            *cur++ = '\0';
        }
        extra = cur;
    }

    /* Otherwise, clean up around the parens */
    else {
        char *cur = lp++;
        while ( (cur > str) && (*cur == ' ') ) {
            *cur = '\0';
            --cur;
        }
    }

    /* Skip extra whitespace preceding the real extra */
    if (extra != NULL) {
        extra = skip_space(extra);
    }

    /* Done: Store the results in the passed in pointers.
     * Note: p_product is filled in above. */
    *p_platform = lp;
    *p_extra = extra;
    return IB_OK;
}

/* Macros used to return the correct value based on a value and an
 * match value.
 * av: Actual value
 * mv: Match value
 */
#define RESULT_EQ(av, mv)  ( ((av) == (mv)) ? YES : NO )
#define RESULT_NE(av, mv) ( ((av) != (mv)) ? YES : NO )

/**
 * Match a field against the specified match rule.
 *
 * Attempts to match the field string (or NULL) against the field match rule.
 * NULL)
 *
 * @param[in] str User agent string to match
 * @param[in] rule Field match rule to match the string against
 *
 * @returns 1 if the string matches, 0 if not.
 */
static modua_matchresult_t modua_frule_match(const char *str,
                                             const modua_field_rule_t *rule)
{
    /* First, handle the simple NULL string case */
    if (str == NULL) {
        return NO;
    }

    /* Match using the rule's match type */
    switch (rule->match_type) {
        case EXISTS:         /* Note: NULL/NO handled above */
            return YES;
        case MATCHES:
            return RESULT_EQ(strcmp(str, rule->string), 0);
        case STARTSWITH:
            return RESULT_EQ(strncmp(str, rule->string, rule->slen), 0);
        case CONTAINS:
            return RESULT_NE(strstr(str, rule->string), NULL);
        case ENDSWITH: {
            size_t slen = strlen(str);
            size_t offset;
            if (slen < rule->slen) {
                return 0;
            }
            offset = (slen - rule->slen);
            return RESULT_EQ(strcmp(str+offset, rule->string), 0);
        }
        default :
            fprintf(stderr,
                    "modua_frule_match: invalid match type %d",
                    rule->match_type);
    }

    /* Should never get here! */
    return NO;
}

/**
 * Apply the user agent category rules.
 *
 * Walks through the internal static category rules, attempts to apply
 * each of them to the passed in agent info.
 *
 * @param[in] fields Array of fields to match (0=product,1=platform,2=extra)
 * @param[in] rule Match rule to match against
 *
 * @returns 1 if all rules match, otherwise 0
 */
static int modua_mrule_match(const char *fields[],
                             const modua_match_rule_t *rule)
{
    const modua_field_rule_t *fr;
    unsigned int ruleno;

    /* Walk through the rules; if any fail, return NULL */
    for (ruleno = 0, fr = rule->rules;
         ruleno < rule->num_rules;
         ++ruleno, ++fr) {

        /* Apply the rule */
        modua_matchresult_t result =
            modua_frule_match(fields[fr->match_field], fr);

        /* If it doesn't match the expect results, we're done, return 0 */
        if (result != fr->match_result) {
            return  0 ;
        }
    }

    /* If we've applied all of the field rules, and all have passed,
     * return the 1 to signify a match */
    return  1 ;
}

/**
 * Apply the user agent category rules.
 *
 * Walks through the internal static category rules, attempts to apply
 * each of them to the passed in agent info, and returns a pointer to the
 * first rule that matches, or NULL if no rules match.
 *
 * Note that the fields array (filled in below) uses values from the
 * modua_matchfield_t enum (PRODUCT, PLATFORM, EXTRA).
 *
 * @param[in] product UA product component
 * @param[in] platform UA platform component
 * @param[in] extra UA extra component
 *
 * @returns Pointer to rule that matched
 */
static const modua_match_rule_t *modua_match_cat_rules(const char *product,
                                                       const char *platform,
                                                       const char *extra)
{
    const char *fields[3] = { product, platform, extra };
    const modua_match_rule_t *rule;
    unsigned int ruleno;

    assert(modua_match_ruleset != NULL);

    /* Walk through the rules; the first to match "wins" */
    for (ruleno = 0, rule = modua_match_ruleset->rules;
         ruleno < modua_match_ruleset->num_rules;
         ++ruleno, ++rule ) {
        int result;

        /* Apply the field rules */
        result = modua_mrule_match(fields, rule);

        /* If the entire rule set matches, return the matching rule */
        if (result != 0) {
            return rule ;
        }
    }

    /* If we've applied all rules, and have had success, return NULL */
    return NULL ;
}

/**
 * Store a field in the agent list
 *
 * Creates a new field and adds it to the agent list field list.
 *
 * @param[in] ib IronBee object
 * @param[in,out] mp Memory pool to allocate from
 * @param[in] agent_list Field to add the field to
 * @param[in] name Field name
 * @param[in] value Field value
 *
 * @returns Status code
 */
static ib_status_t modua_store_field(ib_engine_t *ib,
                                     ib_mpool_t *mp,
                                     ib_field_t *agent_list,
                                     const char *name,
                                     const char *value)
{
    ib_field_t *tmp_field = NULL;
    ib_status_t rc = IB_OK;

    /* No value?  Do nothing */
    if (value == NULL) {
        ib_log_debug3(ib, "No %s field in user agent", name);
        return IB_OK;
    }

    /* Create the field */
    rc = ib_field_create(
        &tmp_field,
        mp,
        IB_FIELD_NAME(name),
        IB_FTYPE_NULSTR,
        ib_ftype_nulstr_in(value)
    );
    if (rc != IB_OK) {
        ib_log_alert(ib,
                     "Error creating user agent %s field: %s", name, ib_status_to_string(rc));
        return rc;
    }

    /* Add the field to the list */
    rc = ib_field_list_add(agent_list, tmp_field);
    if (rc != IB_OK) {
        ib_log_alert(ib,
                     "Error adding user agent %s field: %s", name, ib_status_to_string(rc));
        return rc;
    }

    ib_log_debug3(ib, "Stored user agent %s '%s'", name, value);

    return IB_OK;
}

/**
 * Parse the user agent header, splitting into component fields.
 *
 * Attempt to tokenize the user agent string passed in, storing the
 * result in the DPI associated with the transaction.
 *
 * @param[in] ib IronBee object
 * @param[in,out] tx Transaction object
 * @param[in] bs Byte string containing the agent string
 *
 * @returns Status code
 */
static ib_status_t modua_agent_fields(ib_engine_t *ib,
                                      ib_tx_t *tx,
                                      const ib_bytestr_t *bs)
{
    const modua_match_rule_t *rule = NULL;
    ib_field_t               *agent_list = NULL;
    char                     *product = NULL;
    char                     *platform = NULL;
    char                     *extra = NULL;
    char                     *agent;
    char                     *buf;
    size_t                    len;
    ib_status_t               rc;

    /* Get the length of the byte string */
    len = ib_bytestr_length(bs);

    /* Allocate memory for a copy of the string to split up below. */
    buf = (char *)ib_mpool_calloc(tx->mp, 1, len+1);
    if (buf == NULL) {
        ib_log_error_tx(tx,
                        "Failed to allocate %zd bytes for agent string",
                        len+1);
        return IB_EALLOC;
    }

    /* Copy the string out */
    memcpy(buf, ib_bytestr_const_ptr(bs), len);
    buf[len] = '\0';
    ib_log_debug_tx(tx, "Found user agent: '%s'", buf);

    /* Copy the agent string */
    agent = (char *)ib_mpool_strdup(tx->mp, buf);
    if (agent == NULL) {
        ib_log_error_tx(tx, "Failed to allocate copy of agent string");
        return IB_EALLOC;
    }

    /* Parse the user agent string */
    rc = modua_parse_uastring(buf, &product, &platform, &extra);
    if (rc != IB_OK) {
        ib_log_debug_tx(tx, "Failed to parse User Agent string '%s'", agent);
        return IB_OK;
    }

    /* Categorize the parsed string */
    rule = modua_match_cat_rules(product, platform, extra);
    if (rule == NULL) {
        ib_log_debug_tx(tx, "No rule matched" );
    }
    else {
        ib_log_debug_tx(tx, "Matched to rule #%d / category '%s'",
                        rule->rule_num, rule->category );
    }

    /* Build a new list. */
    rc = ib_data_add_list(tx->data, "UA", &agent_list);
    if (rc != IB_OK)
    {
        ib_log_alert_tx(tx, "Unable to add UserAgent list to DPI.");
        return rc;
    }

    /* Store Agent */
    rc = modua_store_field(ib, tx->mp, agent_list, "agent", agent);
    if (rc != IB_OK) {
        return rc;
    }

    /* Store product */
    rc = modua_store_field(ib, tx->mp, agent_list, "PRODUCT", product);
    if (rc != IB_OK) {
        return rc;
    }

    /* Store Platform */
    rc = modua_store_field(ib, tx->mp, agent_list, "OS", platform);
    if (rc != IB_OK) {
        return rc;
    }

    /* Store Extra */
    rc = modua_store_field(ib, tx->mp, agent_list, "extra", extra);
    if (rc != IB_OK) {
        return rc;
    }

    /* Store Extra */
    if (rule != NULL) {
        rc = modua_store_field(ib, tx->mp, agent_list,
                               "category", rule->category);
    }
    else {
        rc = modua_store_field(ib, tx->mp, agent_list, "category", NULL );
    }
    if (rc != IB_OK) {
        return rc;
    }

    /* Done */
    return IB_OK;
}

/**
 * Handle request_header events for user agent extraction.
 *
 * Extract the "request_headers" field (a list) from the transactions's
 * data provider instance, then loop through the list, looking for the
 * "User-Agent"  field.  If found, the value is parsed and used to update the
 * connection object fields.
 *
 * @param[in] ib IronBee object
 * @param[in,out] tx Transaction.
 * @param[in] event Event type
 * @param[in] data Callback data (not used)
 *
 * @returns Status code
 */
static ib_status_t modua_user_agent(ib_engine_t *ib,
                                    ib_tx_t *tx,
                                    ib_state_event_type_t event,
                                    void *data)
{
    assert(ib != NULL);
    assert(tx != NULL);
    assert(tx->data != NULL);
    assert(event == request_header_finished_event);

    ib_field_t         *req_agent = NULL;
    ib_status_t         rc = IB_OK;
    const ib_list_t *bs_list;
    const ib_bytestr_t *bs;

    /* Extract the User-Agent header field from the provider instance */
    rc = ib_data_get(tx->data, "request_headers:User-Agent", &req_agent);
    if ( (req_agent == NULL) || (rc != IB_OK) ) {
        ib_log_debug_tx(tx, "request_header_finished_event: No user agent");
        return IB_OK;
    }

    if (req_agent->type != IB_FTYPE_LIST) {
        ib_log_error_tx(tx,
            "Expected request_headers:User-Agent to "
            "return list of values.");
        return IB_EINVAL;
    }

    rc = ib_field_value_type(req_agent,
                             ib_ftype_list_out(&bs_list),
                             IB_FTYPE_LIST);
    if (rc != IB_OK) {
        ib_log_error_tx(tx,
                        "Cannot retrieve request_headers:User-Agent: %d",
                        rc);
        return rc;
    }

    if (IB_LIST_ELEMENTS(bs_list) == 0) {
        ib_log_debug_tx(tx, "request_header_finished_event: No user agent");
        return IB_OK;
    }

    req_agent = (ib_field_t *)IB_LIST_NODE_DATA(IB_LIST_LAST(bs_list));

    /* Found it: copy the data into a newly allocated string buffer */
    rc = ib_field_value_type(req_agent,
                             ib_ftype_bytestr_out(&bs),
                             IB_FTYPE_BYTESTR);
    if (rc != IB_OK) {
        ib_log_error_tx(tx, "Request user agent is not a BYTESTR: %s",
                        ib_status_to_string(rc));
        return rc;
    }

    /* Finally, split it up & store the components */
    rc = modua_agent_fields(ib, tx, bs);
    return rc;
}

/**
 * Handle request_header events for remote IP extraction.
 *
 * Extract the "request_headers" field (a list) from the transactions's
 * data provider instance, then loop through the list, looking for the
 * "X-Forwarded-For"  field.  If found, the first value in the (comma
 * separated) list replaces the local ip address string in the connection
 * object.
 *
 * @param[in] ib IronBee object
 * @param[in,out] tx Transaction object
 * @param[in] event Event type
 * @param[in] cbdata Callback data (not used)
 *
 * @returns Status code
 */
static ib_status_t modua_remoteip(ib_engine_t *ib,
                                  ib_tx_t *tx,
                                  ib_state_event_type_t event,
                                  void *cbdata)
{
    assert(ib != NULL);
    assert(tx != NULL);
    assert(tx->data != NULL);
    assert(event == request_header_finished_event);

    ib_field_t           *field = NULL;
    ib_status_t           rc = IB_OK;
    const ib_bytestr_t   *bs;
    const uint8_t        *data;
    size_t                len;
    char                 *buf;
    uint8_t              *comma;
    const ib_list_t      *list;
    const ib_list_node_t *node;
    const ib_field_t     *forwarded;
    uint8_t              *stripped;
    size_t                num;
    ib_flags_t            flags;

    ib_log_debug3_tx(tx, "Checking for alternate remote address");

    /* Extract the X-Forwarded-For from the provider instance */
    rc = ib_data_get(tx->data, "request_headers:X-Forwarded-For", &field);
    if ( (field == NULL) || (rc != IB_OK) ) {
        ib_log_debug_tx(tx, "No X-Forwarded-For field");
        return IB_OK;
    }

    /* Because we asked for a filtered item, what we get back is a list */
    rc = ib_field_value(field, ib_ftype_list_out(&list));
    if (rc != IB_OK) {
        ib_log_debug_tx(tx, "No request header collection");
        return rc;
    }

    num = ib_list_elements(list);
    if (num == 0) {
        ib_log_debug_tx(tx, "No X-Forwarded-For header found");
        return rc;
    }
    else if (num != 1) {
        ib_log_debug_tx(tx, "%zd X-Forwarded-For headers found: ignoring", num);
        return rc;
    }
    node = ib_list_last_const(list);
    if ( (node == NULL) || (node->data == NULL) ) {
        ib_log_notice_tx(tx, "Invalid X-Forwarded-For header found");
        return rc;
    }
    forwarded = (const ib_field_t *)node->data;

    /* Found it: copy the data into a newly allocated string buffer */
    rc = ib_field_value_type(forwarded,
                             ib_ftype_bytestr_out(&bs),
                             IB_FTYPE_BYTESTR);
    if (rc != IB_OK) {
        ib_log_notice_tx(tx, "Invalid X-Forwarded-For header value");
        return rc;
    }

    if (bs == NULL) {
        ib_log_notice_tx(tx, "X-Forwarded-For header not a bytestr");
        return IB_EINVAL;
    }
    len = ib_bytestr_length(bs);
    data = ib_bytestr_const_ptr(bs);

    /* Search for a comma in the buffer */
    comma = memchr(data, ',', len);
    if (comma != NULL) {
        len = comma - data;
    }

    /* Trim whitespace */
    stripped = (uint8_t *)data;
    rc = ib_strtrim_lr_ex(IB_STROP_INPLACE, tx->mp,
                          stripped, len,
                          &stripped, &len, &flags);
    if (rc != IB_OK) {
        return rc;
    }

    /* Verify that it looks like a valid IP v4/6 address */
    rc = ib_ip_validate_ex((const char *)stripped, len);
    if (rc != IB_OK) {
        ib_log_error_tx(tx,
            "X-Forwarded-For \"%.*s\" is not a valid IP address",
            (int)len, stripped
        );
        return IB_OK;
    }

    /* Allocate memory for copy of stripped string */
    buf = (char *)ib_mpool_alloc(tx->mp, len+1);
    if (buf == NULL) {
        ib_log_error_tx(tx,
                        "Failed to allocate %zd bytes for remote address",
                        len+1);
        return IB_EALLOC;
    }

    /* Copy the string out */
    memcpy(buf, stripped, len);
    buf[len] = '\0';

    ib_log_debug_tx(tx, "Remote address changed to \"%s\"", buf);

    /* This will lose the pointer to the original address
     * buffer, but it should be cleaned up with the rest
     * of the memory pool. */
    tx->er_ipstr = buf;

    /* Update the remote address field in the tx collection */
    rc = ib_data_add_bytestr(tx->data, "remote_addr", (uint8_t*)buf, len, NULL);
    if (rc != IB_OK) {
        ib_log_error_tx(tx,
                        "Failed to create remote address TX field: %s",
                        ib_status_to_string(rc));
        return rc;
    }

    return IB_OK;
}

/**
 * Called to initialize the user agent module (when the module is loaded).
 *
 * Registers a handler for the request_header_finished_event event.
 *
 * @param[in,out] ib IronBee object
 * @param[in] m Module object
 * @param[in] cbdata (unused)
 *
 * @returns Status code
 */
static ib_status_t modua_init(ib_engine_t *ib, ib_module_t *m, void *cbdata)
{
    ib_status_t  rc;
    modua_match_rule_t *failed_rule;
    unsigned int failed_frule_num;

    /* Register the user agent callback */
    rc = ib_hook_tx_register(ib, request_header_finished_event,
                             modua_user_agent,
                             NULL);
    if (rc != IB_OK) {
        ib_log_error(ib, "Hook register returned %s", ib_status_to_string(rc));
    }

    /* Register the remote address callback */
    rc = ib_hook_tx_register(ib, request_header_finished_event,
                             modua_remoteip,
                             NULL);
    if (rc != IB_OK) {
        ib_log_error(ib, "Hook register returned %s", ib_status_to_string(rc));
    }

    /* Initializations */
    rc = modua_ruleset_init(&failed_rule, &failed_frule_num);
    if (rc != IB_OK) {
        ib_log_error(ib,
                     "User agent rule initialization failed"
                     " on rule %s field rule #%d: %s",
                     failed_rule->label, failed_frule_num, ib_status_to_string(rc));
    }

    /* Get the rules */
    modua_match_ruleset = modua_ruleset_get( );
    if (modua_match_ruleset == NULL) {
        ib_log_error(ib, "Failed to get user agent rule list: %s", ib_status_to_string(rc));
        return rc;
    }
    ib_log_debug(ib,
                 "Found %d match rules",
                 modua_match_ruleset->num_rules);

    return IB_OK;
}

IB_MODULE_INIT(
    IB_MODULE_HEADER_DEFAULTS,      /* Default metadata */
    MODULE_NAME_STR,                /* Module name */
    IB_MODULE_CONFIG_NULL,          /* Global config data */
    NULL,                           /* Module config map */
    NULL,                           /* Module directive map */
    modua_init,                     /* Initialize function */
    NULL,                           /* Callback data */
    NULL,                           /* Finish function */
    NULL,                           /* Callback data */
    NULL,                           /* Context open function */
    NULL,                           /* Callback data */
    NULL,                           /* Context close function */
    NULL,                           /* Callback data */
    NULL,                           /* Context destroy function */
    NULL                            /* Callback data */
);
