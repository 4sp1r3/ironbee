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
 * @brief IronBee++ --- ParsedNameValue
 *
 * This file defines (Const)ParsedNameValue, a wrapper for
 * ib_parsed_name_value_pair_list_t.
 *
 * @remark Developers should be familiar with @ref ironbeepp to understand
 * aspects of this code, e.g., the public/non-virtual inheritance.
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#ifndef __IBPP__PARSED_NAME_VALUE__
#define __IBPP__PARSED_NAME_VALUE__

#include <ironbeepp/internal/throw.hpp>

#include <ironbeepp/abi_compatibility.hpp>
#include <ironbeepp/byte_string.hpp>
#include <ironbeepp/common_semantics.hpp>
#include <ironbeepp/transaction.hpp>

#include <ironbee/parsed_content.h>

#include <boost/foreach.hpp>

#include <ostream>

// IronBee C Type
typedef struct ib_parsed_name_value_pair_list_t
     ib_parsed_name_value_pair_list_t;

namespace IronBee {

class Transaction;
class ParsedNameValue;
class MemoryPool;

/**
 * Const ParsedNameValue; equivalent to a const pointer to ib_parsed_name_value_pair_list_t.
 *
 * Provides operators ==, !=, <, >, <=, >= and evaluation as a boolean for
 * singularity via CommonSemantics.
 *
 * See ParsedNameValue for discussion of ParsedNameValue
 *
 * @sa ParsedNameValue
 * @sa ironbeepp
 * @sa ib_parsed_name_value_pair_list_t
 * @nosubgrouping
 **/
class ConstParsedNameValue :
    public CommonSemantics<ConstParsedNameValue>
{
public:
    //! C Type.
    typedef const ib_parsed_name_value_pair_list_t* ib_type;

    /**
     * Construct singular ConstParsedNameValue.
     *
     * All behavior of a singular ConstParsedNameValue is undefined except for
     * assignment, copying, comparison, and evaluate-as-bool.
     **/
    ConstParsedNameValue();

    /**
     * @name C Interoperability
     * Methods to access underlying C types.
     **/
    ///@{

    //! const ib_parsed_name_value_pair_list_t accessor.
    // Intentionally inlined.
    ib_type ib() const
    {
        return m_ib;
    }

    //! Construct ParsedNameValue from ib_parsed_name_value_pair_list_t.
    explicit
    ConstParsedNameValue(ib_type ib_parsed_name_value);

    ///@}

    //! Name.
    ByteString name() const;

    //! Value.
    ByteString value() const;

    //! Next name/value.
    ParsedNameValue next() const;

private:
    ib_type m_ib;
};

/**
 * ParsedNameValue; equivalent to a pointer to
 * ib_parsed_name_value_pair_list_t.
 *
 * ParsedNameValue can be treated as ConstParsedNameValue.  See @ref
 * ironbeepp for details on IronBee++ object semantics.
 *
 * ParsedNameValue is forms a simple linked list of pairs of byte strings.  It
 * is used as part of the parsed content interface which provides a very
 * simple (minimal dependency) API for external input providers.
 *
 * ParsedNameValue adds no functionality to ConstParsedNameValue except for
 * exposing a non-const @c ib_parsed_name_value_pair_list_t* via ib().
 *
 * @sa ConstParsedNameValue
 * @sa ironbeepp
 * @sa ib_parsed_name_value_pair_list_t
 * @nosubgrouping
 **/
class ParsedNameValue :
    public ConstParsedNameValue
{
public:
    //! C Type.
    typedef ib_parsed_name_value_pair_list_t* ib_type;

    /**
     * Remove the constness of a ConstParsedNameValue.
     *
     * @warning This is as dangerous as a @c const_cast, use carefully.
     *
     * @param[in] parsed_name_value ConstParsedNameValue to remove const from.
     * @returns ParsedNameValue pointing to same underlying parsed_name_value
     *          as @a parsed_name_value.
     **/
    static ParsedNameValue remove_const(
        ConstParsedNameValue parsed_name_value
    );

    /**
     * Construct singular ParsedNameValue.
     *
     * All behavior of a singular ParsedNameValue is undefined except for
     * assignment, copying, comparison, and evaluate-as-bool.
     **/
    ParsedNameValue();

    /**
     * @name C Interoperability
     * Methods to access underlying C types.
     **/
    ///@{

    //! ib_parsed_name_value_pair_list_t accessor.
    ib_type ib() const
    {
        return m_ib;
    }

    //! Construct ParsedNameValue from ib_parsed_name_value_pair_list_t.
    explicit
    ParsedNameValue(ib_type ib_parsed_name_value);

    ///@}

    /**
     * Create a ParsedNameValue.
     *
     * @param[in] pool  Memory pool to use for allocations.
     * @param[in] name  Name.
     * @param[in] value Value.
     * @returns ParsedNameValue
     **/
    static
    ParsedNameValue create(
        MemoryPool pool,
        ByteString name,
        ByteString value
    );

private:
    ib_type m_ib;
};

/**
 * Output operator for ParsedNameValue.
 *
 * Output IronBee::ParsedNameValue[@e value] where @e value is the name and
 * value joined by a colon.
 *
 * @param[in] o Ostream to output to.
 * @param[in] parsed_name_value ParsedNameValue to output.
 * @return @a o
 **/
std::ostream& operator<<(
    std::ostream&               o,
    const ConstParsedNameValue& parsed_name_value
);


namespace Internal {
/// @cond Internal

/**
 * Turn a sequence of ParsedNameValues into a C API appropriate type.
 *
 * @param[in] transaction Transaction to associate with.
 * @param[in] begin       Beginning of sequence.
 * @param[in] end         End of sequence.
 * @returns ib_parsed_name_value_pair_list_wrapper_t for use in C API.
 **/
template <typename Iterator>
ib_parsed_name_value_pair_list_wrapper_t* make_pnv_list(
    Transaction transaction,
    Iterator    begin,
    Iterator    end
)
{
    ib_parsed_name_value_pair_list_wrapper_t* ib_pnv_list;
    Internal::throw_if_error(
        ib_parsed_name_value_pair_list_wrapper_create(
            &ib_pnv_list,
            transaction.ib()
        )
    );

    BOOST_FOREACH(
        ParsedNameValue pnv,
        std::make_pair(begin, end)
    ) {
        // This will reconstruct the bytestrings but not copy the data.
        // The C API is currently asymmetric: named values are consumed as
        // structures but added to list as members.  IronBee++ hides that
        // asymmetry.
        Internal::throw_if_error(
            ib_parsed_name_value_pair_list_add(
                ib_pnv_list,
                pnv.name().const_data(),
                pnv.name().length(),
                pnv.value().const_data(),
                pnv.value().length()
            )
        );
    }

    return ib_pnv_list;
}

/// @endcond
} // Internal

} // IronBee

#endif
