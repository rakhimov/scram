/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file mocus.h
/// Fault tree analysis with the MOCUS algorithm.
/// This algorithm requires a fault tree in negation normal form.
/// The fault tree must only contain layered AND and OR gates.
/// All gates must be positive.
/// That is, negations must be pushed down to leaves, basic events.
/// The fault tree should not contain constants or house events.

#ifndef SCRAM_SRC_MOCUS_H_
#define SCRAM_SRC_MOCUS_H_

#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/functional/hash.hpp>

#include "boolean_graph.h"
#include "settings.h"

namespace scram {

namespace mocus {

struct CutSetPtrHash;
struct CutSetPtrEqual;

/// @class CutSet
/// Representation of a cut set for operations in MOCUS.
class CutSet {
  friend struct CutSetPtrHash;
  friend struct CutSetPtrEqual;

 public:
  /// @returns The order of the cut set.
  int order() const { return pos_literals_.size(); }

  /// @returns The number of positive literals including modules.
  int size() const { return pos_literals_.size() + modules_.size(); }

  /// @returns true if there are no literals.
  bool empty() const { return pos_literals_.empty() && modules_.empty(); }

  /// @returns Positive literals in the cut set.
  const std::vector<int>& literals() const { return pos_literals_; }

  /// @returns A set of module indices.
  const std::vector<int>& modules() const { return modules_; }

  /// Pops and removes a module from the cut set.
  ///
  /// @returns Positive index of the popped module.
  ///
  /// @pre There are modules to pop.
  int PopModule() {
    assert(!modules_.empty());
    int ret = modules_.back();
    modules_.pop_back();
    return ret;
  }

  /// Checks for positive literals.
  ///
  /// @param[in] index  Positive index of a basic event.
  ///
  /// @returns true if the set has the literal.
  bool HasPositiveLiteral(int index) const {
    assert(index > 0 && "Non-sanitized index.");
    return std::binary_search(pos_literals_.begin(), pos_literals_.end(),
                              index);
  }

  /// Checks if the set has any of given positive literals.
  ///
  /// @param[in] indices  An ordered set of unique indices.
  ///
  /// @returns true if there is a common literal.
  bool HasPositiveLiteral(const std::vector<int>& indices) const {
    return Intersects(pos_literals_, indices);
  }

  /// Checks that the combination of orders of cut sets
  /// doesn't exceed limits.
  ///
  /// @param[in] indices  An ordered set of unique indices.
  /// @param[in] order  Maximum allowed order.
  ///
  /// @returns true if the joint order exceeds the limit.
  bool CheckJointOrder(const std::vector<int>& indices, int order) const {
    return CheckDifference(indices, order);
  }

  /// Adds a unique literal to the cut set.
  ///
  /// @param[in] index  Positive index of the literal.
  ///
  /// @pre The cut set doesn't have the literal.
  void AddPositiveLiteral(int index) {
    AddUniqueElement(index, &pos_literals_);
  }

  /// Checks for negative literals.
  ///
  /// @param[in] index  Positive index of a basic event.
  ///
  /// @returns true if the set has the complement of the literal.
  bool HasNegativeLiteral(int index) const {
    assert(index > 0 && "Non-sanitized index.");
    return std::binary_search(neg_literals_.begin(), neg_literals_.end(),
                              index);
  }

  /// Checks if the set has any of given negative literals.
  ///
  /// @param[in] indices  An ordered set of unique indices.
  ///
  /// @returns true if there is a common literal.
  bool HasNegativeLiteral(const std::vector<int>& indices) const {
    return Intersects(neg_literals_, indices);
  }

  /// Adds a unique literal to the cut set.
  ///
  /// @param[in] index  Positive index of the complement literal.
  ///
  /// @pre The cut set doesn't have the literal.
  void AddNegativeLiteral(int index) {
    AddUniqueElement(index, &neg_literals_);
  }

  /// Checks for module indices.
  ///
  /// @param[in] index  Positive index of a module gate.
  ///
  /// @returns true if the set has the module.
  bool HasModule(int index) const {
    assert(index > 0 && "Non-sanitized index.");
    return std::binary_search(modules_.begin(), modules_.end(), index);
  }

  /// Checks if the set has any of given module indices.
  ///
  /// @param[in] indices  An ordered set of unique indices.
  ///
  /// @returns true if there is a common module.
  bool HasModule(const std::vector<int>& indices) const {
    return Intersects(modules_, indices);
  }

  /// Adds a module to the cut set.
  ///
  /// @param[in] index  Positive index of the module gate.
  ///
  /// @pre The cut set doesn't have the module.
  void AddModule(int index) { AddUniqueElement(index, &modules_); }

  /// Adds a set of positive literals.
  ///
  /// @param[in] literals  A sorted set of unique positive literals.
  void AddPositiveLiterals(const std::vector<int>& literals) {
    JoinCollection(literals, &pos_literals_, /*modular=*/false);
  }

  /// Adds a set of negative literals.
  ///
  /// @param[in] literals  A sorted set of unique negative literals.
  void AddNegativeLiterals(const std::vector<int>& literals) {
    JoinCollection(literals, &neg_literals_, /*modular=*/false);
  }

  /// Adds a set of modules.
  ///
  /// @param[in] modules  A sorted set of unique module indices.
  void AddModules(const std::vector<int>& modules) {
    JoinCollection(modules, &modules_, /*modular=*/false);
  }

  /// Joins module cut set into this set.
  ///
  /// @param[in] cut_set  Fully analyzed module cut set.
  ///
  /// @pre Module cut sets only have modules in common.
  void JoinModuleCutSet(const CutSet& cut_set) {
    assert((cut_set.neg_literals_.empty() && neg_literals_.empty()) &&
           "Non-sanitized cut sets.");
    JoinCollection(cut_set.modules(), &modules_, /*modular=*/false);
    JoinCollection(cut_set.literals(), &pos_literals_, true);
  }

  /// Removes negative literals.
  /// This must be performed before putting the cut set into the database.
  void Sanitize() { neg_literals_.clear(); }

  /// @param[in] cut_set  Cut set to compare.
  ///
  /// @returns true if this cut set includes the argument set.
  ///
  /// @note Cut set generation semantics are taken into account.
  bool Includes(const CutSet& cut_set) const {
    if (cut_set.pos_literals_.size() > pos_literals_.size()) return false;
    if (cut_set.modules_.size() > modules_.size()) return false;
    return std::includes(pos_literals_.begin(), pos_literals_.end(),
                         cut_set.pos_literals_.begin(),
                         cut_set.pos_literals_.end()) &&
           std::includes(modules_.begin(), modules_.end(),
                         cut_set.modules_.begin(), cut_set.modules_.end());
  }

 private:
  /// Joins a collection of sorted and unique indices into a set.
  ///
  /// @param[in] collection  Sorted and unique collection to add.
  /// @param[in,out] base  Sorted and unique collection.
  /// @param[in] modular  Treat the collection as a modular.
  void JoinCollection(const std::vector<int>& collection,
                      std::vector<int>* base, bool modular) {
    int size = base->size();
    base->insert(base->end(), collection.begin(), collection.end());
    std::inplace_merge(base->begin(), base->begin() + size, base->end());
    if (!modular)
      base->erase(std::unique(base->begin(), base->end()), base->end());
  }

  /// Adds unique element known not to be in the set.
  ///
  /// @param[in] index  The index of the element.
  /// @param[in,out] base  Sorted and unique collection.
  void AddUniqueElement(int index, std::vector<int>* base) {
    base->insert(std::lower_bound(base->begin(), base->end(), index), index);
  }

  /// Checks for intersection of two sets.
  ///
  /// @param[in] set_one  First set.
  /// @param[in] set_two  Second set.
  ///
  /// @returns true if the sets have a common element.
  bool Intersects(const std::vector<int>& set_one,
                  const std::vector<int>& set_two) const {
    if (!CheckIntersection(set_one, set_two)) return false;
    auto first1 = set_one.begin();
    auto last1 = set_one.end();
    auto first2 = set_two.begin();
    auto last2 = set_two.end();
    while (first1 != last1 && first2 != last2) {
      if (*first1 < *first2) {
        ++first1;
      } else if (*first2 < *first1) {
        ++first2;
      } else {
        return true;
      }
    }
    return false;
  }

  /// Checks the size of the joint set doesn't exceed the limit order.
  ///
  /// @param[in] set_one  The set to be joined.
  /// @param[in] order  The limit on order.
  ///
  /// @returns true if the joint order exceeds the limit.
  bool CheckDifference(const std::vector<int>& set_one, int order) const {
    if (set_one.size() + pos_literals_.size() <= order) return false;
    if (!CheckIntersection(set_one, pos_literals_))
      return set_one.size() + pos_literals_.size() > order;
    int count = pos_literals_.size();
    auto first1 = set_one.begin();
    auto last1 = set_one.end();
    auto first2 = pos_literals_.begin();
    auto last2 = pos_literals_.end();
    while (first1 != last1 && first2 != last2) {
      if (*first1 < *first2) {
        ++first1;
        if (++count > order) return true;
      } else if (*first2 < *first1) {
        ++first2;
      } else {
        ++first1;
        ++first2;
      }
    }
    return (count + (last1 - first1)) > order;
  }

  /// Checks if two sets can have intersections.
  ///
  /// @param[in] set_one  First set.
  /// @param[in] set_two  Second set.
  ///
  /// @pre Sets have the same ordering.
  ///
  /// @returns true if the sets can have a common element.
  bool CheckIntersection(const std::vector<int>& set_one,
                         const std::vector<int>& set_two) const {
    return !set_one.empty() && !set_two.empty() &&
           std::max(set_one.front(), set_two.front()) <=
               std::min(set_one.back(), set_two.back());
  }

  std::vector<int> pos_literals_;  ///< Positive indices of basic events.
  std::vector<int> neg_literals_;  ///< Negative indices of basic events.
  std::vector<int> modules_;  ///< Positive indices of modules (gates).
};

/// Pointer to manage cut sets.
using CutSetPtr = std::shared_ptr<CutSet>;

/// @struct CutSetPtrHash
/// Functor for hashing sets given by pointers.
struct CutSetPtrHash
    : public std::unary_function<const CutSetPtr, std::size_t> {
  /// Operator overload for hashing.
  ///
  /// @param[in] cut_set  The pointer to cut_set.
  ///
  /// @returns Hash value of the set.
  std::size_t operator()(const CutSetPtr& cut_set) const noexcept {
    std::size_t seed = 0;
    for (int index : cut_set->pos_literals_) boost::hash_combine(seed, index);
    for (int index : cut_set->neg_literals_) boost::hash_combine(seed, index);
    for (int index : cut_set->modules_) boost::hash_combine(seed, index);
    return seed;
  }
};

/// @struct CutSetPtrEqual
/// Functor for equality test for pointers of sets.
struct CutSetPtrEqual
    : public std::binary_function<const CutSetPtr, const CutSetPtr, bool> {
  /// Operator overload for set equality test.
  ///
  /// @param[in] lhs  The first set.
  /// @param[in] rhs  The second set.
  ///
  /// @returns true if the pointed sets are equal.
  bool operator()(const CutSetPtr& lhs, const CutSetPtr& rhs) const noexcept {
    return lhs->pos_literals_ == rhs->pos_literals_ &&
           lhs->neg_literals_ == rhs->neg_literals_ &&
           lhs->modules_ == rhs->modules_;
  }
};

/// Storage for generated cut sets.
using CutSetContainer =
    std::unordered_set<CutSetPtr, CutSetPtrHash, CutSetPtrEqual>;

/// @class SimpleGate
/// A helper class to be used in MOCUS.
/// This gate represents only positive OR or AND gates
/// with basic event indices
/// and pointers to other simple gates.
/// All the child gates of this gate must be of opposite type.
class SimpleGate {
 public:
  using SimpleGatePtr = std::shared_ptr<SimpleGate>;

  /// @param[in] type  The type of this gate. AND or OR types are expected.
  /// @param[in] limit  The limit order for minimal cut sets.
  SimpleGate(Operator type, int limit) noexcept;

  /// @returns The type of this gate.
  const Operator& type() const { return type_; }

  /// Adds an index of a basic event.
  ///
  /// @param[in] index  Positive or negative index of a basic event.
  ///
  /// @pre The absolute index is unique.
  void AddLiteral(int index) {
    assert(index && "Index can't be 0.");
    if (index > 0) {
      pos_literals_.push_back(index);
    } else {
      neg_literals_.push_back(-index);
    }
  }

  /// Adds a module index at the end of a container.
  /// This function is specifically given to initiate the gate.
  /// Note that modules are treated just like basic events.
  ///
  /// @param[in] index  The index of a module.
  ///
  /// @pre The graph does not have complement modules.
  void AddModule(int index) {
    assert(index > 0);
    modules_.push_back(index);
  }

  /// Add a pointer to an argument gate.
  ///
  /// @param[in] gate  The pointer to the child gate.
  ///
  /// @pre The graph does not have complement gates.
  void AddGate(const SimpleGatePtr& gate) {
    assert(gate->type() == kAndGate || gate->type() == kOrGate);
    assert(gate->type() != type_);
    gates_.push_back(gate);
  }

  /// Prepares the state to meet
  /// the preconditions of analysis.
  ///
  /// @pre All the initialization of arguments is done.
  void SetupForAnalysis() {
    std::sort(pos_literals_.begin(), pos_literals_.end());
    std::sort(neg_literals_.begin(), neg_literals_.end());
    std::sort(modules_.begin(), modules_.end());
  }

  /// Generates cut sets by using a provided set.
  ///
  /// @param[in] cut_set  The base cut set to work with.
  /// @param[out] new_cut_sets  Generated cut sets
  ///                           by adding the gate's children.
  void GenerateCutSets(const CutSetPtr& cut_set,
                       CutSetContainer* new_cut_sets) noexcept;

 private:
  /// Generates cut sets for AND gate children
  /// using already generated sets.
  /// The tree is assumed to be layered with OR children of AND gates.
  ///
  /// @param[in] cut_set  The base cut set to work with.
  /// @param[out] new_cut_sets  Generated cut sets by using the gate's children.
  void AndGateCutSets(const CutSetPtr& cut_set,
                      CutSetContainer* new_cut_sets) noexcept;

  /// Generates cut sets for OR gate children
  /// using already generated sets.
  /// The tree is assumed to be layered with AND children of OR gates.
  ///
  /// @param[in] cut_set  The base cut set to work with.
  /// @param[out] new_cut_sets  Generated cut sets by using the gate's children.
  void OrGateCutSets(const CutSetPtr& cut_set,
                     CutSetContainer* new_cut_sets) noexcept;

  Operator type_;  ///< Type of this gate.
  std::vector<int> pos_literals_;  ///< Positive indices of basic events.
  std::vector<int> neg_literals_;  ///< Negative indices of basic events.
  std::vector<int> modules_;  ///< Container of modules' indices.
  std::vector<SimpleGatePtr> gates_;  ///< Container of child gates.
  int limit_order_;  ///< The limit on the order of minimal cut sets.
};

}  // namespace mocus

/// @class Mocus
/// This class analyzes normalized, preprocessed, and indexed fault trees
/// to generate minimal cut sets with the MOCUS algorithm.
class Mocus {
 public:
  /// Constructs a simple graph representation from Boolean graph.
  ///
  /// @param[in] fault_tree  Preprocessed, normalized, and indexed fault tree.
  /// @param[in] settings  The analysis settings.
  Mocus(const BooleanGraph* fault_tree, const Settings& settings);

  /// Finds minimal cut sets from the initiated fault tree with indices.
  void Analyze();

  /// @returns Generated minimal cut sets with basic event indices.
  const std::vector<std::vector<int>>& cut_sets() const { return cut_sets_; }

 private:
  using IGatePtr = std::shared_ptr<IGate>;
  using SimpleGatePtr = mocus::SimpleGate::SimpleGatePtr;
  using CutSetPtr = mocus::CutSetPtr;
  using CutSet = mocus::CutSet;

  /// Traverses the fault tree to convert gates into simple gates.
  ///
  /// @param[in] gate  The gate to start with.
  /// @param[in,out] processed_gates  Gates turned into simple gates.
  ///
  /// @returns The root for the simple tree.
  SimpleGatePtr CreateSimpleTree(
      const IGatePtr& gate,
      std::unordered_map<int, SimpleGatePtr>* processed_gates) noexcept;

  /// Finds minimal cut sets of a simple gate.
  ///
  /// @param[in] gate  The simple gate as a parent for processing.
  /// @param[out] mcs  Minimal cut sets.
  void AnalyzeSimpleGate(const SimpleGatePtr& gate,
                         std::vector<CutSet>* mcs) noexcept;

  /// Finds minimal cut sets from cut sets.
  /// Reduces unique cut sets to minimal cut sets.
  /// The performance is highly dependent on the passed sets.
  /// If the sets share many events,
  /// it takes more time to remove supersets.
  ///
  /// @param[in] cut_sets  Cut sets with primary events.
  /// @param[in] mcs_lower_order  Reference minimal cut sets of some order.
  /// @param[in] min_order  The order of sets to become minimal.
  /// @param[out] mcs  Min cut sets.
  void MinimizeCutSets(const std::vector<const CutSet*>& cut_sets,
                       const std::vector<CutSet>& mcs_lower_order,
                       int min_order,
                       std::vector<CutSet>* mcs) noexcept;

  bool constant_graph_;  ///< No need for analysis.
  const Settings kSettings_;  ///< Analysis settings.
  SimpleGatePtr root_;  ///< The root of the MOCUS graph.
  std::unordered_map<int, SimpleGatePtr> modules_;  ///< Converted modules.
  std::vector<std::vector<int>> cut_sets_;  ///< Min cut sets with indices.
};

}  // namespace scram

#endif  // SCRAM_SRC_MOCUS_H_
