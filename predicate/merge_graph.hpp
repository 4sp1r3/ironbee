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
 * @brief Predicate --- MergeGraph
 *
 * Defines MergeGraph, a datastructure for cominging expression trees into a
 * DAG with common subexpressions merged.
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#ifndef __PREDICATE__GRAPH__
#define __PREDICATE__GRAPH__

#include "dag.hpp"

#include <map>
#include <vector>

namespace IronBee {
namespace Predicate {
namespace DAG {

/**
 * A graph of multiple expression trees with common subexpressions merged.
 *
 * This class facilitates the combination and manipulation of expressions
 * trees.  The graph is defined as all nodes reachable from a set of root
 * nodes.  When a new root is added, all desendants of it that are equivalent
 * to nodes already in the graph are replaced with those nodes.  Operations
 * to add, remove, and replace children are provided that similarly preserve
 * the merging of equivalent nodes.
 *
 * There are two important considerations when using this class:
 *
 * 1. Acyclic.  Adding a root defining a graph containing cycles, or
 *    manipulating children in a way that leads to cycles will result in
 *    undefined behavior.
 * 2. Ownership.  Adding a root or a child transfers ownership of it and its
 *    descendants to MergeGraph.  It or its descendants may be modified and
 *    any external modification to it can result in undefined behavior.  Best
 *    practice is discard any existing references once added.
 *
 * Once you are done merging and transforming the DAG, it is recommended that
 * you extract and store the roots and then discard the MergeGraph as it can
 * use significant additional space.
 **/
class MergeGraph
{
private:
    typedef std::map<std::string, node_p> node_by_sexpr_t;
    typedef std::vector<node_p> roots_t;

public:
    //! Iterator through roots.
    typedef roots_t::const_iterator root_iterator;
    //! Pair of @ref root_iterator.
    typedef std::pair<root_iterator, root_iterator> root_iterators;

    /**
     * Add a new tree rooted at @a root.
     *
     * This routine does three related things to the tree with root @a root.
     * 1. Merge common subexpressions.  If any node in the tree is equivalent
     *    to a known node, it is replaced with the known node.
     * 2. Learn new subexpressions.  If any node in the tree is not equivalent
     *    to a known node, it is added to known nodes.
     * 3. Add root.  A new root is added corresponding to the new tree.
     *
     * @warning The tree at @a root should not be modified after adding to
     *          the graph.  Furthermore, it will be modified in place.  Make
     *          a deep copy if you need a pristine original.  To be safe,
     *          discard any remaining references.
     *
     * @param [in, out] root root of tree to add.  May be modified if shares
     *                       common subexpressions with other trees in the
     *                       forest.  May also have additional parents added
     *                       later when other trees are added.
     * @return Index of new root.
     * @throw IronBee::einval if @a root is singular.
     * @throw IronBee::einval if any node in the tree has more than one
     *                        parent or if @a root has any parents.
     **/
    size_t add_root(node_p& root);

    /**
     * Fetch root tree by index.
     *
     * @warning The children or parents of the return node should not be
     *          modified while the MergeGraph remains.  The non-const pointer
     *          is provided to allow post-MergeGraph use and for evaluation.
     *
     * @param[in] index Index of root to fetch.
     * @return Tree root at @a index.
     * @throw IronBee::enoent if no such root.
     **/
    const node_p& root(size_t index) const;

    /**
     * Find index of tree equivalent to @a root.
     *
     * @param[in] root Root of tree to find index of.
     * @return index.
     * @throw IronBee::enoent if no equivalent tree has been added via
     *        add_root().
     * @throw IronBee::einval() if @a root is singular.
     **/
    size_t root_index(const node_cp& root) const;

    /**
     * Replace a node in the forest with another node.
     *
     * This method is semantically similar to:
     * @code
     * for each parent p of which
     *   p->replace_child(which, with)
     * @endcode
     * but also handles common subexpression merging and adding any new
     * subexpressions.
     *
     * @warning See add_root() and MergeGraph for ownership issues regarding
     *          @a with.
     *
     * @param[in] which Which node to replace.  Equivalent must be in graph.
     * @param[in] with  Which node to replace with.
     * @throw IronBee::enoent if @a which has no equivalent node in graph.
     * @throw IronBee::einval if @a which or @a with is singular.
     **/
    void replace(const node_cp& which, node_p& with);

    /**
     * Add @a child to @a parent as a child.
     *
     * This method is semantically similar to:
     * @code
     * parent->add_child(child);
     * @endcode
     * but also handles common subexpression merging and adding any new
     * subexpressions.
     *
     * @warning See add_root() and MergeGraph for ownership issues regarding
     *          @a child.
     *
     * @param[in] parent Parent to add child to.  Equivalent must be in graph.
     * @param[in] child  Child to add.
     * @throw IronBee::enoent if @a parent has no equivalent node in graph.
     * @throw IronBee::einval if @a parent or @a child is singular.
     **/
    void add(const node_cp& parent, node_p& child);

   /**
    * Remove child @a child from @a parent.
    *
    * This method is semantically similar to:
    * @code
    * parent->remove_child(child);
    * @endcode
    * but also handles updating subexpressions.
    *
    * @param[in] parent Parent to remove child from.  Equivalent must be in
    *                   graph.
    * @param[in] child  Child to remove.  Equivalent must be child of
    *                   @a parent.
    * @throw IronBee::enoent if @a parent or @a child has no equivalent node
    *                        in graph.
    * @throw IronBee::einval if @a parent or @a child is singular or @a child
    *                        is not a child of @a parent.
    **/
    void remove(const node_cp& parent, node_cp& child);

    //! Iterate through all root nodes.
    root_iterators roots() const
    {
        return std::make_pair(m_roots.begin(), m_roots.end());
    }

    //! Number of roots.
    size_t size() const
    {
        return m_roots.size();
    }

    //! True iff size() > 0.
    bool empty() const
    {
        return m_roots.empty();
    }

    /**
     * Write debug report to @a out.
     *
     * This routine writes the sexpr to node map (validating it as well),
     * and a complete dot graph.
     *
     * @param[in] out Ostream to write to.
     **/
    void write_debug_report(std::ostream& out);

private:
    /**
     * Merge @a which into graph, merging subexpressions as needed.
     *
     * See add_root() for discussion and warnings.  Also used by replace(),
     * add(), and remove().
     *
     * @param[in, out] which Node to merge in.
     * @throw IroNe:einval if @a which is singular.
     **/
    void merge_tree(node_p& which);

    /**
     * Look for known subexpression equivalent to @a node.
     *
     * @note In the current implementation, common subexpressions are found
     *       via Node::to_s().  This approach is suboptimal in performance
     *       but is simple to code and understand.
     *
     * @param[in] node Root of subexpression to search for.
     * @return root of known equivalent subexpression if known and @c node_p()
     *         otherwise.
     * @throw IronBee::einval() if @a node is singular.
     **/
    node_p known(const node_cp& node) const;

    /**
     * Add a new subexpression to known subexpressions.
     *
     * @param[in] which Subexpression to add.
     * @return true iff a new entry is created and the root of
     *         the added subexpression.  Note that the returned root will
     *         equal @a which only if a new entry is added.
     * @throw IronBee::einval if @a which is singular.
     **/
    std::pair<bool, node_p> learn(const node_p& which);

    /**
     * Remove a subexpression from known subexpressions.
     *
     * @param[in] which Subexpression to unlearn.
     * @return true iff an entry was removed.
     **/
    bool unlearn(const node_cp& which);

    typedef std::map<node_cp, size_t> root_indices_t;

    //! Map of subexpression string to node.
    node_by_sexpr_t m_node_by_sexpr;

    //! Roots as vector, i.e., map of index to root node.
    roots_t m_roots;

    //! Map of root node to index.
    root_indices_t m_root_indices;
};

} // DAG
} // Predicate
} // IronBee

#endif
