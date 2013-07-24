Title:  IronBee Predicate Rules Manual
Author: Christopher Alfeld <calfeld@qualys.com>

Predicates Notes: Templates
===========================

Christopher Alfeld <calfeld@qualys.com><br>

Introduction
------------

Predicate includes a feature called templates which allows users to define new Predicate functions in terms of existing functions via simple substitutions.  Such templating can also be done in the frontend directly.  This document explores both approaches and their differences.

As a running example, consider a function that takes a header name and a length and is true if the value of that header is longer than the length.  In the front end, we could write:

    local function longHeader(name, length)
      return P.Gt(P.Field('REQUEST_HEADERS'):sub(name):length(), length)
    end

The `longHeader` function could then be called in a predicate rule:

    predicate(longHeader('Content-Type', 1000))

Which would provide the sexpr:

    (gt (transformation 'length' (sub 'Content-Type' (field 'REQUEST_HEADERS'))) 1000)

To the Predicate backend.

The same functionality could, instead, be done via templates:

    P.define('longHeader', {'name', 'length'},
      P.Gt(P.Field('REQUEST_HEADERS'):sub(P.Ref('name')):length(), P.Ref('length'))
    )

The define function would, when run in IronBee, call the `PredicateDefine` directive, giving it the following, templated sexpr:

    (gt (transformation 'length' (sub (ref 'name') (field 'REQUEST_HEADERS'))) (ref 'length'))

It would also add `P.longHeader` as a function, allowing us to use it in a rule as:

    predicate(P.longHeader('Content-Type', 1000))

Which would provide the sexpr:

    (longHeader 'Content-Type' 1000)

To the Predicate backend.  At the transformation phase, the Predicate backend would transform the corresponding node into the expanded form, replacing all `(ref 'name')` nodes with `'content-type'` and `(ref 'length')` with `1000`, resulting in

    (gt (transformation 'length' (sub 'Content-Type' (field 'REQUEST_HEADERS'))) 1000)

The same as in the first approach.

Thus, after the transform phase, both approaches yield the same graph.

The second approach, using templates, has two main advantages:

1. Used properly it can significantly reduce the complexity of the sexprs sent to the Predicate backend.  Templates can be used in other templates, so for complex and much used logic, the savings can add up rapidly and provide noticeably reduced memory usage.
2. The pre-transform graph is both simpler and more meaningful, as general concepts such as "long header" are represented as calls rather than extensive expressions.  This allows for more meaningful error messages,  introspection, and easier debugging.

The main disadvantages of using templates are:

1. It adds a name to the call namespace and Predicate module.  Managing this, effectively global, namespace can be challenging, especially if templates are being added by multiple pieces of code.  See Scoping below.
2. There is a small amount of overhead associated with calling the directive.  This overhead is usually negligible, but can be significant if programmatically generating templates that are then used a small number of times.

Templates also have the limitation of only allowing for direct substitution whereas Lua functions can do more complex computations for generating the sexpr.  As a simple example, for the Lua function approach, we could make the length optional:

    local function longHeader(name, length)
      length = length or 1000
      return P.Gt(P.Field('REQUEST_HEADERS'):sub(name):length(), length)
    end

A similar effect could be done with the template, e.g., via.

    P.define('longHeader', {'name', 'length'},
      P.Gt(
        P.Field('REQUEST_HEADERS'):sub(P.Ref('name')):length(),
        P.If(P.IsNull(P.Ref('length')), 1000, P.Ref('length'))
      )
    )

And then called via `P.longHeader('Content-Length', P.Null)`, but this is not as elegant as the Lua approach.

Scoping
-------

There is effectively a single namespace for templates names which is shared with all other defined Predicate functions.  Managing this namespace when defining templates requires care.

One approach is to, for each collection (e.g., file) of templates, choose a unique prefix to used for all templates names.  `P.define` returns a function that generates the appropriate call.  This returned function can be stored for easy alternate access.  For example, for a Lua module of templates, the returned function could be stored in the module table; for a file primarily defining rules that wants to use templates, the returned function could be stored in local variables.

E.g.,

    local longHeader = P.define('templateNotes_longHeader', {'length'},
      P.Gt(P.Field('REQUEST_HEADERS'):sub(P.Ref('name')):length(), P.Ref('length'))
    )

And then use as

    predicate(longHeader('Content-Length', 1000))

With the corresponding sexpr:

    (templateNotes_longHeader 'Content-Length' 1000)

Recommendations
---------------

It is suggested that rule writers default to using templates for common pieces of logic.  Doing so keeps the complexity of front end to back end communication down and allows for superior error messages and debugging.  The directive costs are, except in unusual cases, negligible.  However, where a Lua function would allow for greater expressiveness, it should be used instead.

For each body (e.g., file) of templates, choose a globally unique prefix to use for all template names.  Store the return of `P.define` either in the module table (if the file defines a library of templates) or in local variables (if the file also defines rules).
