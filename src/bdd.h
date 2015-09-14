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
/// It is expected
/// that in a reduced BDD graphs,
/// there are only two terminal vertices of value 1 or 0.
class Terminal : public Vertex {
 public:
  /// @param[in] value True or False (1 or 0) terminal.
  explicit Terminal(bool value);

  /// @returns The value of the terminal vertex.
  ///
  /// @note The value serves as an id for this terminal vertex.
  ///       Non-terminal if-then-else vertices should never have
  ///       identifications of value 0 or 1.
  inline bool value() const { return value_; }

 private:
  bool value_;  ///< The meaning of the terminal.
};

/// @class Ite
/// Representation of non-terminal if-then-else vertices in BDD graphs.
/// This class is designed to help construct and manipulate BDD graphs.
/// The design goal is to avoid as much RTTI as possible
/// and take into account the performance of algorithms.
class Ite : public Vertex {
 public:
  using TerminalPtr = std::shared_ptr<Terminal>;
  using ItePtr = std::shared_ptr<Ite>;

  /// @param[in] index Index of this non-terminal vertex.
  /// @param[in] order Specific ordering number for BDD graphs.
  Ite(int index, int order);

  /// @returns The index of this vertex.
  inline int index() const {
    assert(index_ > 0);
    return index_;
  }

  /// @returns The order of the vertex.
  inline int order() const {
    assert(order_ > 0);
    return order_;
  }

  /// @returns Unique identifier of the function graph.
  inline int id() const {
    assert(id_ > 1);  // Must not have an ID of terminal nodes.
    return id_;
  }

  /// Sets the unique identifier of the function graph.
  ///
  /// @param[in] id Unique identifier of the function graph.
  ///               The identifier should not collide
  ///               with the identifiers of terminal nodes.
  inline void id(int id) {
    assert(id > 1);  // Must not have an ID of terminal nodes.
    id_ = id;
  }

  /// @returns The left part of the key (id(low)) for the function graph.
  inline int IdLow() const {
    if (low_) return low_->id();
    assert(low_term_);
    return low_term_->value();
  }

  /// @returns The right part of the key (id(high)) for the function graph.
  inline int IdHigh() const {
    if (high_) return high_->id();
    assert(high_term_);
    return high_term_->value();
  }

  /// @returns (1/True/then) branch if-then-else vertex.
  /// @returns nullptr if the branch is Terminal or non-existent.
  inline const ItePtr& high() const { return high_; }

  /// Sets the (1/True/then) branch vertex.
  ///
  /// @param[in] high The if-then-else vertex.
  inline void high(const ItePtr& high) {
    assert(order_ < high->order_);  // Ordered graph.
    high_ = high;
    high_term_ = nullptr;
  }

  /// Sets the (1/True/then) branch to terminal vertex.
  ///
  /// @param[in] high_term The terminal vertex.
  inline void high(const TerminalPtr& high_term) {
    high_term_ = high_term;
    high_ = nullptr;
  }

  /// @returns (0/False/else) branch vertex.
  /// @returns nullptr if the branch is Terminal or non-existent.
  inline const ItePtr& low() const { return low_; }

  /// Sets the (0/False/else) branch vertex.
  ///
  /// @param[in] low The vertex.
  inline void low(const ItePtr& low) {
    assert(order_ < low->order_);  // Ordered graph.
    low_ = low;
    low_term_ = nullptr;
  }

  /// Sets the (0/False/else) branch to terminal vertex.
  ///
  /// @param[in] low_term The terminal vertex.
  inline void low(const TerminalPtr& low_term) {
    low_term_ = low_term;
    low_ = nullptr;
  }

 private:
  int index_;  ///< Index of the variable.
  int order_;  ///< Order of the variable.
  int id_;  ///< Unique identifier of the function graph with this vertex.
  ItePtr high_;  ///< 1 (True/then) branch in the Shannon decomposition.
  ItePtr low_;  ///< O (False/else) branch in the Shannon decomposition.
  TerminalPtr high_term_;  ///< Terminal vertex for 1 (True/then) branch.
  TerminalPtr low_term_;  ///< Terminal vertex for 0 (False/else) branch.
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

  /// Reverses the optimization value assigned by the topological ordering.
  /// This reversing is needed
  /// to put optimization values in ascending order
  /// starting from the root.
  ///
  /// @param[in] root The root or current parent gate of the graph.
  /// @param[in] shift The shift for the vertex ordering.
  ///                  Optimization value is subtracted from the shift.
  ///
  /// @warning Node visit information must be clear.
  void AdjustOrder(const IGatePtr& root, int shift) noexcept;

  /// Converts all gates in the Boolean graph
  /// into if-then-else BDD graphs.
  /// Registers processed gates.
  ///
  /// @param[in] root The root or current parent gate of the graph.
  ///
  /// @warning Gate marks must be clear.
  void ConvertGates(const IGatePtr& root) noexcept;

  /// Applies reduction rules to the BDD graph.
  ///
  /// @param[in] ite The root vertex of the graph.
  ///
  /// @returns Pointer to the replacement vertex.
  /// @returns nullptr if no replacement is required for reduction.
  ItePtr ReduceGraph(const ItePtr& ite) noexcept;

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
  std::unordered_map<std::array<int, 3>, ItePtr, IteHash> unique_table_;
  std::unordered_map<int, ItePtr> gates_;  ///< Processed gates.
  const TerminalPtr kOne_;  ///< Terminal True.
  const TerminalPtr kZero_;  ///< Terminal False.
  int function_id_;  ///< Identification assignment for new function graphs.
};

}  // namespace scram

#endif  // SCRAM_SRC_BDD_H_
