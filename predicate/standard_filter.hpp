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
 * @brief Predicate --- Standard Filter
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#ifndef __PREDICATE__STANDARD_FILTER__
#define __PREDICATE__STANDARD_FILTER__

#include <predicate/call_factory.hpp>
#include <predicate/dag.hpp>
#include <predicate/meta_call.hpp>

namespace IronBee {
namespace Predicate {
namespace Standard {

/**
 * Output elements of second child that are equal to first (simple) child.
 **/
class Eq :
    public MapCall
{
public:
    //! See Call::name()
    virtual std::string name() const;

    //! See Node::validate().
    virtual bool validate(NodeReporter reporter) const;

protected:
    //! See MapCall::value_calculate()
    virtual Value value_calculate(Value v, EvalContext context);

    //! See Node::calculate()
    virtual void calculate(EvalContext context);
};

/**
 * Output elements of second child that are not equal to first (simple) child.
 **/
class Ne :
    public MapCall
{
public:
    //! See Call::name()
    virtual std::string name() const;

    //! See Node::validate().
    virtual bool validate(NodeReporter reporter) const;

protected:
    //! See MapCall::value_calculate()
    virtual Value value_calculate(Value v, EvalContext context);

    //! See Node::calculate()
    virtual void calculate(EvalContext context);
};

/**
 * Output elements of second child that the (simple) first child is less than.
 **/
class Lt :
    public MapCall
{
public:
    //! See Call::name()
    virtual std::string name() const;

    //! See Node::validate().
    virtual bool validate(NodeReporter reporter) const;

protected:
    //! See MapCall::value_calculate()
    virtual Value value_calculate(Value v, EvalContext context);

    //! See Node::calculate()
    virtual void calculate(EvalContext context);
};

/**
 * Output elements of second child that the (simple) first child is less than
 * or equal to.
 **/
class Le :
    public MapCall
{
public:
    //! See Call::name()
    virtual std::string name() const;

    //! See Node::validate().
    virtual bool validate(NodeReporter reporter) const;

protected:
    //! See MapCall::value_calculate()
    virtual Value value_calculate(Value v, EvalContext context);

    //! See Node::calculate()
    virtual void calculate(EvalContext context);
};

/**
 * Output elements of second child that the (simple) first child is greater
 * than.
 **/
class Gt :
    public MapCall
{
public:
    //! See Call::name()
    virtual std::string name() const;

    //! See Node::validate().
    virtual bool validate(NodeReporter reporter) const;

protected:
    //! See MapCall::value_calculate()
    virtual Value value_calculate(Value v, EvalContext context);

    //! See Node::calculate()
    virtual void calculate(EvalContext context);
};

/**
 * Output elements of second child that the (simple) first child is greater
 * than or equal to.
 **/
class Ge :
    public MapCall
{
public:
    //! See Call::name()
    virtual std::string name() const;

    //! See Node::validate().
    virtual bool validate(NodeReporter reporter) const;

protected:
    //! See MapCall::value_calculate()
    virtual Value value_calculate(Value v, EvalContext context);

    //! See Node::calculate()
    virtual void calculate(EvalContext context);
};

/**
 * Output elements of second child of type described by first child.
 **/
class Typed :
    public MapCall
{
public:
    //! Constructor.
    Typed();

    //! See Call::name()
    virtual std::string name() const;

    //! See Node::validate().
    virtual bool validate(NodeReporter reporter) const;

    //! See Node::pre_eval().
    virtual void pre_eval(Environment environment, NodeReporter reporter);

protected:
    //! See MapCall::value_calculate()
    virtual Value value_calculate(Value v, EvalContext context);

    //! See Node::calculate()
    virtual void calculate(EvalContext context);

private:
    //! Hidden complex implementation details.
    struct data_t;

    //! Hidden complex implementation details.
    boost::scoped_ptr<data_t> m_data;
};

/**
 * Output elements of second child with name described by first child.
 **/
class Named :
    public MapCall
{
public:
    //! See Call::name()
    virtual std::string name() const;

    //! See Node::validate().
    virtual bool validate(NodeReporter reporter) const;

protected:
    //! See MapCall::value_calculate()
    virtual Value value_calculate(Value v, EvalContext context);

    //! See Node::calculate()
    virtual void calculate(EvalContext context);
};

/**
 * Load all standard filter calls into a CallFactory.
 *
 * @param [in] to CallFactory to load into.
 **/
void load_filter(CallFactory& to);

} // Standard
} // Predicate
} // IronBee

#endif
