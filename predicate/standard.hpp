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
 * @brief Predicate --- Standard Nodes
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#ifndef __PREDICATE__STANDARD__
#define __PREDICATE__STANDARD__

#include "dag.hpp"
#include "validate.hpp"

#include "call_factory.hpp"

namespace IronBee {
namespace Predicate {
namespace Standard {

/**
 * Falsy value, null.
 **/
class False :
    public Validate::Call<False>,
    public Validate::NChildren<0>
{
public:
    //! See Call::name()
    virtual std::string name() const;

protected:
    //! See Node::calculate()
    virtual Value calculate(Context);
};

/**
 * Truthy value, ''.
 **/
class True :
    public Validate::Call<True>,
    public Validate::NChildren<0>
{
public:
    //! See Call::name()
    virtual std::string name() const;

protected:
    //! See Node::calculate()
    virtual Value calculate(Context);
};

/**
 * True iff any children are truthy.
 **/
class Or :
    public Validate::Call<Or>,
    public Validate::NOrMoreChildren<2>
{
public:
    //! See Call::name()
    virtual std::string name() const;

protected:
    virtual Value calculate(Context context);
};

/**
 * True iff all children are truthy.
 **/
class And :
    public Validate::Call<And>,
    public Validate::NOrMoreChildren<2>
{
public:
    //! See Call::name()
    virtual std::string name() const;

protected:
    virtual Value calculate(Context context);
};

/**
 * True iff child is falsy.
 **/
class Not :
    public Validate::Call<Not>,
    public Validate::NChildren<1>
{
public:
    //! See Call::name()
    virtual std::string name() const;

protected:
    virtual Value calculate(Context context);
};

/**
 * Load all standard calls into a CallFactory.
 *
 * @param [in] to CallFactory to load into.
 **/
void load(CallFactory& to);

} // Standard
} // Predicate
} // IronBee

#endif
