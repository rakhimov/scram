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

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/container/flat_set.hpp>
#include <boost/functional/hash.hpp>

#include "boolean_graph.h"
#include "settings.h"

namespace scram {

using Set = boost::container::flat_set<int>;
using SetPtr = std::shared_ptr<Set>;

/// @struct SetPtrHash
/// Functor for hashing sets given by pointers.
struct SetPtrHash
    : public std::unary_function<const SetPtr, std::size_t> {
  /// Operator overload for hashing.
  ///
  /// @param[in] set  The pointer to set which hash must be calculated.
  ///
  /// @returns Hash value of the set.
  std::size_t operator()(const SetPtr& set) const noexcept {
    return boost::hash_range(set->begin(), set->end());
  }
};

/// @struct SetPtrEqual
/// Functor for equality test for pointers of sets.
struct SetPtrEqual
    : public std::binary_function<const SetPtr, const SetPtr, bool> {
  /// Operator overload for set equality test.
  ///
  /// @param[in] lhs  The first set.
  /// @param[in] rhs  The second set.
  ///
  /// @returns true if the pointed sets are equal.
  bool operator()(const SetPtr& lhs, const SetPtr& rhs) const noexcept {
    return *lhs == *rhs;
  }
};

/// @class SetPtrComp
/// Functor for set pointer comparison.
struct SetPtrComp
    : public std::binary_function<const SetPtr, const SetPtr, bool> {
  /// Operator overload.
  /// Compares sets for sorting.
  ///
  /// @param[in] lhs  Pointer to a set.
  /// @param[in] rhs  Pointer to a set.
  ///
  /// @returns true if the lhs pointed set is less than the rhs pointed set.
  bool operator()(const SetPtr& lhs, const SetPtr& rhs) const noexcept {
    return *lhs < *rhs;
  }
};

/// @class SimpleGate
/// A helper class to be used in MOCUS.
/// This gate represents only positive OR or AND gates
/// with basic event indices
/// and pointers to other simple gates.
/// All the child gates of this gate must be of opposite type.
class SimpleGate {
 public:
  using SimpleGatePtr = std::shared_ptr<SimpleGate>;
  using HashSet = std::unordered_set<SetPtr, SetPtrHash, SetPtrEqual>;

  /// @param[in] type  The type of this gate. AND or OR types are expected.
  explicit SimpleGate(const Operator& type) noexcept : type_(type) {}

  /// @returns The type of this gate.
  const Operator& type() const { return type_; }

  /// Adds a basic event index at the end of a container.
  /// This function is specifically given to initiate the gate.
  ///
  /// @param[in] index  The index of a basic event.
  void InitiateWithBasic(int index) { basic_events_.push_back(index); }

  /// Adds a module index at the end of a container.
  /// This function is specifically given to initiate the gate.
  /// Note that modules are treated just like basic events.
  ///
  /// @param[in] index  The index of a module.
  void InitiateWithModule(int index) {
    assert(index > 0);
    modules_.push_back(index);
  }

  /// Add a pointer to a child gate.
  /// This function assumes that the tree does not have complement gates.
  ///
  /// @param[in] gate  The pointer to the child gate.
  void AddChildGate(const SimpleGatePtr& gate) {
    assert(gate->type() == kAndGate || gate->type() == kOrGate);
    assert(gate->type() != type_);
    gates_.push_back(gate);
  }

  /// Generates cut sets by using a provided set.
  ///
  /// @param[in] cut_set  The base cut set to work with.
  /// @param[out] new_cut_sets  Generated cut sets
  ///                           by adding the gate's children.
  void GenerateCutSets(const SetPtr& cut_set, HashSet* new_cut_sets) noexcept;

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
  void AndGateCutSets(const SetPtr& cut_set, HashSet* new_cut_sets) noexcept;

  /// Generates cut sets for OR gate children
  /// using already generated sets.
  /// The tree is assumed to be layered with AND children of OR gates.
  ///
  /// @param[in] cut_set  The base cut set to work with.
  /// @param[out] new_cut_sets  Generated cut sets by using the gate's children.
  void OrGateCutSets(const SetPtr& cut_set, HashSet* new_cut_sets) noexcept;

  Operator type_;  ///< Type of this gate.
  std::vector<int> basic_events_;  ///< Container of basic events' indices.
  std::vector<int> modules_;  ///< Container of modules' indices.
  std::vector<SimpleGatePtr> gates_;  ///< Container of child gates.
  static int limit_order_;  ///< The limit on the order of minimal cut sets.
};

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
  using SimpleGatePtr = std::shared_ptr<SimpleGate>;
  using IGatePtr = std::shared_ptr<IGate>;

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
                         std::vector<Set>* mcs) noexcept;

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
  ///
  /// @note T_avg(N^3 + N^2*logN + N*logN) = O_avg(N^3)
  void MinimizeCutSets(const std::vector<const Set*>& cut_sets,
                       const std::vector<Set>& mcs_lower_order,
                       int min_order,
                       std::vector<Set>* mcs) noexcept;

  const BooleanGraph* fault_tree_;  ///< The main fault tree.
  std::vector<std::vector<int>> cut_sets_;  ///< Min cut sets with indices.
  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;
};

}  // namespace scram

#endif  // SCRAM_SRC_MOCUS_H_
