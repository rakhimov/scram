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

/// @enum SetOp
/// Operations on sets.
enum class SetOp {
  Without = 0  ///< Without '\' operator.
};

/// @class SetNode
/// Representation of non-terminal nodes in ZBDD.
class SetNode : public NonTerminal {
 public:
  using NonTerminal::NonTerminal;  ///< Constructor with index and order.

  /// Recovers a shared pointer to SetNode from a pointer to Vertex.
  ///
  /// @param[in] vertex Pointer to a Vertex known to be a SetNode.
  ///
  /// @return Casted pointer to SetNode.
  static std::shared_ptr<SetNode> Ptr(const std::shared_ptr<Vertex>& vertex) {
    return std::static_pointer_cast<SetNode>(vertex);
  }
};

/// @class Zbdd
/// Zero-Suppressed Binary Decision Diagrams for set manipulations.
class Zbdd {
 public:
  Zbdd();

  /// Runs the analysis
  /// with the representation of a Boolean graph
  /// as a Reduced Ordered BDD.
  ///
  /// @param[in] bdd ROBDD with the ITE vertices.
  ///
  /// @note Only coherent (monotonic) graphs are accepted.
  /// @note BDD is assumed to have attributed edges with only one terminal 1.
  void Analyze(const Bdd* bdd) noexcept;

  /// @returns Minimal cut sets.
  const std::vector<std::vector<int>>& cut_sets() const { return cut_sets_; }

 private:
  using VertexPtr = std::shared_ptr<Vertex>;
  using TerminalPtr = std::shared_ptr<Terminal>;
  using ItePtr = std::shared_ptr<Ite>;
  using SetNodePtr = std::shared_ptr<SetNode>;
  using UniqueTable = TripletTable<SetNodePtr>;
  using ComputeTable = TripletTable<VertexPtr>;
  using CutSet = std::vector<int>;

  /// Converts BDD graph into ZBDD graph.
  ///
  /// @param[in] vertex Vertex of the ROBDD graph.
  /// @param[in] complement Interpretation of the vertex as complement.
  ///
  /// @returns Pointer to the root vertex of the ZBDD graph.
  VertexPtr ConvertBdd(const VertexPtr& vertex, bool complement) noexcept;

  /// Removes subsets in ZBDD.
  ///
  /// @param[in] vertex The variable node in the set.
  ///
  /// @returns Processed vertex.
  ///
  /// @warning NonTerminal vertex marks are used.
  VertexPtr Subsume(const VertexPtr& vertex) noexcept;

  /// Applies subsume operation on two sets.
  /// Subsume operation removes
  /// paths that exist in Low branch from High branch.
  ///
  /// @param[in] high True/then/high branch of a variable.
  /// @param[in] low False/else/low branch of a variable.
  ///
  /// @returns Minimized high branch for a variable.
  VertexPtr Subsume(const VertexPtr& high, const VertexPtr& low) noexcept;

  /// Traverses the reduced ZBDD graph to generate cut sets.
  /// The generated cut sets are stored in the main container.
  ///
  /// @param[in] vertex The node in traversal.
  /// @param[in, out] path Current path of high branches.
  ///                      This container may get modified drastically
  ///                      upon passing to the main cut sets container.
  /// @param[in, out] cut_sets A set of cut sets generated in this graph.
  void GenerateCutSets(const VertexPtr& vertex, std::vector<int>* path,
                       std::vector<CutSet>* cut_sets) noexcept;

  /// Table of unique SetNodes denoting sets.
  /// The key consists of (index, id_high, id_low) triplet.
  UniqueTable unique_table_;

  /// Table of processed computations over sets.
  /// The key must convey the semantics of the operation over sets.
  /// The argument functions are recorded with their IDs (not vertex indices).
  /// In order to keep only unique computations,
  /// the argument IDs must be ordered.
  ComputeTable compute_table_;

  const Bdd* bdd_graph_;  ///< BDD for transformations.
  std::unordered_map<int, SetNodePtr> ites_;  ///< Processed function graphs.
  std::unordered_map<int, VertexPtr> modules_;  ///< Module graphs.
  const TerminalPtr kBase_;  ///< Terminal Base (Unity/1) set.
  const TerminalPtr kEmpty_;  ///< Terminal Empty (Null/0) set.
  int set_id_;  ///< Identification assignment for new set graphs.
  std::unordered_map<int, VertexPtr> subsume_results_;  ///< Memorize subsume.
  /// Storage for cut sets generated for modules.
  std::unordered_map<int, std::vector<CutSet>> module_cut_sets_;
  std::vector<CutSet> cut_sets_;  ///< Generated cut sets.
};

}  // namespace scram

#endif  // SCRAM_SRC_ZBDD_H_
