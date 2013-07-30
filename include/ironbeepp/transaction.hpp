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
 * @brief IronBee++ --- Transaction
 *
 * This file defines (Const)Transaction, a wrapper for ib_tx_t.
 *
 * @remark Developers should be familiar with @ref ironbeepp to understand
 * aspects of this code, e.g., the public/non-virtual inheritance.
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#ifndef __IBPP__TRANSACTION__
#define __IBPP__TRANSACTION__

#include <ironbeepp/abi_compatibility.hpp>
#include <ironbeepp/common_semantics.hpp>
#include <ironbeepp/module.hpp>

#include <ironbee/engine.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif
#include <boost/date_time/posix_time/ptime.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <ostream>

namespace IronBee {

class Engine;
class ConstModule;
class Context;
class MemoryPool;
class Transaction;
class Connection;
class ParsedRequestLine;
class ParsedNameValue;

/**
 * Const Transaction; equivalent to a const pointer to ib_tx_t.
 *
 * Provides operators ==, !=, <, >, <=, >= and evaluation as a boolean for
 * singularity via CommonSemantics.
 *
 * See Transaction for discussion of Transaction
 *
 * @sa Transaction
 * @sa ironbeepp
 * @sa ib_tx_t
 * @nosubgrouping
 **/
class ConstTransaction :
    public CommonSemantics<ConstTransaction>
{
public:
    //! C Type.
    typedef const ib_tx_t* ib_type;

    /**
     * Construct singular ConstTransaction.
     *
     * All behavior of a singular ConstTransaction is undefined except for
     * assignment, copying, comparison, and evaluate-as-bool.
     **/
    ConstTransaction();

    /**
     * @name C Interoperability
     * Methods to access underlying C types.
     **/
    ///@{

    //! const ib_tx_t accessor.
    // Intentionally inlined.
    ib_type ib() const
    {
        return m_ib;
    }

    //! Construct Transaction from ib_tx_t.
    explicit
    ConstTransaction(ib_type ib_transaction);

    ///@}

    //! Associated engine.
    Engine engine() const;

    //! Memory pool used.
    MemoryPool memory_pool() const;

    //! Identifier.
    const char* id() const;

    //! Associated connection.
    Connection connection() const;

    //! Associated connection.
    Context context() const;

    /**
     * @name Timestamps
     * Timestamps.
     **/
    ///@{

    //! Start of transaction.
    boost::posix_time::ptime started_time() const;

    //! Start of request.
    boost::posix_time::ptime request_started_time() const;

    //! Start of request header.
    boost::posix_time::ptime request_header_time() const;

    //! Start of request body.
    boost::posix_time::ptime request_body_time() const;

    //! Finish of request.
    boost::posix_time::ptime request_finished_time() const;

    //! Start of response.
    boost::posix_time::ptime response_started_time() const;

    //! Start of response header.
    boost::posix_time::ptime response_header_time() const;

    //! Start of response body.
    boost::posix_time::ptime response_body_time() const;

    //! Finish of response.
    boost::posix_time::ptime response_finished_time() const;

    //! Start of post processing.
    boost::posix_time::ptime postprocess_time() const;

    //! Start of event logging.
    boost::posix_time::ptime logtime_time() const;

    //! Finish of transaction.
    boost::posix_time::ptime finished_time() const;
    ///@}

    //! Next transaction in current sequence.
    Transaction next() const;

    //! Hostname used in request.
    const char* hostname() const;

    //! Effective remote IP string.
    const char* effective_remote_ip_string() const;

    //! Path used in request.
    const char* path() const;

    //! Parsed request line.
    ParsedRequestLine request_line() const;

    /**
     * Parsed header.
     *
     * This method returns the first parsed header.  Later individual
     * headers can be accessed via ParsedNameValue::next().
     **/
    ParsedNameValue request_header() const;

    /**
     * @name Flags
     * Transaction Flags
     *
     * The masks for the flags are defined by the flags_e enum.  All flags
     * as a set of bits can be accessed via flags().  Individual flags can be
     * checked either via flags() @c & @c flag_X or via @c is_X().
     **/
    ///@{

    //! Possible flags.  Treat as bit masks.
    enum flags_e {
        flag_none                  = IB_TX_FNONE,
        flag_error                 = IB_TX_FERROR,
        flag_http09                = IB_TX_FHTTP09,
        flag_pipelined             = IB_TX_FPIPELINED,
        flag_request_started       = IB_TX_FREQ_STARTED,
        flag_request_seen_header   = IB_TX_FREQ_SEENHEADER,
        flag_request_no_body       = IB_TX_FREQ_NOBODY,
        flag_request_seen_body     = IB_TX_FREQ_SEENBODY,
        flag_request_seen_trailer  = IB_TX_FREQ_SEENTRAILER,
        flag_request_finished      = IB_TX_FREQ_FINISHED,
        flag_response_started      = IB_TX_FRES_STARTED,
        flag_response_seen_header  = IB_TX_FRES_SEENHEADER,
        flag_response_seen_body    = IB_TX_FRES_SEENBODY,
        flag_response_seen_trailer = IB_TX_FRES_SEENTRAILER,
        flag_response_finished     = IB_TX_FRES_FINISHED,
        flag_suspicious            = IB_TX_FSUSPICIOUS
    };

    //! All flags.
    uint32_t flags() const;

    //! flags() & flag_none
    bool is_none() const
    {
        return flags() & flag_none;
    }

    //! flags() & flag_error
    bool is_error() const
    {
        return flags() & flag_error;
    }

    //! flags() & flag_pipelined
    bool is_pipelined() const
    {
        return flags() & flag_pipelined;
    }

    //! flags() & flag_request_started
    bool is_request_started() const
    {
        return flags() & flag_request_started;
    }

    //! flags() & flag_request_seen_header
    bool is_request_seen_header() const
    {
        return flags() & flag_request_seen_header;
    }

    //! flags() & flag_request_no_body
    bool is_request_no_body() const
    {
        return flags() & flag_request_no_body;
    }

    //! flags() & flag_request_seen_body
    bool is_request_seen_body() const
    {
        return flags() & flag_request_seen_body;
    }

    //! flags() & flag_request_finished
    bool is_request_finished() const
    {
        return flags() & flag_request_finished;
    }

    //! flags() & flag_response_started
    bool is_response_started() const
    {
        return flags() & flag_response_started;
    }

    //! flags() & flag_response_seen_header
    bool is_response_seen_header() const
    {
        return flags() & flag_response_seen_header;
    }

    //! flags() & flag_response_seen_body
    bool is_response_seen_body() const
    {
        return flags() & flag_response_seen_body;
    }

    //! flags() & flag_response_finished
    bool is_response_finished() const
    {
        return flags() & flag_response_finished;
    }

    //! flags() & flag_suspicious
    bool is_suspicious() const
    {
        return flags() & flag_suspicious;
    }

    ///@}

private:
    ib_type m_ib;
};

/**
 * Transaction; equivalent to a pointer to ib_tx_t.
 *
 * Transaction can be treated as ConstTransaction.  See @ref ironbeepp for
 * details on IronBee++ object semantics.
 *
 * A transaction is a request/response pair within a connection.  Current
 * C++ support is limited.
 *
 * This class adds no functionality to ConstTransaction beyond providing a
 * non-const @c ib_tx_t* via ib().
 *
 * @sa ConstTransaction
 * @sa ironbeepp
 * @sa ib_tx_t
 * @nosubgrouping
 **/
class Transaction :
    public ConstTransaction
{
public:
    //! C Type.
    typedef ib_tx_t* ib_type;

    /**
     * Remove the constness of a ConstTransaction.
     *
     * @warning This is as dangerous as a @c const_cast, use carefully.
     *
     * @param[in] transaction ConstTransaction to remove const from.
     * @returns Transaction pointing to same underlying transaction as
     *          @a transaction.
     **/
    static Transaction remove_const(ConstTransaction transaction);

    /**
     * Construct singular Transaction.
     *
     * All behavior of a singular Transaction is undefined except for
     * assignment, copying, comparison, and evaluate-as-bool.
     **/
    Transaction();

    /**
     * @name C Interoperability
     * Methods to access underlying C types.
     **/
    ///@{

    //! ib_tx_t accessor.
    ib_type ib() const
    {
        return m_ib;
    }

    //! Construct Transaction from ib_tx_t.
    explicit
    Transaction(ib_type ib_transaction);

    ///@}


    /**
     * Copy @a t into the T module data.
     *
     * ConstTransaction::memory_pool() will be charged with
     * destroying the copy of @a t when the transaction is over.
     *
     * @param[in] m The module to store @a t for.
     * @param[in] t The module data.
     * @throws IronBee errors on C API failures.
     */
    template<typename T>
    void set_module_data(ConstModule m, T t);

    /**
     * Return a reference to the stored module transaction data.
     *
     * @param[in] m The module that the data is stored for.
     * @throws IronBee errors on C API failures.
     */
    template<typename T>
    T& get_module_data(ConstModule m);
    
    /**
     * Create a new transaction.
     *
     * The C API provides a plugin context @c void* parameter for transaction
     * creation.  This is currently unsupported.  It is intended that C++
     * server plugins not need that context.
     *
     * @param[in] connection Connection to associate transaction with.
     * @returns Transaction
     **/
    static
    Transaction create(Connection connection);

    /**
     * Destroy transaction.
     **/
    void destroy() const;

private:
    ib_type m_ib;
};

template<typename T>
void Transaction::set_module_data(ConstModule m, T t) {
    void *v = value_to_data(t, memory_pool().ib());

    throw_if_error(
        ib_tx_set_module_data(ib(), m.ib(), v)
    );
}

template<typename T>
T& Transaction::get_module_data(ConstModule m)
{
    void *v = NULL;

    throw_if_error(
        ib_tx_get_module_data(ib(), m.ib(), &v)
    );

    return data_to_value<T>(v);
}
    

/**
 * Output operator for Transaction.
 *
 * Output IronBee::Transaction[@e value] where @e value is the id.
 *
 * @param[in] o Ostream to output to.
 * @param[in] transaction Transaction to output.
 * @return @a o
 **/
std::ostream& operator<<(
    std::ostream&           o,
    const ConstTransaction& transaction
);

} // IronBee

#endif
