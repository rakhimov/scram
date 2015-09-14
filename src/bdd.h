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

#include <array>
#include <memory>
#include <unordered_map>

#include <boost/functional/hash.hpp>

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

/// @class Ite
/// Representation of non-terminal if-then-else vertices in BDD graphs.
class Ite : public Vertex {
 public:
  using VertexPtr = std::shared_ptr<Vertex>;

  /// @param[in] index Unique identifier of this non-terminal vertex.
  explicit Ite(int index);

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

using IteTriplet = std::array<int, 3>;  ///< (v, G, H) triplet for functions.

/// @struct IteHash
/// Functor for hashing if-then-else nodes.
struct IteHash : public std::unary_function<const IteTriplet, std::size_t> {
  /// Operator overload for hashing if-then-else nodes.
  ///
  /// @param[in] triplet (v, G, H) nodes.
  ///
  /// @returns Hash value of the triplet.
  std::size_t operator()(const IteTriplet& triplet) const noexcept {
    return boost::hash_range(triplet.begin(), triplet.end());
  }
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
  using NodePtr = std::shared_ptr<Node>;
  using IGatePtr = std::shared_ptr<IGate>;
  using VertexPtr = std::shared_ptr<Vertex>;
  using TerminalPtr = std::shared_ptr<Terminal>;
  using ItePtr = std::shared_ptr<Ite>;

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

  /// Converts all gates in the Boolean graph
  /// into if-then-else BDD graphs.
  /// Registers processed gates.
  ///
  /// @param[in] root The root or current parent gate of the graph.
  ///
  /// @warning Gate marks must be clear.
  void ConvertGates(const IGatePtr& root) noexcept;

  /// Turns a Boolean graph gate into a BDD graph.
  /// Registers the results for future references.
  ///
  /// @param[in] gate The gate with a formula and arguments.
  void ProcessGate(const IGatePtr& gate) noexcept;

  /// Considers the Shannon decomposition
  /// for the given Boolean operator
  /// with ordered arguments.
  ///
  /// @param[in] type The operator or type of the gate.
  /// @param[in] args Ordered arguments of the gate.
  /// @param[in] pos The position of the argument to decompose against.
  ///
  /// @returns Pointer to the vertex in the BDD graph for the decomposition.
  ItePtr IfThenElse(Operator type,
                    const std::vector<std::pair<int, NodePtr>>& args,
                    int pos) noexcept;

  BooleanGraph* fault_tree_;  ///< The main fault tree.
  /// Table of unique if-then-else calculation results.
  std::unordered_map<std::array<int, 3>, VertexPtr, IteHash> unique_table_;
  std::unordered_map<int, ItePtr> gates_;  ///< Processed gates.
  const TerminalPtr kOne_;  ///< Terminal True.
  const TerminalPtr kZero_;  ///< Terminal False.
};

}  // namespace scram

#endif  // SCRAM_SRC_BDD_H_
