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

#ifndef _IB_EXPAND_H_
#define _IB_EXPAND_H_

/**
 * @file
 * @brief IronBee --- String Expansion Utility Functions
 *
 * @author Nick LeRoy <nleroy@qualys.com>
 */

#include <ironbee/build.h>
#include <ironbee/field.h>
#include <ironbee/hash.h>
#include <ironbee/mpool.h>
#include <ironbee/types.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup IronBeeUtilExpand Expand
 * @ingroup IronBeeUtil
 *
 * String expansion routines.
 *
 * @{
 */


/**
 * Function to lookup a key for expansion
 *
 * @param[in] data Object to look up data in
 * @param[in] name Key name
 * @param[in] nlen Length of @a name
 * @param[out] pf Pointer to field from hash or NULL
 *
 * @returns
 * - IB_OK if successful
 * - IB_ENOENT if not found
 */

typedef ib_status_t (* ib_expand_lookup_fn_t)(const void *data,
                                              const char *name,
                                              size_t nlen,
                                              ib_field_t **pf);

/**
 * Expand a NUL-terminated string using the given hash.
 *
 * This function looks through @a str for instances of
 * @a prefix + _name_ + @a suffix (e.g. "%{FOO}"), then attempts to look up
 * each of such name found in @a hash.  If the name is not found in @a hash,
 * the "%{_name_}" sub-string is replaced with an empty string.  If the
 * name is found, the associated field value is used to replace "%{_name_}"
 * sub-string for string and numeric types (numbers are converted to strings);
 * for others, the replacement is an empty string.
 *
 * @param[in] mp Memory pool
 * @param[in] str String to expand
 * @param[in] prefix Prefix string (e.g. "%{")
 * @param[in] suffix Suffix string (e.g. "}")
 * @param[in] recurse Do recursive expansion?
 * @param[in] hash Hash from which to lookup and expand names in @a str
 * @param[out] result Resulting string
 *
 * @returns Status code
 */
ib_status_t DLL_PUBLIC ib_expand_str(ib_mpool_t *mp,
                                     const char *str,
                                     const char *prefix,
                                     const char *suffix,
                                     bool recurse,
                                     ib_hash_t *hash,
                                     char **result);

/**
 * Expand a NUL-terminated string using the given hash.
 *
 * This function looks through @a str for instances of @a prefix + _name_ + @a
 * suffix (e.g. "%{FOO}"), then attempts to look up each of such name found in
 * @a lookup_data using @a lookup_fn.  If the name is not found, the
 * "%{_name_}" sub-string is replaced with an empty string.  If the name is
 * found, the associated field value is used to replace "%{_name_}" sub-string
 * for string and numeric types (numbers are converted to strings); for
 * others, the replacement is an empty string.
 *
 * @param[in] mp Memory pool
 * @param[in] str String to expand
 * @param[in] prefix Prefix string (e.g. "%{")
 * @param[in] suffix Suffix string (e.g. "}")
 * @param[in] recurse Do recursive expansion?
 * @param[in] lookup_fn Function to lookup a key in @a lookup_data
 * @param[in] lookup_data Hash-like object in which to expand names in @a str
 * @param[out] result Resulting string
 *
 * @returns Status code of ib_expand_str_gen_ex.
 *   - IB_OK on success or if the string is not expandable.
 *   - IB_EINVAL if prefix or suffix is zero length.
 *   - IB_EALLOC if a memory allocation failed.
 */
ib_status_t DLL_PUBLIC ib_expand_str_gen(ib_mpool_t *mp,
                                         const char *str,
                                         const char *prefix,
                                         const char *suffix,
                                         bool recurse,
                                         ib_expand_lookup_fn_t lookup_fn,
                                         const void *lookup_data,
                                         char **result);

/**
 * Expand a NUL-terminated string using the given hash, ex version.
 *
 * This function looks through @a str for instances of
 * @a prefix + _name_ + @a suffix (e.g. "%{FOO}"), then attempts to look up
 * each of such name found in @a hash.  If the name is not found in @a hash,
 * the "%{_name_}" sub-string is replaced with an empty string.  If the
 * name is found, the associated field value is used to replace "%{_name_}"
 * sub-string for string and numeric types (numbers are converted to strings);
 * for others, the replacement is an empty string.
 *
 * @param[in] mp Memory pool
 * @param[in] str String to expand
 * @param[in] str_len Length of @a str
 * @param[in] prefix Prefix string (e.g. "%{")
 * @param[in] suffix Suffix string (e.g. "}")
 * @param[in] nul Append a NUL byte to the end of @a result?
 * @param[in] recurse Do recursive expansion?
 * @param[in] hash Hash from which to lookup and expand names in @a str
 * @param[out] result Resulting string
 * @param[out] result_len Length of @a result
 *
 * @returns
 *   - IB_OK on success or if the string is not expandable.
 *   - IB_EINVAL if prefix or suffix is zero length.
 *   - IB_EALLOC if a memory allocation failed.
 */
ib_status_t DLL_PUBLIC ib_expand_str_ex(ib_mpool_t *mp,
                                        const char *str,
                                        size_t str_len,
                                        const char *prefix,
                                        const char *suffix,
                                        bool nul,
                                        bool recurse,
                                        const ib_hash_t *hash,
                                        char **result,
                                        size_t *result_len);

/**
 * Expand a NUL-terminated string using the given hash, generic/ex version.
 *
 * This function looks through @a str for instances of @a prefix + _name_ + @a
 * suffix (e.g. "%{FOO}"), then attempts to look up each of such name found in
 * @a lookup_data using @a lookup_fn.  If the name is not found, the
 * "%{_name_}" sub-string is replaced with an empty string.  If the name is
 * found, the associated field value is used to replace "%{_name_}" sub-string
 * for string and numeric types (numbers are converted to strings); for
 * others, the replacement is an empty string.
 *
 * @param[in] mp Memory pool
 * @param[in] str String to expand
 * @param[in] str_len Length of @a str
 * @param[in] prefix Prefix string (e.g. "%{")
 * @param[in] suffix Suffix string (e.g. "}")
 * @param[in] nul Append a NUL byte to the end of @a result?
 * @param[in] recurse Do recursive expansion?
 * @param[in] lookup_fn Function to lookup a key in @a lookup_data
 * @param[in] lookup_data Hash-like object in which to expand names in @a str
 * @param[out] result Resulting string
 * @param[out] result_len Length of @a result
 *
 * @returns
 *   - IB_OK on success or if the string is not expandable.
 *   - IB_EINVAL if prefix or suffix is zero length.
 *   - IB_EALLOC if a memory allocation failed.
 */
ib_status_t DLL_PUBLIC ib_expand_str_gen_ex(ib_mpool_t *mp,
                                            const char *str,
                                            size_t str_len,
                                            const char *prefix,
                                            const char *suffix,
                                            bool nul,
                                            bool recurse,
                                            ib_expand_lookup_fn_t lookup_fn,
                                            const void *lookup_data,
                                            char **result,
                                            size_t *result_len);

/**
 * Determine if a string would be expanded by expand_str().
 *
 * This function looks through @a str for instances of
 * @a startpat + _name_ + @a endpat (e.g. "%{FOO}").
 *
 * @param[in] str String to check for expansion
 * @param[in] prefix Prefix string (e.g. "%{")
 * @param[in] suffix Suffix string (e.g. "}")
 * @param[out] result true if @a str would be expanded by expand_str().
 *
 * @returns
 * - IB_OK On success.
 * - IB_EINVAL If prefix or suffix is a zero-length string.
 */
ib_status_t DLL_PUBLIC ib_expand_test_str(const char *str,
                                          const char *prefix,
                                          const char *suffix,
                                          bool *result);

/**
 * Determine if a string would be expanded by expand_str(), ex version.
 *
 * This function looks through @a str for instances of
 * @a startpat + _name_ + @a endpat (e.g. "%{FOO}").
 *
 * @param[in] str String to check for expansion
 * @param[in] str_len Length of @a str
 * @param[in] prefix Prefix string (e.g. "%{")
 * @param[in] suffix Suffix string (e.g. "}")
 * @param[out] result true if @a str would be expanded by expand_str().
 *
 * @returns
 * - IB_OK On success.
 * - IB_EINVAL If prefix or suffix is a zero-length string.
 */
ib_status_t DLL_PUBLIC ib_expand_test_str_ex(const char *str,
                                             size_t str_len,
                                             const char *prefix,
                                             const char *suffix,
                                             bool *result);


/** @} IronBeeUtilExpand */


#ifdef __cplusplus
}
#endif

#endif /* _IB_EXPAND_H_ */
