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
 * @brief IronBee --- Configuration File Parser
 *
 * @author Brian Rectanus <brectanus@qualys.com>
 */

#include "ironbee_config_auto.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

#include <ironbee/engine.h>
#include <ironbee/util.h>
#include <ironbee/config.h>
#include <ironbee/mpool.h>
#include <ironbee/path.h>

#include "config-parser.h"

/**
 * Finite state machine type.
 *
 * Contains state information for Ragel's parser.
 * Many of these values and names come from the Ragel documentation, section
 * 5.1 Variable Used by Ragel. p35 of The Ragel Guide 6.7 found at
 * http://www.complang.org/ragel/ragel-guide-6.7.pdf
 */
typedef struct {
    const char    *p;     /**< Pointer to the chunk being parsed. */
    const char    *pe;    /**< Pointer past the end of p (p+length(p)). */
    const char    *eof;   /**< eof==p==pe on last chunk. NULL otherwise. */
    const char    *ts;    /**< Pointer to character data for Ragel. */
    const char    *te;    /**< Pointer to character data for Ragel. */
    int      cs;          /**< Current state. */
    int      top;         /**< Top of the stack. */
    int      act;         /**< Used to track the last successful match. */
    int      stack[1024]; /**< Stack of states. */
} fsm_t;


/**
 * @brief Malloc and unescape into that buffer the marked string.
 * @param[in] cp The configuration parser
 * @param[in] fpc_mark The start of the string.
 * @param[in] fpc The current character from ragel.
 * @param[in,out] mp Temporary memory pool passed in by Ragel.
 * @return a buffer allocated from the tmpmp memory pool
 *         available in ib_cfgparser_ragel_parse_chunk. This buffer may be
 *         larger than the string stored in it if the length of the string is
 *         reduced by Javascript unescaping.
 */
static char* alloc_cpy_marked_string(ib_cfgparser_t *cp,
                                     const char *fpc_mark,
                                     const char *fpc,
                                     ib_mpool_t* mp)
{
    const char *afpc = fpc;
    size_t pvallen;
    char* pval;
    /* Adjust for quoted value. */
    if ((*fpc_mark == '"') && (*(afpc-1) == '"') && (fpc_mark+1 < afpc)) {
        fpc_mark++;
        afpc--;
    }
    pvallen = (size_t)(afpc - fpc_mark);
    pval = (char *)ib_mpool_memdup(mp, fpc_mark, (pvallen + 1) * sizeof(*pval));

    pval[pvallen] = '\0';

    /* At this point the buffer i pvallen+1 in size, but we cannot shrink it. */
    /* This is not considered a problem for configuration parsing and it is
       deallocated after parsing and configuration is complete. */
    return pval;
}

static ib_status_t include_config_fn(ib_cfgparser_t *cp,
                                     ib_mpool_t* mp,
                                     const char *directive,
                                     const char *mark,
                                     const char *fpc,
                                     const char *file,
                                     unsigned int lineno)
{
    struct stat statbuf;
    ib_status_t rc;
    int statval;
    char *incfile;
    char *pval;
    char *real;
    char *lookup;
    void *freeme = NULL;
    bool if_exists = strcasecmp("IncludeIfExists", directive) ? false : true;

    pval = alloc_cpy_marked_string(cp, mark, fpc, mp);
    incfile = ib_util_relative_file(mp, file, pval);
    if (incfile == NULL) {
        ib_cfg_log_error(cp, "Failed to resolve included file \"%s\": %s",
                         file, strerror(errno));
        return IB_ENOENT;
    }

    real = realpath(incfile, NULL);
    if (real == NULL) {
        if (!if_exists) {
            ib_cfg_log_error(cp,
                             "Failed to find real path of included file "
                             "(using original \"%s\"): %s",
                             incfile, strerror(errno));
        }

        real = incfile;
    }
    else {
        if (strcmp(real, incfile) != 0) {
            ib_cfg_log_info(cp,
                            "Real path of included file \"%s\" is \"%s\"",
                            incfile, real);
        }
        freeme = real;
    }

    /* Look up the real file path in the hash */
    rc = ib_hash_get(cp->includes, &lookup, real);
    if (rc == IB_OK) {
        ib_cfg_log_warning(cp,
                           "Included file \"%s\" already included: skipping",
                           real);
        return IB_OK;
    }
    else if (rc != IB_ENOENT) {
        ib_cfg_log_error(cp, "Error looking up include file \"%s\": %s",
                         real, strerror(errno));
    }

    /* Put the real name in the hash */
    lookup = ib_mpool_strdup(mp, real);
    if (freeme != NULL) {
        free(freeme);
        freeme = NULL;
    }
    if (lookup != NULL) {
        rc = ib_hash_set(cp->includes, lookup, lookup);
        if (rc != IB_OK) {
            ib_cfg_log_error(cp,
                             "Error adding include file to hash \"%s\": %s",
                             lookup, strerror(errno));
        }
    }

    if (access(incfile, R_OK) != 0) {
        if (if_exists) {
            ib_cfg_log(cp, (errno == ENOENT) ? IB_LOG_DEBUG : IB_LOG_NOTICE,
                       "Ignoring include file \"%s\": %s",
                       incfile, strerror(errno));
            return IB_OK;
        }

        ib_cfg_log_error(cp, "Cannot access included file \"%s\": %s",
                         incfile, strerror(errno));
        return IB_ENOENT;
    }

    statval = stat(incfile, &statbuf);
    if (statval != 0) {
        if (if_exists) {
            ib_cfg_log(cp, (errno == ENOENT) ? IB_LOG_DEBUG : IB_LOG_NOTICE,
                       "Ignoring include file \"%s\": %s",
                       incfile, strerror(errno));
            return IB_OK;
        }

        ib_cfg_log_error(cp,
                         "Failed to stat include file \"%s\": %s",
                         incfile, strerror(errno));
        return IB_ENOENT;
    }

    if (S_ISREG(statbuf.st_mode) == 0) {
        if (if_exists) {
            ib_cfg_log_info(cp,
                            "Ignoring include file \"%s\": Not a regular file",
                            incfile);
            return IB_OK;
        }

        ib_cfg_log_error(cp,
	                 "Included file \"%s\" is not a regular file", incfile);
        return IB_ENOENT;
    }

    ib_cfg_log_debug(cp, "Including '%s'", incfile);
    rc = ib_cfgparser_parse(cp, incfile);
    if (rc != IB_OK) {
        ib_cfg_log_error(cp, "Error parsing included file \"%s\": %s",
	                 incfile, ib_status_to_string(rc));
        return rc;
    }

    ib_cfg_log_debug(cp, "Done processing include file \"%s\"", incfile);
    return IB_OK;
}

%%{
    machine ironbee_config;

    # Mark the start of a string.
    action mark { mark = fpc; }
    action error_action {
        rc = IB_EOTHER;
        ib_cfg_log_error(cp,
                         "parser error before \"%.*s\"",
                         (int)(fpc - mark), mark);
    }

    # Parameter
    action push_param {
        pval = alloc_cpy_marked_string(cp, mark, fpc, mpcfg);
        ib_list_push(plist, pval);
    }
    action push_blkparam {
        pval = alloc_cpy_marked_string(cp, mark, fpc, mpcfg);
        ib_list_push(plist, pval);
    }

    # Directives
    action start_dir {
        size_t namelen = (size_t)(fpc - mark);
        directive = (char *)calloc(namelen + 1, sizeof(*directive));
        memcpy(directive, mark, namelen);
        ib_list_clear(plist);
    }
    action push_dir {
        rc = ib_config_directive_process(cp, directive, plist);
        if (rc != IB_OK) {
            ib_cfg_log_error(cp,
                             "Failed to process directive \"%s\" "
                             ": %s (see preceeding messages for details)",
                             directive, ib_status_to_string(rc));
        }
        if (directive != NULL) {
            free(directive);
            directive = NULL;
        }
    }

    # Blocks
    action start_block {
        size_t namelen = (size_t)(fpc - mark);
        blkname = (char *)calloc(namelen + 1, sizeof(*blkname));
        memcpy(blkname, mark, namelen);
        ib_list_clear(plist);
    }
    action push_block {
        rc = ib_config_block_start(cp, blkname, plist);
        if (rc != IB_OK) {
            ib_cfg_log_error(cp,
	                     "Failed to start block \"%s\": %s",
                             blkname, ib_status_to_string(rc));
        }
    }
    action pop_block {
        blkname = (char *)cp->cur_blkname;
        rc = ib_config_block_process(cp, blkname);
        if (rc != IB_OK) {
            ib_cfg_log_error(cp,
                             "Failed to process block \"%s\": %s",
                             blkname, ib_status_to_string(rc));
        }
        if (blkname != NULL) {
            free(blkname);
        }
        blkname = (char *)cp->cur_blkname;
    }

    # include file logic
    action include_config {
        rc = include_config_fn(cp, mpcfg, directive, mark, fpc, file, lineno);

        if (rc == IB_OK) {
            ib_cfg_log_debug(cp, "Done processing include directive");
        }
        else {
            ib_cfg_log_error(cp,
                             "Failed to process include directive: %s",
                             ib_status_to_string(rc));
        }

        if (directive != NULL) {
            free(directive);
            directive = NULL;
        }
    }

    # Line continuation logic
    action handle_continuation {
        size_t len = (size_t)(fpc - mark);
        ib_cfg_log_debug(cp, "contination: \"%.*s\"", (int)len, mark);
        /* blkname = (char *)calloc(namelen + 1, sizeof(*blkname));
           memcpy(blkname, mark, namelen); */
    }

    WS = [ \t];
    EOLSEQ = '\r'? '\n';
    EOL = WS* EOLSEQ;
    CONT = '\\' EOL;

    sep = WS+;
    qchar = '\\' any;
    qtoken = '"' ( qchar | ( any - ["\\] ) )* '"';
    token = (qchar | (any - (WS | EOL | [<>#"\\]))) (qchar | (any - ( WS | EOL | [<>"\\])))*;
    param = qtoken | token;
    keyval = token '=' param;
    iparam = ( '"' (any - (EOL | '"'))+ '"' ) | (any - (WS | EOL) )+;

    comment = '#' (any -- EOLSEQ)*;

    parameters := |*
        WS* param >mark %push_param $/push_param $/push_dir;
        EOL @push_dir { fret; };
    *|;

    block_parameters := |*
        WS* param >mark %push_blkparam;
        WS* ">" @push_block { fret; };
    *|;

    newblock := |*
        WS* token >mark %start_block $!error_action { fcall block_parameters; };
        EOL $!error_action { fret; };
    *|;

    endblock := |*
	WS* EOL %error_action { fret; };
        WS* token >mark $!error_action %pop_block;
        WS* ">" EOL $!error_action { fret; };
    *|;

    finclude := |*
        WS* iparam >mark $!error_action %include_config EOL >mark $!error_action { fret; };
    *|;

    main := |*
        WS* comment;
        WS* CONT %handle_continuation;
        WS* [Ii] [Nn] [Cc] [Ll] [Uu] [Dd] [Ee] ([Ii] [Ff] [Ee] [Xx] [Ii] [Ss] [Tt] [Ss])? { fcall finclude; };
        WS* token >mark %start_dir { fcall parameters; };
        "<" { fcall newblock; };
        "</" { fcall endblock; };
        WS+;
        EOL;
    *|;
}%%

%% write data;

ib_status_t ib_cfgparser_ragel_parse_chunk(ib_cfgparser_t *cp,
                                           const char *buf,
                                           const size_t blen,
                                           const char *file,
                                           const unsigned lineno,
                                           const int is_last_chunk)
{
    ib_engine_t *ib_engine = cp->ib;

    /* Temporary memory pool. */
    ib_mpool_t *mptmp = ib_engine_pool_temp_get(ib_engine);

    /* Configuration memory pool. */
    ib_mpool_t *mpcfg = ib_engine_pool_config_get(ib_engine);

    /* Error actions will update this. */
    ib_status_t rc = IB_OK;

    /* Directive name being parsed. */
    char *directive = NULL;

    /* Block name being parsed. */
    char *blkname = NULL;

    /* Parameter value being added to the plist. */
    char *pval = NULL;

    /* Store the start of a string to act on.
     * fpc - mark is the string length when processing after
     * a mark action. */
    const char *mark = buf;

    /* Temporary list for storing values before they are committed to the
     * configuration. */
    ib_list_t *plist;

    /* Create a finite state machine type. */
    fsm_t fsm;

    fsm.p = buf;
    fsm.pe = buf + blen;
    fsm.eof = (is_last_chunk ? fsm.pe : NULL);
    memset(fsm.stack, 0, sizeof(fsm.stack));

    /* Create a temporary list for storing parameter values. */
    ib_list_create(&plist, mptmp);
    if (plist == NULL) {
        return IB_EALLOC;
    }

    /* Access all ragel state variables via structure. */
    %% access fsm.;
    %% variable p fsm.p;
    %% variable pe fsm.pe;
    %% variable eof fsm.eof;

    %% write init;
    %% write exec;

    /* Ensure that our block is always empty on last chunk. */
    if ( is_last_chunk && blkname != NULL ) {
        return IB_EINVAL;
    }

    return rc;
}
