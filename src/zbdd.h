/*
 * Copyright (C) 2015 Olzhas Rakhimov
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

/// @file zbdd.h
/// Zero-Suppressed Binary Decision Diagram facilities.

#ifndef SCRAM_SRC_ZBDD_H_
#define SCRAM_SRC_ZBDD_H_

#include "bdd.h"

namespace scram {

/// @class SetNode
/// Representation of non-terminal nodes in ZBDD.
class SetNode : public NonTerminal<SetNode> {
 public:
  using TerminalPtr = std::shared_ptr<Terminal>;
  using ItePtr = std::shared_ptr<Ite>;
  using SetNodePtr = std::shared_ptr<SetNode>;

  using NonTerminal::NonTerminal;  ///< Constructor with index and order.

  /// Converts terminal ROBDD vertex
  /// into ZBDD SetNode terminals.
  ///
  /// @param[in] ite If-then-else vertex.
  ///
  /// @returns Pair of bools for (high, low) branch application results.
  ///          True means that terminal vertices are processed.
  std::pair<bool, bool> ConvertIfTerminal(const ItePtr& ite) noexcept;

  /// Helper function for ROBDD conversion.
  /// Checks the reduction rule upon assigning the high branch of the SetNode.
  ///
  /// @param[in] node Reduced ZBDD node.
  inline void ReduceHigh(const SetNodePtr& node) noexcept {
    ReduceApply(node, &high_, &high_term_);
  }

  /// Helper function for ROBDD conversion.
  /// Checks the reduction rule upon assigning the low branch of the SetNode.
  ///
  /// @param[in] node Reduced ZBDD node.
  inline void ReduceLow(const SetNodePtr& node) noexcept {
    ReduceApply(node, &low_, &low_term_);
  }

 private:
  /// Checks for the reduction rule upon assigning to branches.
  ///
  /// @param[in] node Reduced ZBDD node.
  /// @param[out] branch The branch for SetNode assignment.
  /// @param[out] branch_term The branch for Terminal vertex assignment.
  void ReduceApply(const SetNodePtr& node, SetNodePtr* branch,
                   TerminalPtr* branch_term) noexcept;
};

/// @class Zbdd
/// Zero-Suppressed Binary Desicision Diagrams for set manipulations.
class Zbdd {
 public:
  Zbdd();

  /// Runs the analysis
  /// with the representation of a Boolean graph
  /// as a Reduced Ordered BDD.
  ///
  /// @param[in] bdd ROBDD with the ITE vertices.
  void Analyze(const Bdd* bdd) noexcept;

  inline const std::vector<std::vector<int>>& cut_sets() const {
    return cut_sets_;
  }

 private:
  using TerminalPtr = std::shared_ptr<Terminal>;
  using ItePtr = std::shared_ptr<Ite>;
  using SetNodePtr = std::shared_ptr<SetNode>;
  using HashTable = TripletTable<SetNodePtr>;

  /// Converts BDD graph into ZBDD graph.
  ///
  /// @param[in] ite If-then-else vertex of the ROBDD graph.
  ///
  /// @returns Pointer to the root SetNode vertex of the ZBDD graph.
  SetNodePtr ConvertBdd(const ItePtr& ite) noexcept;

  /// Traverses the reduced ZBDD graph to generate cut sets.
  /// The generated cut sets are stored in the main container.
  ///
  /// @param[in] node The node in traversal.
  /// @param[in, out] path Current path of high branches.
  ///                      This container may get modified drastically
  ///                      upon passing to the main cut sets container.
  void GenerateCutSets(const SetNodePtr& node, std::vector<int>* path) noexcept;

  /// Table of unique SetNodes denoting sets.
  /// The key consists of (index, id_high, id_low) triplet.
  HashTable unique_table_;

  /// Table of processed computations over sets.
  /// The key must convey the semantics of the operation over sets.
  /// The argument functions are recorded with their IDs (not vertex indices).
  /// In order to keep only unique computations,
  /// the argument IDs must be ordered.
  HashTable compute_table_;

  std::unordered_map<int, SetNodePtr> ites_;  ///< Processed function graphs.

  const TerminalPtr kBase_;  ///< Terminal Base (Unity/1) set.
  const TerminalPtr kEmpty_;  ///< Terminal Empty (Null/0) set.
  int set_id_;  ///< Identification assignment for new set graphs.

  std::vector<std::vector<int>> cut_sets_;  ///< Generated cut sets.
};

}  // namespace scram

#endif  // SCRAM_SRC_ZBDD_H_
