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

/// @file bdd.h
/// Fault tree analysis with the Binary Decision Diagram algorithms.

#ifndef SCRAM_SRC_BDD_H_
#define SCRAM_SRC_BDD_H_

#include <memory>

#include "boolean_graph.h"

namespace scram {

/// @class Vertex
/// Representation of a vertex in BDD graphs.
class Vertex {
 public:
  virtual ~Vertex() = 0;  ///< Abstract class.
};

/// @class Terminal
/// Representation of terminal vertices in BDD graphs.
class Terminal : public Vertex {
 public:
  /// @param[in] value True or False (1 or 0) terminal.
  explicit Terminal(bool value);

  /// @returns The value of the terminal vertex.
  inline bool value() const { return value_; }

 private:
  bool value_;  ///< The meaning of the terminal.
};

/// @class NonTerminal
/// Representation of non-terminal vertices in BDD graphs.
class NonTerminal : public Vertex {
 public:
  using VertexPtr = std::shared_ptr<Vertex>;

  /// @param[in] index Unique identifier of this non-terminal vertex.
  explicit NonTerminal(int index);

  /// @returns The index of this vertex.
  inline int index() const { return index_; }

  /// @returns (1/True/then) branch vertex.
  inline const VertexPtr& high() const { return high_; }

  /// Sets the (1/True/then) branch vertex.
  ///
  /// @param[in] high The vertex.
  inline void high(const VertexPtr& high) { high_ = high; }

  /// @returns (0/False/else) branch vertex.
  inline const VertexPtr& low() const { return low_; }

  /// Sets the (0/False/else) branch vertex.
  ///
  /// @param[in] low The vertex.
  inline void low(const VertexPtr& low) { low_ = low; }

 private:
  int index_;  ///< Index of the variable.
  VertexPtr high_;  ///< 1 (True/then) branch in the Shannon decomposition.
  VertexPtr low_;  ///< O (False/else) branch in the Shannon decomposition.
};

/// @class Bdd
/// Analysis of Boolean graphs with Binary Decision Diagrams.
class Bdd {
 public:
  /// Constructor with the analysis target.
  ///
  /// @param[in] fault_tree Preprocessed, normalized, and indexed fault tree.
  explicit Bdd(BooleanGraph* fault_tree);

  /// Runs the analysis.
  void Analyze() noexcept;

 private:
  using IGatePtr = std::shared_ptr<IGate>;

  /// Assigns topological ordering to nodes of the Boolean Graph.
  /// The ordering is assigned to the optimization value of the nodes.
  /// The nodes are sorted in descending optimization value.
  /// The highest optimization value belongs to the root.
  ///
  /// @param[in] root The root or current parent gate of the graph.
  /// @param[in] order The current order value.
  ///
  /// @returns The final order value.
  ///
  /// @note Optimization values must be clear before the assignment.
  int TopologicalOrder(const IGatePtr& root, int order) noexcept;

  BooleanGraph* fault_tree_;  ///< The main fault tree.
};

}  // namespace scram

#endif  // SCRAM_SRC_BDD_H_
