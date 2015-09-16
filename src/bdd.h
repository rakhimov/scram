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
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

#include "boolean_graph.h"

namespace scram {

/// @class Vertex
/// Representation of a vertex in BDD graphs.
/// The design goal for vertex classes
/// is to avoid as much RTTI as possible
/// and take into account the performance of algorithms.
class Vertex {
 public:
  Vertex() = default;
  Vertex(const Vertex&) = delete;
  Vertex& operator=(const Vertex&) = delete;
  virtual ~Vertex() = 0;  ///< Abstract class.
};

/// @class Terminal
/// Representation of terminal vertices in BDD graphs.
/// It is expected
/// that in a reduced BDD graphs,
/// there are at most two terminal vertices of value 1 or 0.
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

/// @class NonTerminal
/// Representation of non-terminal vertices in BDD graphs.
template<typename Node>
class NonTerminal : public Vertex {
 public:
  using TerminalPtr = std::shared_ptr<Terminal>;
  using NonTerminalPtr = std::shared_ptr<Node>;

  /// @param[in] index Index of this non-terminal vertex.
  /// @param[in] order Specific ordering number for BDD graphs.
  NonTerminal(int index, int order)
      : index_(index),
        order_(order),
        id_(0),
        mark_(false) {}

  virtual ~NonTerminal() {}

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

  /// @returns Unique identifier of the ROBDD graph.
  /// @returns 0 if no reduction is performed with a resulting id.
  inline int id() const {
    return id_;
  }

  /// Sets the unique identifier of the ROBDD graph.
  ///
  /// @param[in] id Unique identifier of the ROBDD graph.
  ///               The identifier should not collide
  ///               with the identifiers of terminal nodes.
  inline void id(int id) {
    assert(id > 1);  // Must not have an ID of terminal nodes.
    id_ = id;
  }

  /// @returns The left part of the key (id(low)) for the ROBDD graph.
  inline int IdLow() const {
    if (low_) return low_->id();
    assert(low_term_);
    return low_term_->value();
  }

  /// @returns The right part of the key (id(high)) for the ROBDD graph.
  inline int IdHigh() const {
    if (high_) return high_->id();
    assert(high_term_);
    return high_term_->value();
  }

  /// @returns (1/True/then) branch if-then-else vertex.
  /// @returns nullptr if the branch is Terminal or non-existent.
  inline const NonTerminalPtr& high() const { return high_; }

  /// Sets the (1/True/then) branch vertex.
  ///
  /// @param[in] high The if-then-else vertex.
  inline void high(const NonTerminalPtr& high) {
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
  inline const NonTerminalPtr& low() const { return low_; }

  /// Sets the (0/False/else) branch vertex.
  ///
  /// @param[in] low The vertex.
  inline void low(const NonTerminalPtr& low) {
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

  /// @returns (1/True/then) branch terminal vertex.
  /// @returns nullptr if the branch is not terminal.
  inline const TerminalPtr& high_term() const { return high_term_; }

  /// @returns (0/False/else) branch terminal vertex.
  /// @returns nullptr if the branch is not terminal.
  inline const TerminalPtr& low_term() const { return low_term_; }

  /// @returns The mark of this vertex.
  inline bool mark() const { return mark_; }

  /// Marks this vertex.
  ///
  /// @param[in] flag A flag with the meaning for the user of marks.
  inline void mark(bool flag) { mark_ = flag; }

 protected:
  int index_;  ///< Index of the variable.
  int order_;  ///< Order of the variable.
  int id_;  ///< Unique identifier of the ROBDD graph with this vertex.
  NonTerminalPtr high_;  ///< 1 (True/then) branch in the Shannon decomposition.
  NonTerminalPtr low_;  ///< O (False/else) branch in the Shannon decomposition.
  TerminalPtr high_term_;  ///< Terminal vertex for 1 (True/then) branch.
  TerminalPtr low_term_;  ///< Terminal vertex for 0 (False/else) branch.
  bool mark_;  ///< Traversal mark.
};

/// @class Ite
/// Representation of non-terminal if-then-else vertices in BDD graphs.
/// This class is designed to help construct and manipulate BDD graphs.
class Ite : public NonTerminal<Ite> {
 public:
  using ItePtr = std::shared_ptr<Ite>;

  using NonTerminal::NonTerminal;  ///< Constructor with index and order.

  /// @returns The probability of the function graph.
  inline double prob() const { return prob_; }

  /// Sets the probability of the function graph.
  ///
  /// @param[in] value Calculated value for the probability.
  inline void prob(double value) { prob_ = value; }

  /// Checks and applies Boolean operators for terminal branches.
  /// The function is coupled with the BDD Apply procedures.
  ///
  /// @param[in] type The operator or type of the gate.
  /// @param[in] arg_one First argument function graph with lower order.
  /// @param[in] arg_two Second argument function graph with higher order.
  ///
  /// @returns Pair of bools for (high, low) branch application results.
  ///          True means that terminal vertices are processed.
  ///
  /// @note This if-then-else vertex should be new without any branches.
  /// @note Argument vertices must have equal order,
  ///       or they must be passed in ascending order.
  /// @note The main reason for the conditional Apply
  ///       is to avoid RTTI and related hacks by the users of Vertex classes.
  std::pair<bool, bool> ApplyIfTerminal(Operator type, const ItePtr& arg_one,
                                        const ItePtr& arg_two) noexcept;

 private:
  using TerminalPtr = std::shared_ptr<Terminal>;

  /// Applies the logic of a Boolean operator
  /// to terminal vertices.
  ///
  /// @param[in] type The operator to apply.
  /// @param[in] term_one First argument terminal vertex or nullptr.
  /// @param[in] term_two Second argument terminal vertex or nullptr.
  ///
  /// @returns Terminal vertex as a result of operations.
  /// @returns nullptr if no operations are performed.
  ///
  /// @note If the input pointer parameters are nullptr,
  ///       no operations are performed.
  TerminalPtr ApplyTerminal(Operator type, const TerminalPtr& term_one,
                            const TerminalPtr& term_two) noexcept;

  /// Applies the logic of a Boolean operator
  /// to non-terminal and terminal vertices.
  ///
  /// @param[in] type The operator or type of the gate.
  /// @param[in] v_one Non-terminal if-then-else vertex.
  /// @param[in] term_one Terminal vertex.
  /// @param[out] branch The resulting if-then-else branch of the operation.
  /// @param[out] branch_term The resulting terminal branch of the operation.
  ///
  /// @note The result is assigned to only of the the branches
  ///       depending on the logic and the input arguments.
  /// @note If the input pointer parameters are nullptr,
  ///       no operations are performed.
  void ApplyTerminal(Operator type, const ItePtr& v_one,
                     const TerminalPtr& term_one,
                     ItePtr* branch, TerminalPtr* branch_term) noexcept;

  double prob_ = 0;  ///< Probability of the function graph.
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

  /// @returns The total probability of the graph.
  inline double p_graph() const { return p_graph_; }

 private:
  using NodePtr = std::shared_ptr<Node>;
  using IGatePtr = std::shared_ptr<IGate>;
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
  /// @param[in] gate The root or current parent gate of the graph.
  ///
  /// @returns Pointer to the root vertex of the BDD graph.
  ItePtr ConvertGate(const IGatePtr& gate) noexcept;

  /// Applies reduction rules to the BDD graph.
  ///
  /// @param[in] ite The root vertex of the graph.
  ///
  /// @returns Pointer to the replacement vertex.
  /// @returns nullptr if no replacement is required for reduction.
  ItePtr ReduceGraph(const ItePtr& ite) noexcept;

  /// Considers the Shannon decomposition
  /// for the given Boolean operator
  /// with ordered arguments.
  ///
  /// @param[in] type The operator or type of the gate.
  /// @param[in] args Ordered arguments of the gate.
  /// @param[in] pos The position of the argument to decompose against.
  ///
  /// @returns Pointer to the vertex in the BDD graph for the decomposition.
  ///
  /// @note It is expected that arguments are variables (modules) but not gates.
  ItePtr IfThenElse(Operator type,
                    const std::vector<std::pair<int, NodePtr>>& args,
                    int pos) noexcept;

  /// Applies Boolean operation to BDD graphs.
  ///
  /// @param[in] type The operator or type of the gate.
  /// @param[in] arg_one First argument function graph.
  /// @param[in] arg_two Second argument function graph.
  ///
  /// @returns Poitner to the root vertex of the resultant BDD graph.
  ///
  /// @note The order of arguments does not matter for two variable operators.
  ItePtr Apply(Operator type, const ItePtr& arg_one,
               const ItePtr& arg_two) noexcept;

  /// Calculates exact probability
  /// of a function graph represented by its root vertex.
  ///
  /// @param[in] vertex The root vertex of a function graph.
  ///
  /// @returns Probability value.
  double CalculateProbability(const ItePtr& vertex) noexcept;

  BooleanGraph* fault_tree_;  ///< The main fault tree.

  /// Table of unique if-then-else nodes denoting function graphs.
  /// The key consists of ite(index, id_high, id_low),
  /// where IDs are unique (id_high != id_low) identifications of
  /// unique reduced-ordered function graphs.
  std::unordered_map<std::array<int, 3>, ItePtr, IteHash> unique_table_;

  /// Table of processed computations over functions.
  /// The key must convey the semantics of the operation over functions.
  /// For example, AND(F, G) is ite(F, G, 0),
  /// which is also equivalent to ite(G, F, 0).
  /// The argument functions are recorded with their IDs (not vertex indices).
  /// In order to keep only unique computations,
  /// the argument IDs must be ordered.
  std::unordered_map<std::array<int, 3>, ItePtr, IteHash> compute_table_;

  std::unordered_map<int, ItePtr> gates_;  ///< Processed gates.
  std::vector<double> probs_;  ///< Probabilities of variables.
  double p_graph_;  ///< Total probability of the graph.

  const TerminalPtr kOne_;  ///< Terminal True.
  const TerminalPtr kZero_;  ///< Terminal False.
  int function_id_;  ///< Identification assignment for new function graphs.
};

}  // namespace scram

#endif  // SCRAM_SRC_BDD_H_
