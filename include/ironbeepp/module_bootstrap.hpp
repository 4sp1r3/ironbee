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
 * @brief IronBee++ &mdash; Module Bootstrap
 *
 * This file defines macros to aid in building an IronBee module using
 * IronBee++.  If you are interested in IronBee::Module, the analogue to
 * ib_module_t, see module.hpp instead.
 *
 * To write a new module, include this file in a source file and call either
 * IBPP_BOOTSTRAP_MODULE() or IBPP_BOOTSTRAP_MODULE_DELEGATE().  Then compile
 * that file (and any dependencies) into a shared library.  You can then load
 * shared library into IronBee via a @c LoadModule configuration directive.
 *
 * The different macros provide for two different approaches to writing
 * modules:
 *
 * IBPP_BOOTSTRAP_MODULE() takes a function to call on module load.  That
 * function is passed an IronBee::Module reference and can then set up
 * whatever hooks it needs, e.g., via IronBee::Module::set_context_open().
 *
 * IBPP_BOOTSTRAP_MODULE_DELEGATE() supports an object oriented approach.
 * Instead of a function, it takes the name of a class.  An instance of the
 * class is constructed on module *loading* and destructed on module
 * destruction.  The module hooks are automatically mapped to methods of the
 * class.  Any class with the correct constructor and methods can be used.  A
 * class, ModuleDelegate, is provided with default nop behavior for all hooks.
 * ModuleDelegate can be subclassed to provide an appropriate delegate.
 * However, it is not required.
 *
 * The methods a delegate must define are:
 *
 * - @c constructor(@c IronBee::Module @a module) &mdash; The constructor is
 *   the only time the module is provided.  If subclassing ModuleDelegate, it
 *   should be passed to the parent constructor which will make it available
 *   via ModuleDelegate::module().  If not subclassing, you should store it
 *   in a member variable if needed later.  The lifetime of @a module will
 *   exceed that of the delegate.  Use the constructor to call any methods
 *   of module that you need to, e.g., to set up a configuration map.  Use
 *   @c initialize() (see below) to interact with the engine, e.g., to set
 *   up hooks.
 * - @c destructor &mdash; The destructor will be called when the module is
 *   destroyed.  A default destructor is acceptable.
 * - @c initialize() &mdash; @c initialize is called when the engine
 *   initializes the module.  This is where you should set up hooks or
 *   otherwise do initial interaction with the engine.
 * - @c context_open(@c IronBee::Context @a context) &mdash; @c
 *   context_open is called whenever a new context is opened.  The lifetime
 *   of @a context is until just after the corresponding @c context_destroy
 *   is called.
 * - @c context_close(@c IronBee::Context @a context) &mdash; As above, but
 *   for the context closing.
 * - @c context_destroy(@c IronBee::Context @a context) &mdash; As above,
 *   but for the context being destroyed.
 *
 * Any exceptions thrown in your code, will be translated into, when possible,
 * log error message, and appropriate ib_status_t values.  To assist and
 * control this process, use the exceptions defined in exception.hpp.
 *
 * Example file using a delegate:
 * @code
 * #include "my_module_delegate.hpp"
 * #include <ironbeepp/module_bootstrap.hpp>
 *
 * IBPP_BOOTSTRAP_MODULE_DELEGATE("my_module", MyModuleDelegate);
 * @endcode
 *
 * Example file using an on load function:
 * @code
 * #include "my_module_implementation.hpp"
 * #include <ironbeepp/module_bootstrap.hpp>
 *
 * IBPP_BOOTSTRAP_MODULE("my_module", MyModule::on_load);
 * @endcode
 *
 * @sa exception.hpp
 * @sa module.hpp
 * @sa module_delegate.hpp
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 **/

#ifndef __IBPP__MODULE_BOOTSTRAP__
#define __IBPP__MODULE_BOOTSTRAP__

#include <ironbeepp/internal/catch.hpp>

#include <ironbeepp/abi_compatibility.hpp>
#include <ironbeepp/context.hpp>
#include <ironbeepp/engine.hpp>
#include <ironbeepp/module.hpp>

#include <ironbee/module.h>

#include <boost/bind.hpp>

#include <cassert>

namespace IronBee {
namespace Internal {
/// @cond Internal

/**
 * Context open handler for delegate.  Forwards to delegate.
 *
 * @tparam DelegateType Type of @a *delegate.
 * @param[in] delegate Pointer to delegate.
 * @param[in] module   Module.
 * @param[in] context  Context.
 **/
template <typename DelegateType>
void delegate_context_open(
    DelegateType* delegate,
    Module        module,
    Context       context
)
{
    delegate->context_open(context);
}

/**
 * Context close handler for delegate.  Forwards to delegate.
 *
 * @tparam DelegateType Type of @a *delegate.
 * @param[in] delegate Pointer to delegate.
 * @param[in] module   Module.
 * @param[in] context  Context.
 **/
template <typename DelegateType>
void delegate_context_close(
    DelegateType* delegate,
    Module        module,
    Context       context
)
{
    delegate->context_close(context);
}

/**
 * Context destroy handler for delegate.  Forwards to delegate.
 *
 * @tparam DelegateType Type of @a *delegate.
 * @param[in] delegate Pointer to delegate.
 * @param[in] module   Module.
 * @param[in] context  Context.
 **/
template <typename DelegateType>
void delegate_context_destroy(
    DelegateType* delegate,
    Module        module,
    Context       context
)
{
    delegate->context_destroy(context);
}

/**
 * Finalizer for delegates &mdash; destroys delegate.
 *
 * This is called at module finalization.  It destroys the delegate causing
 * the destructor to be called.
 *
 * @tparam DelegateType Type of @a *delegate.
 * @param[in,out] delegate Delegate to destroy.
 * @param[in]     module   Ignored.
 **/
template <typename DelegateType>
void delegate_finalize(
    DelegateType* delegate,
    Module        module
)
{
    delete delegate;
}

/**
 * Initialize handler for delegate.  Forwards to delegate.
 *
 * @tparam DelegateType Type of @a *delegate.
 * @param[in] delegate Pointer to delegate.
 * @param[in] module   Ignored.
 **/
template <typename DelegateType>
void delegate_initialize(
    DelegateType* delegate,
    Module        module
)
{
    delegate->initialize();
}

/**
 * @c on_load handlers for delegates.
 *
 * Constructs delegate and connects hooks to delegate.
 *
 * @tparam DelegateType Type of module delegate.
 * @param[in] module Module being loaded.
 **/
template <typename DelegateType>
void delegate_on_load(
    Module module
)
{
    DelegateType* delegate = new DelegateType(module);

    module.set_initialize(boost::bind(
        delegate_initialize<DelegateType>,
        delegate,
        _1
    ));
    module.set_context_open(boost::bind(
        delegate_context_open<DelegateType>,
        delegate,
        _1,
        _2
    ));
    module.set_context_close(boost::bind(
        delegate_context_close<DelegateType>,
        delegate,
        _1,
        _2
    ));
    module.set_context_destroy(boost::bind(
        delegate_context_destroy<DelegateType>,
        delegate,
        _1,
        _2
    ));

    // Note this one is different!
    module.set_finalize(boost::bind(
        delegate_finalize<DelegateType>,
        delegate,
        _1
    ));

}

/**
 * Fill in the ib_module_t.
 *
 * This function will fill in @a ib_module using the other arguments.
 *
 * @remark A major purpose of this function is to move the code flow out of
 * preprocessor macros and into a proper C++ function call.  The call to this
 * function is wrapped in try/catch and the function is defined in  an
 * unpolluted C++ environment.
 *
 * @warning @a name and @a filename should be string literals if possible.  In
 * particular, their lifetime must exceed that of the module.
 *
 * @param[in] ib_engine Engine handle.
 * @param[in] ib_module ib_module_t to fill in.
 * @param[in] name      Name of module (stored in
 *                      @a ib_module_t->@c name).
 * @param[in] filename  Name of file defining module (stored in
 *                      @a ib_module_t->@c filename).
 **/
void bootstrap_module(
    ib_engine_t* ib_engine,
    ib_module_t& ib_module,
    const char*  name,
    const char*  filename
);

/// @endcond
} // Internal
} // IronBee

/**
 * Establish file as the loading point for a module.
 *
 * When the module is loaded, the @a on_load function will be called with an
 * IronBee::Module reference as the only argument.
 *
 * @sa IBPP_BOOTSTRAP_MODULE_DELEGATE
 *
 * @param[in] name    Name of module.  String literal.
 * @param[in] on_load Name of function to call on module load.  Should be
 *                    @c void(@c IronBee::Module)
 **/
#define IBPP_BOOTSTRAP_MODULE(name, on_load) \
extern "C" { \
ib_module_t* IB_MODULE_SYM(ib_engine_t* ib) \
{ \
    static ib_module_t ib_module; \
    try { \
        ::IronBee::Internal::bootstrap_module(\
            ib, \
            ib_module, \
            name, \
            __FILE__ \
        ); \
        on_load(::IronBee::Module(&ib_module)); \
    } \
    catch (...) { \
        ::IronBee::Internal::convert_exception(); \
        return NULL; \
    } \
    return &ib_module; \
} \
}

/**
 * Establish file as the loading point for a module.
 *
 * When the module is loaded, a instance of @a delegate_type will be created,
 * passing an IronBee::Module reference as the only argument to the
 * constructor.  The instance will be destroyed when the module is destroyed.
 * All module hooks are mapped the the virtual functions of ModuleDelegate
 * and can be overridden by the child.
 *
 * @sa IBPP_BOOTSTRAP_MODULE
 * @sa ModuleDelegate
 *
 * @param[in] name          Name of module.  String literal.
 * @param[in] delegate_type Name of a child class of ModuleDelegate to use
 *                          for hooks.
 **/
#define IBPP_BOOTSTRAP_MODULE_DELEGATE(name, delegate_type) \
    IBPP_BOOTSTRAP_MODULE(\
        (name), \
        ::IronBee::Internal::delegate_on_load<delegate_type> \
    )

#endif
