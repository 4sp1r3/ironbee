#include <ironbeepp/configuration_parser.hpp>
#include <ironbeepp/engine.hpp>
#include <ironbeepp/site.hpp>
#include <ironbeepp/context.hpp>

#include <ironbee/config.h>

using namespace std;

namespace IronBee {

// ConstConfigurationParser
ConstConfigurationParser::ConstConfigurationParser() :
    m_ib(NULL)
{
    // nop
}

ConstConfigurationParser::ConstConfigurationParser(
    ib_type ib_configuration_parser
) :
    m_ib(ib_configuration_parser)
{
    // nop
}

Engine ConstConfigurationParser::engine() const
{
    return Engine(ib()->ib);
}

MemoryPool ConstConfigurationParser::memory_pool() const
{
    return MemoryPool(ib()->mp);
}

Context ConstConfigurationParser::current_context() const
{
    return Context(ib()->cur_ctx);
}

const char* ConstConfigurationParser::current_file() const
{
    return ib()->curr->file;
}

const char* ConstConfigurationParser::current_block_name() const
{
    return ib()->curr->directive;
}

// ConfigurationParser

ConfigurationParser ConfigurationParser::create(Engine engine)
{
    ib_cfgparser_t* ib_cp = NULL;
    throw_if_error(
        ib_cfgparser_create(&ib_cp, engine.ib())
    );
    throw_if_error(
        ib_engine_config_started(engine.ib(), ib_cp)
    );
    return ConfigurationParser(ib_cp);
}

ConfigurationParser ConfigurationParser::remove_const(
     ConstConfigurationParser configuration_parser
)
{
    return ConfigurationParser(
        const_cast<ib_type>(configuration_parser.ib())
    );
}

ConfigurationParser::ConfigurationParser() :
    m_ib(NULL)
{
    // nop
}

ConfigurationParser::ConfigurationParser(ib_type ib_configuration_parser) :
    ConstConfigurationParser(ib_configuration_parser),
    m_ib(ib_configuration_parser)
{
    // nop
}

void ConfigurationParser::parse_file(const string& path) const
{
    throw_if_error(
        ib_cfgparser_parse(ib(), path.c_str())
    );
}
void ConfigurationParser::parse_buffer(
    const char* buffer,
    size_t      length,
    const char* file,
    unsigned    lineno,
    bool        more
) const
{
    throw_if_error(
        ib_cfgparser_parse_buffer(
            ib(),
            buffer, length,
            file, lineno,
            more
        )
    );
}

void ConfigurationParser::parse_buffer(const std::string& s,
                                       const std::string& file,
                                       unsigned lineno,
                                       bool more) const
{
    parse_buffer(s.data(), s.length(), file.c_str(), lineno, more);
}

void ConfigurationParser::destroy() const
{
    ib_engine_config_finished(engine().ib());
    ib_cfgparser_destroy(ib());
}

ostream& operator<<(
    ostream& o,
    const ConstConfigurationParser& configuration_parser
)
{
    if (! configuration_parser) {
        o << "IronBee::ConfigurationParser[!singular!]";
    }
    else {
        o << "IronBee::ConfigurationParser["
          << configuration_parser.current_file() << ":"
          << configuration_parser.current_block_name()
          << "]";
    }
    return o;
}

} // IronBee
