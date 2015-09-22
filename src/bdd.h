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
class Vertex {
 public:
  /// @param[in] terminal Flag for terminal nodes.
  /// @param[in] id Identificator of the BDD graph.
  explicit Vertex(bool terminal = false, int id = 0);

  Vertex(const Vertex&) = delete;
  Vertex& operator=(const Vertex&) = delete;

  virtual ~Vertex() = 0;  ///< Abstract class.

  /// @returns true if this vertex is terminal.
  inline bool terminal() const { return terminal_; }

  /// @returns Identificator of the BDD graph rooted by this vertex.
  ///
  /// @todo Deal with 0 id.
  inline int id() const { return id_; };

 protected:
  int id_;  ///< Unique identifier of the BDD graph with this vertex.

 private:
  bool terminal_;  ///< Flag for terminal vertices. RTTI hack.
};

/// @class Terminal
/// Representation of terminal vertices in BDD graphs.
/// It is expected
/// that in a reduced BDD graphs,
/// there are at most two terminal vertices of value 1 or 0.
/// If the BDD graph has attributed edges,
/// only single terminal vertex is expected with value 1.
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

  /// Recovers a shared pointer to Terminal from a pointer to Vertex.
  ///
  /// @param[in] vertex Pointer to a Vertex known to be a Terminal.
  ///
  /// @return Casted pointer to Terminal.
  inline static std::shared_ptr<Terminal> Ptr(
      const std::shared_ptr<Vertex>& vertex) {
    return std::static_pointer_cast<Terminal>(vertex);
  }

 private:
  bool value_;  ///< The meaning of the terminal.
};

/// @class NonTerminal
/// Representation of non-terminal vertices in BDD graphs.
class NonTerminal : public Vertex {
 public:
  using VertexPtr = std::shared_ptr<Vertex>;

  /// @param[in] index Index of this non-terminal vertex.
  /// @param[in] order Specific ordering number for BDD graphs.
  NonTerminal(int index, int order);

  virtual ~NonTerminal() = 0;  ///< Abstract base class.

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

  /// @returns true if this vertex represents a module gate.
  inline bool module() const { return module_; }

  /// Sets this vertex for representation of a module.
  inline void module(bool flag) { module_ = flag; }

  /// Sets the unique identifier of the ROBDD graph.
  ///
  /// @param[in] id Unique identifier of the ROBDD graph.
  ///               The identifier should not collide
  ///               with the identifiers of terminal nodes.
  inline void id(int id) {
    assert(id > 1);  // Must not have an ID of terminal nodes.
    Vertex::id_ = id;
  }

  /// @returns (1/True/then/left) branch if-then-else vertex.
  inline const VertexPtr& high() const { return high_; }

  /// Sets the (1/True/then/left) branch vertex.
  ///
  /// @param[in] high The if-then-else vertex.
  inline void high(const VertexPtr& high) { high_ = high; }

  /// @returns (0/False/else/right) branch vertex.
  ///
  /// @note This edge may have complement interpretation.
  ///       Check complement_edge() upon using this edge.
  inline const VertexPtr& low() const { return low_; }

  /// Sets the (0/False/else/right) branch vertex.
  ///
  /// @param[in] low The vertex.
  ///
  /// @note This may have complement interpretation.
  ///       Keep the complement_edge() flag up-to-date
  ///       after setting this edge.
  inline void low(const VertexPtr& low) { low_ = low; }

  /// @returns true if the low edge is complement.
  inline bool complement_edge() const { return complement_edge_; }

  /// Sets the complement flag for the low edge.
  ///
  /// @param[in] flag Indicator to treat the low edge as a complement.
  inline void complement_edge(bool flag) { complement_edge_ = flag; }

  /// @returns The mark of this vertex.
  inline bool mark() const { return mark_; }

  /// Marks this vertex.
  ///
  /// @param[in] flag A flag with the meaning for the user of marks.
  inline void mark(bool flag) { mark_ = flag; }

 protected:
  int index_;  ///< Index of the variable.
  int order_;  ///< Order of the variable.
  bool module_;  ///< Mark for module variables.
  VertexPtr high_;  ///< 1 (True/then) branch in the Shannon decomposition.
  VertexPtr low_;  ///< O (False/else) branch in the Shannon decomposition.
  bool complement_edge_;  ///< Flag for complement low edge.
  bool mark_;  ///< Traversal mark.
};

/// @class Ite
/// Representation of non-terminal if-then-else vertices in BDD graphs.
/// This class is designed to help construct and manipulate BDD graphs.
class Ite : public NonTerminal {
 public:
  using NonTerminal::NonTerminal;  ///< Constructor with index and order.

  /// @returns The probability of the function graph.
  inline double prob() const { return prob_; }

  /// Sets the probability of the function graph.
  ///
  /// @param[in] value Calculated value for the probability.
  inline void prob(double value) { prob_ = value; }

  /// Recovers a shared pointer to Ite from a pointer to Vertex.
  ///
  /// @param[in] vertex Pointer to a Vertex known to be an Ite.
  ///
  /// @return Casted pointer to Ite.
  inline static std::shared_ptr<Ite> Ptr(
      const std::shared_ptr<Vertex>& vertex) {
    return std::static_pointer_cast<Ite>(vertex);
  }

 private:
  double prob_ = 0;  ///< Probability of the function graph.
};

using Triplet = std::array<int, 3>;  ///< (v, G, H) triplet for functions.

/// @struct TripletHash
/// Functor for hashing triplets of ordered numbers.
struct TripletHash : public std::unary_function<const Triplet, std::size_t> {
  /// Operator overload for hashing three ordered ID numbers.
  ///
  /// @param[in] triplet (v, G, H) nodes.
  ///
  /// @returns Hash value of the triplet.
  std::size_t operator()(const Triplet& triplet) const noexcept {
    return boost::hash_range(triplet.begin(), triplet.end());
  }
};

/// Hash table for triplets of numbers as keys.
template<typename Value>
using TripletTable = std::unordered_map<Triplet, Value, TripletHash>;

/// @class Bdd
/// Analysis of Boolean graphs with Binary Decision Diagrams.
/// This binary dicision diagram data structure
/// represents Reduced Ordered BDD with attributed edges.
/// There is only one terminal vertex of value 1/True.
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

  /// @returns The root vertex of the ROBDD.
  inline const std::shared_ptr<Vertex>& root() const { return root_; }

  /// @returns true if the root must be interpreted as complement.
  inline bool complement_root() const { return complement_root_; }

 private:
  using NodePtr = std::shared_ptr<Node>;
  using VariablePtr = std::shared_ptr<Variable>;
  using IGatePtr = std::shared_ptr<IGate>;
  using VertexPtr = std::shared_ptr<Vertex>;
  using TerminalPtr = std::shared_ptr<Terminal>;
  using ItePtr = std::shared_ptr<Ite>;

  /// @struct Result
  /// Holder of computation results.
  struct Result {
    bool complement;  ///< The interpretation for the result.
    VertexPtr vertex;  ///< The root vertex of the resulting BDD graph.
  };

  using UniqueTable = TripletTable<ItePtr>;  ///< To store unique vertices.
  using ComputeTable = TripletTable<Result>;  ///< To store computation results.

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
  const Result& IfThenElse(const IGatePtr& gate) noexcept;

  /// Converts variable argument of a Boolean graph gate
  /// into if-then-else BDD graph vertex.
  /// Registers processed variable.
  ///
  /// @param[in] variable The variable argument.
  ///
  /// @returns Pointer to the root vertex of the BDD graph.
  ItePtr IfThenElse(const VariablePtr& variable) noexcept;

  /// Creates a vertex to represent a module gate.
  ///
  /// @param[in] gate The root or current parent gate of the graph.
  ///
  /// @returns Pointer to the BDD if-then-else vertex.
  ///
  /// @note The gate still needs to be converted and saved.
  ItePtr CreateModuleProxy(const IGatePtr& gate) noexcept;

  /// Applies reduction rules to a BDD graph.
  ///
  /// @param[in,out] vertex The root vertex of the graph.
  ///
  /// @returns The root vertex of the reduced graph.
  ///          It is the same vertex as the input vertex
  ///          only if the resulting graph is unique.
  ///
  /// @todo Review with the one terminal changes.
  VertexPtr Reduce(const VertexPtr& vertex) noexcept;

  /// Applies Boolean operation to BDD graphs.
  ///
  /// @param[in] type The operator or type of the gate.
  /// @param[in] arg_one First argument function graph.
  /// @param[in] arg_two Second argument function graph.
  /// @param[in] complement_one Interpretation of arg_one as complement.
  /// @param[in] complement_two Interpretation of arg_two as complement.
  ///
  /// @returns Poitner to the root vertex of the resultant BDD graph.
  ///
  /// @note The order of arguments does not matter for two variable operators.
  Result Apply(Operator type,
               const VertexPtr& arg_one, const VertexPtr& arg_two,
               bool complement_one, bool complement_two) noexcept;

  /// Applies the logic of a Boolean operator
  /// to terminal vertices.
  ///
  /// @param[in] type The operator to apply.
  /// @param[in] term_one First argument terminal vertex.
  /// @param[in] term_two Second argument terminal vertex.
  /// @param[in] complement_one Interpretation of term_one as complement.
  /// @param[in] complement_two Interpretation of term_two as complement.
  ///
  /// @returns Terminal vertex as a result of operations.
  Result Apply(Operator type,
               const TerminalPtr& term_one, const TerminalPtr& term_two,
               bool complement_one, bool complement_two) noexcept;

  /// Applies the logic of a Boolean operator
  /// to non-terminal and terminal vertices.
  ///
  /// @param[in] type The operator or type of the gate.
  /// @param[in] ite_one Non-terminal vertex.
  /// @param[in] term_two Terminal vertex.
  /// @param[in] complement_one Interpretation of ite_one as complement.
  /// @param[in] complement_two Interpretation of term_two as complement.
  ///
  /// @returns Pointer to the vertex as a result of operations.
  Result Apply(Operator type,
               const ItePtr& ite_one, const TerminalPtr& term_two,
               bool complement_one, bool complement_two) noexcept;

  /// Produces canonical signature of application of Boolean operations.
  /// The signature of the operations helps
  /// detect equivalent operations.
  ///
  /// @param[in] type The operator or type of the gate.
  /// @param[in] arg_one First argument function graph.
  /// @param[in] arg_two Second argument function graph.
  /// @param[in] complement_one Interpretation of arg_one as complement.
  /// @param[in] complement_two Interpretation of arg_two as complement.
  ///
  /// @returns Unique signature of the operation.
  ///
  /// @note The arguments should not be the same functions.
  ///       It is assumed
  ///       that equal ID functions are handled by the reduction.
  /// @note Even though the arguments are not ItePtr,
  ///       it is expected that they are if-then-else vertices.
  Triplet GetSignature(Operator type,
                       const VertexPtr& arg_one, const VertexPtr& arg_two,
                       bool complement_one, bool complement_two) noexcept;

  /// Calculates exact probability
  /// of a function graph represented by its root vertex.
  ///
  /// @param[in] vertex The root vertex of a function graph.
  ///
  /// @returns Probability value.
  double CalculateProbability(const VertexPtr& vertex) noexcept;

  BooleanGraph* fault_tree_;  ///< The main fault tree.
  VertexPtr root_;  ///< The root vertex of this BDD.
  bool complement_root_;  ///< The interpretation of the root as complement.

  /// Table of unique if-then-else nodes denoting function graphs.
  /// The key consists of ite(index, id_high, id_low),
  /// where IDs are unique (id_high != id_low) identifications of
  /// unique reduced-ordered function graphs.
  UniqueTable unique_table_;

  /// Table of processed computations over functions.
  /// The key must convey the semantics of the operation over functions.
  /// For example, AND(F, G) is ite(F, G, 0),
  /// which is also equivalent to ite(G, F, 0).
  /// The argument functions are recorded with their IDs (not vertex indices).
  /// In order to keep only unique computations,
  /// the argument IDs must be ordered.
  ComputeTable compute_table_;

  std::unordered_map<int, Result> gates_;  ///< Processed gates.
  std::vector<double> probs_;  ///< Probabilities of variables.
  double p_graph_;  ///< Total probability of the graph.

  const TerminalPtr kOne_;  ///< Terminal True.
  int function_id_;  ///< Identification assignment for new function graphs.
};

}  // namespace scram

#endif  // SCRAM_SRC_BDD_H_
