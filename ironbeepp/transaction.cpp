#include <ironbeepp/transaction.hpp>
#include <ironbeepp/connection.hpp>
#include <ironbeepp/context.hpp>
#include <ironbeepp/engine.hpp>
#include <ironbeepp/memory_pool.hpp>
#include <ironbeepp/clock.hpp>
#include <ironbeepp/parsed_request_line.hpp>
#include <ironbeepp/parsed_name_value.hpp>
#include <ironbeepp/throw.hpp>

namespace IronBee {

// ConstTransaction

ConstTransaction::ConstTransaction() :
    m_ib(NULL)
{
    // nop
}

ConstTransaction::ConstTransaction(ib_type ib_transaction) :
    m_ib(ib_transaction)
{
    // nop
}

Engine ConstTransaction::engine() const
{
    return Engine(ib()->ib);
}

MemoryPool ConstTransaction::memory_pool() const
{
    return MemoryPool(ib()->mp);
}

const char* ConstTransaction::id() const
{
    return ib()->id;
}

Connection ConstTransaction::connection() const
{
    return Connection(ib()->conn);
}

Context ConstTransaction::context() const
{
    return Context(ib()->ctx);
}

boost::posix_time::ptime ConstTransaction::started_time() const
{
    return ib_to_ptime(ib()->tv_created);
}

boost::posix_time::ptime ConstTransaction::request_started_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.request_started - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::request_header_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.request_header - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::request_body_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.request_body - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::request_finished_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.request_finished - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::response_started_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.response_started - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::response_header_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.response_header - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::response_body_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.response_body - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::response_finished_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.response_finished - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::postprocess_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.postprocess - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::logtime_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.logtime - ib()->t.started));
}

boost::posix_time::ptime ConstTransaction::finished_time() const
{
    return ib_to_ptime(ib()->tv_created,
                       (ib()->t.finished - ib()->t.started));
}

Transaction ConstTransaction::next() const
{
    return Transaction(ib()->next);
}

const char* ConstTransaction::hostname() const
{
    return ib()->hostname;
}

const char* ConstTransaction::effective_remote_ip_string() const
{
    return ib()->er_ipstr;
}

const char* ConstTransaction::path() const
{
    return ib()->path;
}

uint32_t ConstTransaction::flags() const
{
    return ib()->flags;
}

ParsedRequestLine ConstTransaction::request_line() const
{
    return ParsedRequestLine(ib()->request_line);
}

ParsedNameValue ConstTransaction::request_header() const
{
    return ParsedNameValue(ib()->request_header->head);
}

// Transaction

Transaction Transaction::remove_const(ConstTransaction transaction)
{
    return Transaction(const_cast<ib_type>(transaction.ib()));
}

Transaction::Transaction() :
    m_ib(NULL)
{
    // nop
}

Transaction::Transaction(ib_type ib_transaction) :
    ConstTransaction(ib_transaction),
    m_ib(ib_transaction)
{
    // nop
}

Transaction Transaction::create(Connection connection)
{
    ib_tx_t* ib_tx;
    throw_if_error(
        ib_tx_create(&ib_tx, connection.ib(), NULL)
    );

    return Transaction(ib_tx);
}

void Transaction::destroy() const
{
    ib_tx_destroy(ib());
}

std::ostream& operator<<(std::ostream& o, const ConstTransaction& transaction)
{
    if (! transaction) {
        o << "IronBee::Transaction[!singular!]";
    } else {
        o << "IronBee::Transaction[" << transaction.id() << "]";
    }
    return o;
}

} // IronBee
