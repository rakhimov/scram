/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

/// @file boolean_graph.h
/// Classes and facilities to represent simplified fault trees
/// as Boolean graphs with event and gate indices instead of ID names.
/// These facilities are designed to work
/// with FaultTreeAnalysis and Preprocessor classes.
///
/// The terminologies of the graphs and Boolean logic are mixed
/// to represent the Boolean graph;
/// however, if there is a conflict,
/// the Boolean terminology is preferred.
/// For example, instead of "children", "arguments" are preferred.

#ifndef SCRAM_SRC_BOOLEAN_GRAPH_H_
#define SCRAM_SRC_BOOLEAN_GRAPH_H_

#include <cassert>

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/container/flat_set.hpp>
#include <boost/functional/hash.hpp>

#include "event.h"
#include "ext.h"
#include "linear_map.h"

namespace scram {
namespace core {

class Gate;  // Indexed gate parent of nodes.
using GatePtr = std::shared_ptr<Gate>;  ///< Shared gates in the graph.
using GateWeakPtr = std::weak_ptr<Gate>;  ///< Acyclic ptr to parent gates.

/// Manager of information about parents.
/// Only gates can manipulate the data.
class NodeParentManager {
  friend class Gate;  ///< The main manipulator of parent information.

 public:
  using Parent = std::pair<int, GateWeakPtr>;  ///< Parent index and ptr.

  /// Map of parent gate positive indices and weak pointers to them.
  using ParentMap = LinearMap<int, GateWeakPtr, std::vector, MoveEraser>;

  /// @returns Parents of a node.
  const ParentMap& parents() const { return parents_; }

 protected:
  ~NodeParentManager() = default;

 private:
  /// Adds a new parent of a node.
  ///
  /// @param[in] gate  Pointer to the parent gate.
  ///
  /// @pre The parent is not in the container.
  void AddParent(const GatePtr& gate);

  /// Removes a parent from the node.
  ///
  /// @param[in] index  Positive index of the parent gate.
  ///
  /// @pre There is a parent with the given index.
  void EraseParent(int index) {
    assert(parents_.count(index) && "No parent with the given index exists.");
    parents_.erase(index);
  }

  ParentMap parents_;  ///< All registered parents of this node.
};

/// An abstract base class that represents a node in a Boolean graph.
/// The index of the node is a unique identifier for the node.
/// The node holds weak pointers to the parents
/// that are managed by the parents.
class Node : public NodeParentManager {
 public:
  /// Creates a graph node with its index assigned sequentially.
  Node() noexcept;

  /// Creates a graph node with its index.
  ///
  /// @param[in] index  An unique positive index of this node.
  ///
  /// @warning The index is not validated upon instantiation.
  explicit Node(int index) noexcept;

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  virtual ~Node() = 0;  ///< Abstract class.

  /// Resets the starting index.
  static void ResetIndex() { next_index_ = 1e6; }

  /// @returns The index of this node.
  int index() const { return index_; }

  /// @returns Assigned order for this node.
  int order() const { return order_; }

  /// Sets the order number for this node.
  /// The order is interpreted by the assigner.
  ///
  /// @param[in] val  Positive integer.
  void order(int val) { order_ = val; }

  /// @returns Optimization value for failure propagation.
  int opti_value() const { return opti_value_; }

  /// Sets the optimization value for failure propagation.
  ///
  /// @param[in] val  Value that makes sense to the caller.
  void opti_value(int val) { opti_value_ = val; }

  /// Registers the visit time for this node upon graph traversal.
  /// This information can be used to detect dependencies.
  ///
  /// @param[in] time  The current visit time of this node.
  ///                  It must be positive.
  ///
  /// @returns true if this node was previously visited.
  /// @returns false if this is visited and re-visited only once.
  bool Visit(int time) {
    assert(time > 0);
    if (!visits_[0]) {
      visits_[0] = time;
    } else if (!visits_[1]) {
      visits_[1] = time;
    } else {
      visits_[2] = time;
      return true;
    }
    return false;
  }

  /// @returns The time when this node was first encountered or entered.
  /// @returns 0 if no enter time is registered.
  int EnterTime() const { return visits_[0]; }

  /// @returns The exit time upon traversal of the graph.
  /// @returns 0 if no exit time is registered.
  int ExitTime() const { return visits_[1]; }

  /// @returns The last time this node was visited.
  /// @returns 0 if no last time is registered.
  int LastVisit() const { return visits_[2] ? visits_[2] : visits_[1]; }

  /// @returns The minimum time of the visit.
  /// @returns 0 if no time is registered.
  virtual int min_time() const { return visits_[0]; }

  /// @returns The maximum time of the visit.
  /// @returns 0 if no time is registered.
  virtual int max_time() const { return LastVisit(); }

  /// @returns false if this node was only visited once upon graph traversal.
  /// @returns true if this node was revisited at least one more time.
  bool Revisited() const { return visits_[2]; }

  /// @returns true if this node was visited at least once.
  /// @returns false if this node was never visited upon traversal.
  bool Visited() const { return visits_[0]; }

  /// Clears all the visit information. Resets the visit times to 0s.
  void ClearVisits() { std::fill_n(visits_, 3, 0); }

  /// @returns The positive count of this node.
  int pos_count() const { return pos_count_; }

  /// @returns The negative count of this node.
  int neg_count() const { return neg_count_; }

  /// Increases the count of this node.
  ///
  /// @param[in] positive  Indication of a positive node.
  void AddCount(bool positive) { positive ? ++pos_count_ : ++neg_count_; }

  /// Resets positive and negative counts of this node.
  void ResetCount() {
    pos_count_ = 0;
    neg_count_ = 0;
  }

 private:
  static int next_index_;  ///< Automatic indexation of the next new node.
  int index_;  ///< Index of this node.
  int order_;  ///< Ordering of nodes in the graph.
  int visits_[3];  ///< Traversal array with first, second, and last visits.
  int opti_value_;  ///< Failure propagation optimization value.
  int pos_count_;  ///< The number of occurrences as a positive node.
  int neg_count_;  ///< The number of occurrences as a negative node.
};

/// Representation of a node that is a Boolean constant
/// with True or False state.
class Constant : public Node {
 public:
  /// Constructs a new constant indexed node.
  ///
  /// @param[in] state  Binary state of the Boolean constant.
  explicit Constant(bool state) noexcept;

  /// @returns The state of the constant.
  bool state() const { return state_; }

 private:
  bool state_;  ///< The Boolean value for the constant state.
};

/// Boolean variables in a Boolean formula or graph.
/// Variables can represent the basic events of fault trees.
///
/// Indexation of the variables is special.
/// It starts from 1 and ends with the number of the basic events
/// in the fault tree.
/// This indexation technique helps
/// preprocessing and analysis algorithms
/// optimize their work with basic events.
class Variable : public Node {
 public:
  /// Creates a new indexed variable with its index assigned sequentially.
  Variable() noexcept;

  /// Resets the starting index for variables.
  static void ResetIndex() { next_variable_ = 1; }

 private:
  static int next_variable_;  ///< The next index for a new variable.
};

using NodePtr = std::shared_ptr<Node>;  ///< Shared base nodes in the graph.
using ConstantPtr = std::shared_ptr<Constant>;  ///< Shared Boolean constants.
using VariablePtr = std::shared_ptr<Variable>;  ///< Shared Boolean variables.

/// Boolean operators of gates
/// for representation, preprocessing, and analysis purposes.
/// The operator defines a type and logic of a gate.
///
/// @warning If a new operator is added,
///          all the preprocessing and Boolean graph algorithms
///          must be reviewed and updated.
///          The algorithms may assume
///          for performance and simplicity reasons
///          that these are the only kinds of operators possible.
enum Operator {
  kAnd = 0,  ///< Simple AND gate.
  kOr,  ///< Simple OR gate.
  kVote,  ///< Combination, K/N, or Vote gate representation.
  kXor,  ///< Exclusive OR gate with two inputs.
  kNot,  ///< Boolean negation.
  kNand,  ///< NAND gate.
  kNor,  ///< NOR gate.
  kNull  ///< Special pass-through or NULL gate. This is not NULL set.
};

/// The number of operators in the enum.
/// This number is useful for optimizations and algorithms.
const int kNumOperators = 8;  // Update this number if operators change.

/// State of a gate as a set of Boolean variables.
/// This state helps detect null and unity sets
/// that are formed upon Boolean operations.
enum State {
  kNormalState,  ///< The default case with any set that is not null or unity.
  kNullState,  ///< The set is null. This indicates no failure.
  kUnityState  ///< The set is unity. This set guarantees failure.
};

/// Indexed gate for use in BooleanGraph.
/// Initially this gate can represent any type of gate or logic;
/// however, this gate can be only of OR and AND type
/// at the end of all simplifications and processing.
/// This gate class helps process the fault tree
/// before any complex analysis is done.
class Gate : public Node, public std::enable_shared_from_this<Gate> {
 public:
  /// Argument entry type in the gate's argument containers.
  /// The entry contains
  /// the positive or negative index (indicating a complement)
  /// and a pointer to the argument node.
  ///
  /// @tparam T  The type of the argument node.
  template <class T>
  using Arg = std::pair<int, std::shared_ptr<T>>;

  /// The associative container type to store the gate arguments.
  /// This container type maps the index of the argument to a pointer to it.
  ///
  /// @tparam T  The type of the argument node.
  template <class T>
  using ArgMap = LinearMap<int, std::shared_ptr<T>, std::vector, MoveEraser>;

  /// The ordered set of gate argument indices.
  using ArgSet = boost::container::flat_set<int>;

  /// Creates an indexed gate with its unique index.
  /// It is assumed that smart pointers are used to manage the graph,
  /// and one shared pointer exists for this gate
  /// to manage parent-child hierarchy.
  ///
  /// @param[in] type  The type of this gate.
  explicit Gate(Operator type) noexcept;

  /// Destructs parent information from the arguments.
  ~Gate() noexcept {
    assert(Node::parents().empty());
    Gate::EraseAllArgs();
  }

  /// Clones arguments and parameters.
  /// The semantics of the gate is cloned,
  /// not the gate data like index and parents.
  ///
  /// @returns Shared pointer to a newly created gate.
  ///
  /// @warning This function does not destroy modules.
  ///          If cloning destroys modules,
  ///          module(false) member function must be called.
  GatePtr Clone() noexcept;

  /// @returns Type of this gate.
  Operator type() const { return type_; }

  /// Changes the gate type information.
  /// This function is expected to be used
  /// with only simple AND, OR, NOT, NULL gates.
  ///
  /// @param[in] t  The type for this gate.
  void type(Operator t) {
    assert(t == kAnd || t == kOr || t == kNot || t == kNull);
    type_ = t;
  }

  /// @returns Vote number.
  ///
  /// @warning The function does not validate the vote number,
  ///          nor does it check for the VOTE type of the gate.
  int vote_number() const { return vote_number_; }

  /// Sets the vote number for this gate.
  /// This function is used for K/N gates.
  ///
  /// @param[in] number  The vote number of VOTE gate.
  ///
  /// @warning The function does not validate the vote number,
  ///          nor does it check for the VOTE type of the gate.
  void vote_number(int number) { vote_number_ = number; }

  /// @returns The state of this gate.
  State state() const { return state_; }

  /// @returns true if this gate has become constant.
  bool IsConstant() const { return state_ != kNormalState; }

  /// @returns The ordered set of argument indices of this gate.
  const ArgSet& args() const { return args_; }

  /// Generic accessor to the gate argument containers.
  ///
  /// @tparam T  The type of the argument nodes.
  ///
  /// @returns The map container of the gate arguments with the given type.
  template <class T>
  const ArgMap<T>& args() const;

  /// Marks are used for linear traversal of graphs.
  /// This can be an alternative
  /// to visit information provided by the base Node class.
  ///
  /// @returns The mark of this gate.
  bool mark() const { return mark_; }

  /// Sets the mark of this gate.
  ///
  /// @param[in] flag  Marking with the meaning for the marker.
  void mark(bool flag) { mark_ = flag; }

  /// @returns Pre-assigned index of one of gate's descendants.
  int descendant() const { return descendant_; }

  /// Assigns a descendant index of this gate.
  ///
  /// @param[in] index  Index of the descendant.
  void descendant(int index) { descendant_ = index; }

  /// @returns Pre-assigned index of one of the gate's ancestors.
  int ancestor() { return ancestor_; }

  /// Assigns an ancestor index of this gate.
  ///
  /// @param[in] index  Index of the ancestor.
  void ancestor(int index) { ancestor_ = index; }

  /// @returns The minimum time of visits of the gate's sub-graph.
  /// @returns 0 if no time assignment was performed.
  int min_time() const override { return min_time_; }

  /// Sets the queried minimum visit time of the sub-graph.
  ///
  /// @param[in] time  The positive min time of this gate's sub-graph.
  void min_time(int time) {
    assert(time > 0);
    min_time_ = time;
  }

  /// @returns The maximum time of the visits of the gate's sub-graph.
  /// @returns 0 if no time assignment was performed.
  int max_time() const override { return max_time_; }

  /// Sets the queried maximum visit time of the sub-graph.
  ///
  /// @param[in] time  The positive max time of this gate's sub-graph.
  void max_time(int time) {
    assert(time > 0);
    max_time_ = time;
  }

  /// @returns true if the whole graph of this gate is marked coherent.
  bool coherent() const { return coherent_; }

  /// Sets a coherence flag for the graph rooted by this gate.
  ///
  /// @param[in] flag  true if the whole graph is coherent.
  void coherent(bool flag) { coherent_ = flag; }

  /// @returns true if this gate is set to be a module.
  bool module() const { return module_; }

  /// Sets this gate's module flag.
  ///
  /// @param[in] flag  true for modular gates.
  ///
  /// @pre The gate has already been marked with an opposite flag.
  void module(bool flag) {
    assert(module_ != flag);
    module_ = flag;
  }

  /// Helper function to use the sign of the argument.
  ///
  /// @param[in] arg  One of the arguments of this gate.
  ///
  /// @returns 1 if the argument is positive.
  /// @returns -1 if the argument is negative (complement).
  ///
  /// @warning The function assumes that the argument exists.
  ///          If it doesn't, the return value is invalid.
  int GetArgSign(const NodePtr& arg) const noexcept {
    assert(arg->parents().count(Node::index()) && "Invalid argument.");
    return args_.count(arg->index()) ? 1 : -1;
  }

  /// Helper function for algorithms
  /// to get nodes from argument indices.
  ///
  /// @param[in] index  Positive or negative index of the existing argument.
  ///
  /// @returns Pointer to the argument node of this gate.
  ///
  /// @warning The function assumes that the argument exists.
  ///          If it doesn't, the behavior is undefined.
  /// @warning Never try to use dynamic casts to find the type of the node.
  ///          There are other gate's helper functions
  ///          that will avoid any need for the RTTI or other hacks.
  NodePtr GetArg(int index) const noexcept {
    assert(args_.count(index));
    if (auto it = ext::find(gate_args_, index)) return it->second;

    if (auto it = ext::find(variable_args_, index)) return it->second;

    return constant_args_.find(index)->second;
  }

  /// Adds an argument node to this gate.
  ///
  /// Before adding the argument,
  /// the existing arguments are checked for complements and duplicates.
  /// If there is a complement,
  /// the gate may change its state (erasing its arguments) or type.
  ///
  /// The duplicates are handled according to the logic of the gate.
  /// The caller must be aware of possible changes
  /// due to the logic of the gate.
  ///
  /// @tparam T  The type of the argument node.
  ///
  /// @param[in] index  A positive or negative index of an argument.
  /// @param[in] arg  A pointer to the argument node.
  ///
  /// @warning The function does not indicate invalid state.
  ///          For example, a second argument for NOT or NULL type gates
  ///          is not going to be reported in any way.
  /// @warning This function does not indicate error
  ///          for future additions
  ///          in case the state is nulled or becomes unity.
  /// @warning Duplicate arguments may change the type and state of the gate.
  ///          Depending on the logic of the gate,
  ///          new gates may be introduced
  ///          instead of the existing arguments.
  /// @warning Complex logic gates like VOTE and XOR
  ///          are handled specially
  ///          if the argument is duplicate.
  ///          The caller must be very cautious of
  ///          the side effects of the manipulations.
  template <class T>
  void AddArg(int index, const std::shared_ptr<T>& arg) noexcept {
    assert(index);
    assert(std::abs(index) == arg->index());
    assert(state_ == kNormalState);
    assert(!((type_ == kNot || type_ == kNull) && !args_.empty()));
    assert(!(type_ == kXor && args_.size() > 1));
    assert(vote_number_ >= 0);

    if (args_.count(index)) return Gate::ProcessDuplicateArg(index);
    if (args_.count(-index)) return Gate::ProcessComplementArg(index);

    args_.insert(index);
    Gate::mutable_args<T>().data().emplace_back(index, arg);
    arg->AddParent(shared_from_this());
  }

  /// Transfers this gate's argument to another gate.
  ///
  /// @param[in] index  Positive or negative index of the argument.
  /// @param[in,out] recipient  A new parent for the argument.
  ///
  /// @pre No constant arguments are present.
  void TransferArg(int index, const GatePtr& recipient) noexcept;

  /// Shares this gate's argument with another gate.
  ///
  /// @param[in] index  Positive or negative index of the argument.
  /// @param[in,out] recipient  Another parent for the argument.
  ///
  /// @pre No constant arguments are present.
  void ShareArg(int index, const GatePtr& recipient) noexcept;

  /// Makes all arguments complements of themselves.
  /// This is a helper function to propagate a complement gate
  /// and apply the De Morgan's Law.
  ///
  /// @pre No constant arguments are present.
  void InvertArgs() noexcept;

  /// Replaces an argument with the complement of it.
  /// This is a helper function to propagate a complement gate
  /// and apply the De Morgan's Law.
  ///
  /// @param[in] existing_arg  Positive or negative index of the argument.
  ///
  /// @pre No constant arguments are present.
  void InvertArg(int existing_arg) noexcept;

  /// Adds arguments of an argument gate to this gate.
  /// This is a helper function for gate coalescing.
  /// The argument gate of the same logic is removed
  /// from the arguments list.
  /// The sign of the argument gate is expected to be positive.
  ///
  /// @param[in] arg_gate  The gate which arguments to be added to this gate.
  ///
  /// @pre No constant arguments are present.
  ///
  /// @warning This function does not test
  ///          if the parent and argument logics are
  ///          correct for coalescing.
  void CoalesceGate(const GatePtr& arg_gate) noexcept;

  /// Swaps a single argument of a NULL type argument gate.
  /// This is separate from other coalescing functions
  /// because this function takes into account the sign of the argument.
  ///
  /// @param[in] index  Positive or negative index of the argument gate.
  void JoinNullGate(int index) noexcept;

  /// Changes the state of a gate
  /// or removes a constant argument.
  /// The function determines its actions depending on
  /// the type of a gate and state of an argument.
  ///
  /// @param[in] arg  The pointer the argument of this gate.
  /// @param[in] state  False or True constant state of the argument.
  ///
  /// @note This is a helper function that propagates constants.
  /// @note This function takes into account the sign of the index
  ///       to properly assess the Boolean constant argument.
  /// @note This function may change the state of the gate.
  /// @note This function may change type and parameters of the gate.
  void ProcessConstantArg(const NodePtr& arg, bool state) noexcept;

  /// Removes an argument from the arguments container.
  /// The passed argument index must be
  /// in this gate's arguments container and initialized.
  ///
  /// @param[in] index  The positive or negative index of the existing argument.
  ///
  /// @warning The parent gate may become empty or one-argument gate,
  ///          which must be handled by the caller.
  void EraseArg(int index) noexcept;

  /// Clears all the arguments of this gate.
  void EraseAllArgs() noexcept;

  /// Sets the state of this gate to null
  /// and clears all its arguments.
  /// This function is expected to be used only once.
  void Nullify() noexcept {
    assert(state_ == kNormalState);
    state_ = kNullState;
    Gate::EraseAllArgs();
  }

  /// Sets the state of this gate to unity
  /// and clears all its arguments.
  /// This function is expected to be used only once.
  void MakeUnity() noexcept {
    assert(state_ == kNormalState);
    state_ = kUnityState;
    Gate::EraseAllArgs();
  }

 private:
  /// Mutable getter for the gate arguments.
  ///
  /// @tparam T  The type of the argument nodes.
  ///
  /// @returns The map container of the argument nodes with the given type.
  template <class T>
  ArgMap<T>& mutable_args() {
    return const_cast<ArgMap<T>&>(static_cast<const Gate*>(this)->args<T>());
  }

  /// Process an addition of an argument
  /// that already exists in this gate.
  ///
  /// @param[in] index  Positive or negative index of the existing argument.
  ///
  /// @warning The addition of a duplicate argument
  ///          has a complex set of possible outcomes
  ///          depending on the context.
  ///          The complex corner cases must be handled by the caller.
  void ProcessDuplicateArg(int index) noexcept;

  /// Handles the complex case of duplicate arguments for K/N gates.
  ///
  /// @param[in] index  Positive or negative index of the existing argument.
  ///
  /// @warning New gates may be introduced.
  void ProcessVoteGateDuplicateArg(int index) noexcept;

  /// Process an addition of a complement of an existing argument.
  ///
  /// @param[in] index  Positive or negative index of the argument.
  void ProcessComplementArg(int index) noexcept;

  /// Processes Boolean constant argument with True value.
  ///
  /// @param[in] index  The positive or negative index of the argument.
  ///
  /// @note This is a helper function that propagates constants.
  /// @note This function may change the state of the gate.
  /// @note This function may change type and parameters of the gate.
  void ProcessTrueArg(int index) noexcept;

  /// Processes Boolean constant argument with False value.
  ///
  /// @param[in] index  The positive or negative index of the argument.
  ///
  /// @note This is a helper function that propagates constants.
  /// @note This function may change the state of the gate.
  /// @note This function may change type and parameters of the gate.
  void ProcessFalseArg(int index) noexcept;

  /// Removes Boolean constant arguments from a gate
  /// taking into account the logic.
  /// This is a helper function
  /// for NULL and UNITY set or constant propagation for the graph.
  /// If the final gate is empty,
  /// its state is turned into NULL or UNITY
  /// depending on the logic of the gate
  /// and the logic of the Boolean constant propagation.
  ///
  /// @param[in] index  The positive or negative index of the argument.
  ///
  /// @note This is a helper function that propagates constants,
  ///       so it is coupled with the logic of
  ///       the constant propagation algorithms.
  ///
  /// @warning This function does not handle complex K/N gate parents.
  ///          The logic is not simple for K/N gates,
  ///          so it must be handled by the caller.
  void RemoveConstantArg(int index) noexcept;

  Operator type_;  ///< Type of this gate.
  State state_;  ///< Indication if this gate's state is normal, null, or unity.
  int vote_number_;  ///< Vote number for VOTE gate.
  int descendant_;  ///< Mark by descendant indices.
  int ancestor_;  ///< Mark by ancestor indices.
  int min_time_;  ///< Minimum time of visits of the sub-graph of the gate.
  int max_time_;  ///< Maximum time of visits of the sub-graph of the gate.
  bool mark_;  ///< Marking for linear traversal of a graph.
  bool module_;  ///< Indication of an independent module gate.
  bool coherent_;  ///< Indication of a coherent graph.
  ArgSet args_;  ///< Argument indices of the gate.
  /// Associative containers of gate arguments of certain type.
  /// @{
  ArgMap<Gate> gate_args_;
  ArgMap<Variable> variable_args_;
  ArgMap<Constant> constant_args_;
  /// @}
};

/// @returns The Gate type arguments of a gate.
template <>
inline const Gate::ArgMap<Gate>& Gate::args<Gate>() const { return gate_args_; }

/// @returns The Variable type arguments of a gate.
template <>
inline const Gate::ArgMap<Variable>& Gate::args<Variable>() const {
  return variable_args_;
}

/// @returns The Constant type arguments of a gate.
template <>
inline const Gate::ArgMap<Constant>& Gate::args<Constant>() const {
  return constant_args_;
}

/// Container of unique gates.
/// This container acts like an unordered set of gates.
/// The gates are equivalent
/// if they have the same semantics.
/// However, this set does not test
/// for the isomorphism of the gates' Boolean formulas.
class GateSet {
 public:
  /// Inserts a gate into the set
  /// if it is semantically unique.
  ///
  /// @param[in] gate  The gate to insert.
  ///
  /// @returns A pair of the unique gate and
  ///          the insertion success flag.
  std::pair<GatePtr, bool> insert(const GatePtr& gate) noexcept {
    auto result = table_[gate->type()].insert(gate);
    return {*result.first, result.second};
  }

 private:
  /// Functor for hashing gates by their arguments.
  ///
  /// @note The hashing discards the logic of the gate.
  struct Hash {
    /// Operator overload for hashing.
    ///
    /// @param[in] gate  The gate which hash must be calculated.
    ///
    /// @returns Hash value of the gate
    ///          from its arguments but not logic.
    std::size_t operator()(const GatePtr& gate) const noexcept {
      return boost::hash_range(gate->args().begin(), gate->args().end());
    }
  };
  /// Functor for equality test for gates by their arguments.
  ///
  /// @note The equality discards the logic of the gate.
  struct Equal {
    /// Operator overload for gate argument equality test.
    ///
    /// @param[in] lhs  The first gate.
    /// @param[in] rhs  The second gate.
    ///
    /// @returns true if the gate arguments are equal.
    bool operator()(const GatePtr& lhs, const GatePtr& rhs) const noexcept {
      assert(lhs->type() == rhs->type());
      if (lhs->args() != rhs->args()) return false;
      if (lhs->type() == kVote &&
          lhs->vote_number() != rhs->vote_number()) return false;
      return true;
    }
  };
  /// Container of gates grouped by their types.
  std::array<std::unordered_set<GatePtr, Hash, Equal>, kNumOperators> table_;
};

class Preprocessor;

/// BooleanGraph is a propositional directed acyclic graph (PDAG).
/// This class provides a simpler representation of a fault tree
/// that takes into account the indices of events
/// instead of IDs and pointers.
/// This graph can also be called an indexed fault tree.
///
/// This class is designed
/// to help preprocessing and other graph transformation functions.
///
/// @warning Never hold a shared pointer to any other indexed gate
///          except for the root gate of a Boolean graph.
///          Extra reference count will prevent
///          automatic deletion of the node
///          and management of the structure of the graph.
///          Moreover, the graph may become
///          a multiple-top-event fault tree,
///          which is not the assumption of
///          all the other preprocessing and analysis algorithms.
class BooleanGraph {
  friend class Preprocessor;  ///< The main manipulator of Boolean graphs.

 public:
  /// Constructs a BooleanGraph
  /// starting from the top gate of a fault tree.
  /// Upon construction,
  /// features of the fault tree are recorded
  /// to help preprocessing and analysis functions.
  ///
  /// @param[in] root  The top gate of the fault tree.
  /// @param[in] ccf  Incorporation of CCF gates and events for CCF groups.
  ///
  /// @post The BooleanGraph is stable as long as
  ///       the argument fault tree and its underlying containers are stable.
  ///       If the fault tree has been manipulated (event addition, etc.),
  ///       its BooleanGraph representation is not guaranteed to be the same.
  explicit BooleanGraph(const mef::Gate& root, bool ccf = false) noexcept;

  BooleanGraph(const BooleanGraph&) = delete;
  BooleanGraph& operator=(const BooleanGraph&) = delete;

  /// @returns true if the fault tree is coherent.
  bool coherent() const { return coherent_; }

  /// @returns true if all gates of the fault tree are normalized AND/OR.
  bool normal() const { return normal_; }

  /// @returns The current root gate of the graph.
  const GatePtr& root() const { return root_; }

  /// @returns true if the root must be complemented.
  bool complement() const { return root_sign_ < 0; }

  /// @returns Original basic event
  ///          as initialized in this indexed fault tree.
  ///          The position of a basic event equals (its index - 1).
  const std::vector<mef::BasicEvent*>& basic_events() const {
    return basic_events_;
  }

  /// Helper function to map the results of the indexation
  /// to the original basic events.
  /// This function, for example, helps transform
  /// products with indices into
  /// products with IDs or pointers.
  ///
  /// @param[in] index  Positive index of the basic event.
  ///
  /// @returns Pointer to the original basic event from its index.
  mef::BasicEvent* GetBasicEvent(int index) const {
    assert(index > 0);
    assert(index <= basic_events_.size());
    return basic_events_[index - 1];
  }

  /// Prints the Boolean graph in the shorthand format.
  /// This is a helper for logging and debugging.
  /// The output is the standard error.
  ///
  /// @warning Node visits are used.
  void Print();

  /// Writes Boolean graph properties into logs.
  ///
  /// @pre The graph is valid and well formed.
  /// @pre Logging cutoff level is Debug 4 or higher.
  ///
  /// @post Gate marks are clear.
  ///
  /// @warning Gate marks are manipulated.
  void Log() noexcept;

 private:
  /// Sets the root gate.
  /// This function is helpful for preprocessing.
  ///
  /// @param[in] gate  Replacement root gate.
  void root(const GatePtr& gate) { root_ = gate; }

  /// Holder for nodes that are created from fault tree events.
  /// This is a helper structure
  /// for functions that transform a fault tree into a Boolean graph.
  struct ProcessedNodes {  /// @{
    std::unordered_map<const mef::Gate*, GatePtr> gates;
    std::unordered_map<const mef::BasicEvent*, VariablePtr> variables;
    std::unordered_map<const mef::HouseEvent*, ConstantPtr> constants;
  };  /// @}

  /// Processes a Boolean formula of a gate into a Boolean graph.
  ///
  /// @param[in] formula  The Boolean formula to be processed.
  /// @param[in] ccf  A flag to replace basic events with CCF gates.
  /// @param[in,out] nodes  The mapping of processed nodes.
  ///
  /// @returns Pointer to the newly created indexed gate.
  GatePtr ProcessFormula(const mef::Formula& formula, bool ccf,
                         ProcessedNodes* nodes) noexcept;

  /// Processes a Boolean formula's basic events
  /// into variable arguments of an indexed gate in the Boolean graph.
  /// Basic events are saved for reference in analysis.
  ///
  /// @param[in,out] parent  The parent gate to own the arguments.
  /// @param[in] basic_event  The basic event argument of the formula.
  /// @param[in] ccf  A flag to replace basic events with CCF gates.
  /// @param[in,out] nodes  The mapping of processed nodes.
  void ProcessBasicEvent(const GatePtr& parent, mef::BasicEvent* basic_event,
                         bool ccf, ProcessedNodes* nodes) noexcept;

  /// Processes a Boolean formula's house events
  /// into constant arguments of an indexed gate of the Boolean graph.
  /// Newly created constants are registered for removal for Preprocessor.
  ///
  /// @param[in,out] parent  The parent gate to own the arguments.
  /// @param[in] house_event  The house event argument of the formula.
  /// @param[in,out] nodes  The mapping of processed nodes.
  void ProcessHouseEvent(const GatePtr& parent,
                         const mef::HouseEvent& house_event,
                         ProcessedNodes* nodes) noexcept;

  /// Processes a Boolean formula's gates
  /// into gate arguments of an indexed gate of the Boolean graph.
  ///
  /// @param[in,out] parent  The parent gate to own the arguments.
  /// @param[in] gate  The gate argument of the formula.
  /// @param[in] ccf  A flag to replace basic events with CCF gates.
  /// @param[in,out] nodes  The mapping of processed nodes.
  void ProcessGate(const GatePtr& parent, const mef::Gate& gate, bool ccf,
                   ProcessedNodes* nodes) noexcept;

  /// Sets the visit marks to False for all indexed gates,
  /// starting from the root gate,
  /// that have been visited top-down.
  /// Any function updating and using the visit marks of gates
  /// must ensure to clean visit marks
  /// before running algorithms.
  /// However, cleaning after finishing algorithms is not mandatory.
  ///
  /// @warning If the marks have not been assigned in a top-down traversal,
  ///          this function will fail silently.
  void ClearGateMarks() noexcept;

  /// Sets the visit marks of descendant gates to False
  /// starting from the given gate as the root.
  /// The top-down traversal marking is assumed.
  ///
  /// @param[in,out] gate  The root gate to be traversed and marks.
  ///
  /// @warning If the marks have not been assigned in a top-down traversal,
  ///          starting from the given gate,
  ///          this function will fail silently.
  void ClearGateMarks(const GatePtr& gate) noexcept;

  /// Clears visit time information from all indexed nodes
  /// that have been visited.
  /// Any member function updating and using the visit information of nodes
  /// must ensure to clean visit times
  /// before running algorithms.
  /// However, cleaning after finishing algorithms is not mandatory.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearNodeVisits() noexcept;

  /// Clears visit information from descendant nodes
  /// starting from the given gate as the root.
  ///
  /// @param[in,out] gate  The root gate to be traversed and cleaned.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearNodeVisits(const GatePtr& gate) noexcept;

  /// Clears optimization values of all nodes in the graph.
  /// The optimization values are set to 0.
  /// Resets the number of failed arguments of gates.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearOptiValues() noexcept;

  /// Clears optimization values of nodes.
  /// The optimization values are set to 0.
  /// Resets the number of failed arguments of gates.
  ///
  /// @param[in,out] gate  The root gate to be traversed and cleaned.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearOptiValues(const GatePtr& gate) noexcept;

  /// Clears counts of all nodes in the graph.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearNodeCounts() noexcept;

  /// Clears counts of nodes.
  ///
  /// @param[in,out] gate  The root gate to be traversed and cleaned.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearNodeCounts(const GatePtr& gate) noexcept;

  /// Clears descendant indices of all gates in the graph.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearDescendantMarks() noexcept;

  /// Clears descendant marks of gates.
  ///
  /// @param[in,out] gate  The root gate to be traversed and cleaned.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearDescendantMarks(const GatePtr& gate) noexcept;

  /// Clears ancestor indices of all gates in the graph.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearAncestorMarks() noexcept;

  /// Clears ancestor marks of gates.
  ///
  /// @param[in,out] gate  The root gate to be traversed and cleaned.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearAncestorMarks(const GatePtr& gate) noexcept;

  /// Clears ordering marks of nodes in the graph.
  ///
  /// @post Node order marks are set to 0.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearNodeOrders() noexcept;

  /// Clears ordering marks of descendant nodes of a gate.
  ///
  /// @param[in,out] gate  The root gate to be traversed and cleaned.
  ///
  /// @post The root and descendant node order marks are set to 0.
  ///
  /// @note Gate marks are used for linear time traversal.
  void ClearNodeOrders(const GatePtr& gate) noexcept;

  GatePtr root_;  ///< The root gate of this graph.
  int root_sign_;  ///< The negative or positive sign of the root node.
  bool coherent_;  ///< Indication that the graph does not contain negation.
  bool normal_;  ///< Indication for the graph containing only OR and AND gates.
  std::vector<mef::BasicEvent*> basic_events_;  ///< Mapping for basic events.
  /// Registered house events upon the creation of the Boolean graph.
  std::vector<std::weak_ptr<Constant>> constants_;
  /// Registered NULL type gates upon the creation of the Boolean graph.
  std::vector<std::weak_ptr<Gate>> null_gates_;
};

/// Prints Boolean graph nodes in the shorthand format.
/// @{
std::ostream& operator<<(std::ostream& os, const ConstantPtr& constant);
std::ostream& operator<<(std::ostream& os, const VariablePtr& variable);
std::ostream& operator<<(std::ostream& os, const GatePtr& gate);
/// @}

/// Prints the BooleanGraph as a fault tree in the shorthand format.
/// This function is mostly for debugging purposes.
/// The output is not meant to be human readable.
///
/// @param[in,out] os  Output stream.
/// @param[in] ft  The fault tree to be printed.
///
/// @returns The provided output stream in its original state.
///
/// @warning Visits of nodes must be clean.
///          Visit information may get changed.
std::ostream& operator<<(std::ostream& os, const BooleanGraph* ft);

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_BOOLEAN_GRAPH_H_
