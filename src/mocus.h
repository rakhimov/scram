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

  /// @returns true if there are not literals.
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
  /// @param[in/out] base  Sorted and unique collection.
  void AddUniqueElement(int index, std::vector<int>* base) {
    base->push_back(index);
    std::inplace_merge(base->begin(), --base->end(), base->end());
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
  explicit SimpleGate(Operator type) noexcept : type_(type) {}

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

  /// Sets the limit order for all analysis with simple gates.
  ///
  /// @param[in] limit  The limit order for minimal cut sets.
  static void limit_order(int limit) { limit_order_ = limit; }

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
  static int limit_order_;  ///< The limit on the order of minimal cut sets.
};

}  // namespace mocus

/// @class Mocus
/// This class analyzes normalized, preprocessed, and indexed fault trees
/// to generate minimal cut sets with the MOCUS algorithm.
class Mocus {
 public:
  /// Constructor with the analysis target.
  ///
  /// @param[in] fault_tree  Preprocessed, normalized, and indexed fault tree.
  /// @param[in] settings  The analysis settings.
  explicit Mocus(const BooleanGraph* fault_tree, const Settings& settings);

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
  void CreateSimpleTree(
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

  const BooleanGraph* fault_tree_;  ///< The main fault tree.
  const Settings kSettings_;  ///< Analysis settings.
  std::vector<std::vector<int>> cut_sets_;  ///< Min cut sets with indices.
};

}  // namespace scram

#endif  // SCRAM_SRC_MOCUS_H_
