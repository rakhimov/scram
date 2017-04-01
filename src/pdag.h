/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file pdag.h
/// Classes and facilities to represent fault trees
/// as PDAGs with event and gate indices instead of ID names.
/// These facilities are designed to work
/// with FaultTreeAnalysis and Preprocessor classes.
///
/// The terminologies of the graphs and Boolean logic are mixed
/// to represent the PDAG;
/// however, if there is a conflict,
/// the Boolean terminology is preferred.
/// For example, instead of "children", "arguments" are preferred.

#ifndef SCRAM_SRC_PDAG_H_
#define SCRAM_SRC_PDAG_H_

#include <cstdint>
#include <cstdlib>

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/container/flat_set.hpp>
#include <boost/noncopyable.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "ext/find_iterator.h"
#include "ext/index_map.h"
#include "ext/linear_map.h"

namespace scram {

namespace mef {  // Declarations to decouple from the initialization code.
class Gate;
class BasicEvent;
class HouseEvent;
class Formula;
}  // namespace mef

namespace core {

class Gate;  // An indexed gate parent of nodes.
using GatePtr = std::shared_ptr<Gate>;  ///< Shared gates in the graph.
using GateWeakPtr = std::weak_ptr<Gate>;  ///< An acyclic ptr to parent gates.

/// A manager of information about parents.
/// Only gates can manipulate the data.
class NodeParentManager : private boost::noncopyable {
  friend class Gate;  ///< The main manipulator of parent information.

 public:
  using Parent = std::pair<int, GateWeakPtr>;  ///< Parent index and ptr.

  /// A map type of parent gate positive indices and weak pointers to them.
  using ParentMap = ext::linear_map<int, GateWeakPtr, ext::MoveEraser>;

  /// @returns The parents of a node.
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

class Pdag;  // Manager of the graph, node indices and uniqueness.

/// An abstract base class that represents a node in a PDAG.
/// The index of the node is a unique identifier for the node.
/// The node holds weak pointers to the parents
/// that are managed by the parents.
///
/// @pre A node does not outlive its PDAG.
class Node : public NodeParentManager {
 public:
  /// Creates a unique graph node as a member of a PDAG.
  ///
  /// @param[in] graph  The graph this node belongs to.
  explicit Node(Pdag* graph) noexcept;

  virtual ~Node() = 0;  ///< Abstract class.

  /// @returns The host graph of the node.
  Pdag& graph() { return graph_; }

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
  int index_;  ///< Index of this node.
  int order_;  ///< Ordering of nodes in the graph.
  int visits_[3];  ///< Traversal array with first, second, and last visits.
  int opti_value_;  ///< Failure propagation optimization value.
  int pos_count_;  ///< The number of occurrences as a positive node.
  int neg_count_;  ///< The number of occurrences as a negative node.
  Pdag& graph_;  ///< The host graph for the node.
};

/// Representation of a node that is a Boolean constant TRUE.
/// The Node index 1 is reserved for this special argument.
/// There's only one constant per graph.
/// FALSE is represented as NOT TRUE with -1 as its index.
class Constant : public Node {
  friend class Pdag;  // Only one constant per graph must be enforced by PDAG.

 public:
  /// @returns The constant Boolean value.
  bool value() const { return true; }

 private:
  /// Constructs a new Constant node with index 1.
  using Node::Node;
};

/// Boolean variables in a Boolean formula or graph.
/// Variables can represent the basic events of fault trees.
class Variable : public Node {
 public:
  using Node::Node;
};

using NodePtr = std::shared_ptr<Node>;  ///< Shared base nodes in the graph.
using ConstantPtr = std::shared_ptr<Constant>;  ///< Shared Boolean constants.
using VariablePtr = std::shared_ptr<Variable>;  ///< Shared Boolean variables.

/// Boolean operators of gates
/// for representation, preprocessing, and analysis purposes.
/// The operator defines a type and logic of a gate.
///
/// @warning If a new operator is added,
///          all the preprocessing and PDAG algorithms
///          must be reviewed and updated.
///          The algorithms may assume
///          for performance and simplicity reasons
///          that these are the only kinds of operators possible.
enum Operator : std::uint8_t {
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

/// An indexed gate for use in a PDAG.
/// Initially this gate can represent any type of gate or logic;
/// however, this gate can be only of OR and AND type
/// at the end of all simplifications and processing.
/// This gate class helps process the fault tree
/// before any complex analysis is done.
class Gate : public Node, public std::enable_shared_from_this<Gate> {
 public:
  /// An argument entry type in the gate's argument containers.
  /// The entry contains
  /// the positive or negative index (indicating a complement)
  /// and the pointer to the argument node.
  ///
  /// The constant version is to simulate transitive const
  /// so that the reference count of the argument cannot be changed.
  ///
  /// @tparam T  The type of the argument node.
  /// @{
  template <class T>
  using Arg = std::pair<int, std::shared_ptr<T>>;
  template <class T>
  using ConstArg = std::pair<int, const T&>;
  /// @}

  /// An associative container type to store the gate arguments.
  /// This container type maps the index of the argument to the pointer to it.
  ///
  /// @tparam T  The type of the argument node.
  template <class T>
  using ArgMap = ext::linear_map<int, std::shared_ptr<T>, ext::MoveEraser>;

  /// An ordered set of gate argument indices.
  using ArgSet = boost::container::flat_set<int>;

  /// Creates an indexed gate with its unique index.
  /// It is assumed that smart pointers are used to manage the graph,
  /// and one shared pointer exists for this gate
  /// to manage parent-child hierarchy.
  ///
  /// @param[in] type  The type of this gate.
  /// @param[in,out] graph  The host PDAG.
  Gate(Operator type, Pdag* graph) noexcept;

  /// Destructs parent information from the arguments.
  ~Gate() noexcept {
    assert(Node::parents().empty());
    EraseArgs();
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

  /// Changes the logic of the gate.
  /// Depending on the original and new type of the gate,
  /// the graph or gate properties may be changed or recorded.
  ///
  /// @param[in] type  A new type for this gate.
  ///
  /// @pre The new logic is compatible with the existing arguments
  ///      and preserves the gate invariants.
  /// @pre The previous type is not equal to the new one.
  void type(Operator type);

  /// @returns Vote number.
  ///
  /// @pre The vote number is relevant to the gate logic.
  int vote_number() const { return vote_number_; }

  /// Sets the vote number for this gate.
  /// This function is used for K/N gates.
  ///
  /// @param[in] number  The vote number of VOTE gate.
  ///
  /// @pre The vote number is appropriate for the gate logic and arguments.
  void vote_number(int number) { vote_number_ = number; }

  /// @returns true if this gate has become constant.
  bool constant() const { return constant_ != nullptr; }

  /// @returns The ordered set of argument indices of this gate.
  const ArgSet& args() const { return args_; }

  /// Generic accessor to the gate argument containers.
  ///
  /// @tparam T  The type of the argument nodes.
  ///
  /// @returns The map container of the gate arguments with the given type.
  template <class T>
  const ArgMap<T>& args();

  /// Provides const access to gate arguments
  /// w/o exposing the shared pointers.
  /// The arguments are provided by reference.
  ///
  /// @tparam  The type of the arguments.
  ///
  /// @returns Forward-iterable range with ConstArg value type.
  template <class T>
  auto args() const {
    return boost::adaptors::transform(
        const_cast<Gate*>(this)->args<T>(),
        [](const Arg<T>& arg) { return ConstArg<T>{arg.first, *arg.second}; });
  }

  /// Marks are used for linear traversal of graphs.
  /// This can be an alternative
  /// to visit information provided by the base Node class.
  ///
  /// @returns The mark of this gate.
  bool mark() const { return mark_; }

  /// Sets the mark of this gate.
  ///
  /// @param[in] flag  Marking with the meaning for the marker.
  ///
  /// @pre The marks are assigned in a top-down traversal.
  /// @pre The marks are continuous.
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
    if (auto it = ext::find(gate_args_, index))
      return it->second;

    if (auto it = ext::find(variable_args_, index))
      return it->second;
    assert(constant_ && std::abs(index) == constant_->index());
    return constant_;
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
    assert(!constant_);
    assert(!((type_ == kNot || type_ == kNull) && !args_.empty()));
    assert(!(type_ == kXor && args_.size() > 1));
    assert(vote_number_ >= 0);

    if (args_.count(index))
      return ProcessDuplicateArg(index);
    if (args_.count(-index))
      return ProcessComplementArg(index);

    args_.insert(index);
    mutable_args<T>().data().emplace_back(index, arg);
    arg->AddParent(shared_from_this());
  }
  /// Wrapper to add gate arguments with index retrieval from the arg.
  template <class T>
  void AddArg(const std::shared_ptr<T>& arg, bool complement = false) noexcept {
    return AddArg(complement ? -arg->index() : arg->index(), arg);
  }
  /// Wrapper to add arguments from the containers.
  template <class T>
  void AddArg(const Arg<T>& arg) noexcept {
    return AddArg(arg.first, arg.second);
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
  void NegateArgs() noexcept;

  /// Replaces an argument with the complement of it.
  /// This is a helper function to propagate a complement gate
  /// and apply the De Morgan's Law.
  ///
  /// @param[in] existing_arg  Positive or negative index of the argument.
  ///
  /// @pre No constant arguments are present.
  void NegateArg(int existing_arg) noexcept;

  /// Turns all non-coherent arguments of type gate (NOT, NAND, etc.)
  /// into complement arguments.
  ///
  /// This is a helper function for gate normalization
  /// to efficiently normalize non-coherent gates.
  void NegateNonCoherentGateArgs() noexcept {
    for (Arg<Gate>& arg : gate_args_) {
      switch (arg.second->type()) {
        case kNor:
        case kNand:
        case kNot:
          args_.erase(arg.first);
          args_.insert(-arg.first);
          arg.first = -arg.first;
          break;
        default:
          assert("Update the logic if new gate types are introduced.");
      }
    }
  }

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
  /// the type of the gate and state of an argument.
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
  void EraseArgs() noexcept;

  /// Sets the logic of this gate to pass-through
  /// and clears all its arguments except for a Boolean constant.
  /// This function is expected to be used only once.
  ///
  /// @param[in] state  The value for the Boolean constant.
  void MakeConstant(bool state) noexcept;

 private:
  using std::enable_shared_from_this<Gate>::shared_from_this;

  /// Mutable getter for the gate arguments.
  ///
  /// @tparam T  The type of the argument nodes.
  ///
  /// @returns The map container of the argument nodes with the given type.
  template <class T>
  ArgMap<T>& mutable_args() {
    return const_cast<ArgMap<T>&>(args<T>());
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

  /// Processes the addition of a constant to the gate.
  ///
  /// @tparam State  The Boolean Constant value.
  template <bool State>
  void AddConstantArg() noexcept;

  /// Downgrades the logic of the gate
  /// if the number of arguments meets a certain number.
  ///
  /// @param[in] target_type  The logic compatible with the current one.
  /// @param[in] num_args  The number of required arguments.
  void ReduceLogic(Operator target_type, int num_args = 1) noexcept {
    assert(!args_.empty());
    if (args_.size() == num_args)
      type(target_type);
  }

  Operator type_;  ///< Type of this gate.
  bool mark_;  ///< Marking for linear traversal of a graph.
  bool module_;  ///< Indication of an independent module gate.
  bool coherent_;  ///< Indication of a coherent graph.
  int vote_number_;  ///< Vote number for VOTE gate.
  int descendant_;  ///< Mark by descendant indices.
  int ancestor_;  ///< Mark by ancestor indices.
  int min_time_;  ///< Minimum time of visits of the sub-graph of the gate.
  int max_time_;  ///< Maximum time of visits of the sub-graph of the gate.
  ArgSet args_;  ///< Argument indices of the gate.
  /// Associative containers of gate arguments of certain type.
  /// @{
  ArgMap<Gate> gate_args_;
  ArgMap<Variable> variable_args_;
  /// @}

  /// The single constant argument for the whole graph.
  /// The gate is considered constant if this argument is set.
  /// nullptr if the gate doesn't have the Constant argument.
  ConstantPtr constant_;
};

/// @returns The Gate type arguments of a gate.
template <>
inline const Gate::ArgMap<Gate>& Gate::args<Gate>() { return gate_args_; }

/// @returns The Variable type arguments of a gate.
template <>
inline const Gate::ArgMap<Variable>& Gate::args<Variable>() {
  return variable_args_;
}

/// Specialization to handle Boolean constants in arguments.
/// @copydoc Gate::AddArg
template <>
void Gate::AddArg<Constant>(int index, const ConstantPtr& arg) noexcept;

/// PDAG is a propositional directed acyclic graph.
/// This class provides a simpler representation of a fault tree
/// that takes into account the indices of events
/// instead of IDs and pointers.
/// This graph can also be called an indexed fault tree.
///
/// This class is designed
/// to help preprocessing and other graph transformation functions.
///
/// @pre There are no shared pointers to any other graph gate
///      except for the root gate of the graph upon graph transformations.
///      Extra reference count will prevent
///      automatic deletion of the node
///      and management of the structure of the graph.
///      Moreover, the graph may become
///      a multi-root graph,
///      which is not the assumption of
///      all the other preprocessing and analysis algorithms.
class Pdag : private boost::noncopyable {
 public:
  static const int kVariableStartIndex = 2;  ///< The shift value for mapping.
  /// Sequential mapping of Variable indices to other data of type T.
  template<typename T>
  using IndexMap = ext::index_map<kVariableStartIndex, T>;

  /// Generator of unique indices for graph nodes.
  class NodeIndexGenerator {
    friend class Node;  // Access for a new index request.
    /// @returns A new unique index in the graph.
    ///
    /// @param[in,out] graph  A graph within which the index is unique.
    int operator()(Pdag* graph) const { return ++graph->node_index_; }
  };

  /// Registers pass-through or Null logic gates belonging to the graph.
  class NullGateRegistrar {
    friend class Gate;
    /// @param[in] gate  A Null gate with a single argument.
    void operator()(GatePtr gate) const {
      assert(gate->type() == kNull && "Only Null logic gates are expected.");
      if (gate->graph().register_null_gates_)
        gate->graph().null_gates_.emplace_back(std::move(gate));
    }
  };

  /// Various kinds of marks applied to the nodes.
  enum NodeMark {
    kGateMark,  ///< General graph traversal (dirty upon traversal end!).
    kVisit,  ///< General visit times for graph nodes.
    kCount,
    kOptiValue,
    kDescendant,
    kAncestor,
    kOrder
  };

  /// Constructs a graph with no root gate
  /// ready for general purpose (test) Boolean formulas.
  Pdag() noexcept;

  /// Constructs a PDAG
  /// starting from the top gate of a fault tree.
  /// Upon construction,
  /// features of the fault tree are recorded
  /// to help preprocessing and analysis functions.
  ///
  /// @param[in] root  The top gate of the fault tree.
  /// @param[in] ccf  Incorporation of CCF gates and events for CCF groups.
  ///
  /// @pre No new Variable nodes are introduced after the construction.
  ///
  /// @post The PDAG is stable as long as
  ///       the argument fault tree and its underlying containers are stable.
  ///       If the fault tree has been manipulated (event addition, etc.),
  ///       its PDAG representation is not guaranteed to be the same.
  ///
  /// @post The index assignment for variables is special:
  ///       index in [kVariableStartIndex, num of vars + kVariableStartIndex).
  ///       This indexing technique helps
  ///       preprocessing and analysis algorithms
  ///       optimize their work with basic events.
  ///
  /// @post All Gate indices >= (num of vars + kVariableStartIndex).
  explicit Pdag(const mef::Gate& root, bool ccf = false) noexcept;

  /// @returns true if the fault tree is coherent.
  bool coherent() const { return coherent_; }

  /// @param[in] flag  true if fault tree doesn't contain non-coherent gates.
  ///
  /// @todo Implement automatic tracking/marking of coherence.
  void coherent(bool flag) { coherent_ = flag; }

  /// @returns true if all gates of the fault tree are normalized AND/OR.
  bool normal() const { return normal_; }

  /// @param[in] flag  true if the graph has been normalized.
  ///
  /// @todo Consider processing and tracking internally.
  void normal(bool flag) { normal_ = flag; }

  /// @returns The shared pointer to current root gate of the graph.
  ///          nullptr iff the graph has been constructed root-less.
  const GatePtr& root() { return root_; }

  /// @returns The current root gate of the graph.
  ///
  /// @pre The graph has been constructed with a root gate.
  const Gate& root() const { return *root_; }

  /// Sets the root gate.
  /// This function is helpful for transformations.
  ///
  /// @param[in] gate  Replacement root gate.
  void root(const GatePtr& gate) {
    assert(gate && "The graph cannot be made root-less.");
    assert(this == &gate->graph() && "The gate is from a different graph.");
    root_ = gate;
  }

  /// @returns true if graph = ~root.
  /// @{
  bool complement() const { return complement_; }
  bool& complement() { return complement_; }  // Allows XOR setting.
  /// @}

  /// @returns The single Boolean constant for the whole graph.
  ///
  /// @todo Consider limiting access to transform functions and gates.
  const ConstantPtr& constant() const { return constant_; }

  /// @returns true if the graph contains pass-through gates with a constant.
  bool HasConstants() const { return !constant_->parents().empty(); }

  /// @returns true if the graph has at least one pass-through logic gate.
  bool HasNullGates() const { return !null_gates_.empty(); }

  /// @returns true if the graph represents a trivial Boolean function;
  ///               that is, graph = Constant or graph = Variable.
  ///               The only gate is the root pass-through to the simple arg.
  ///
  /// @note The root gate may be swapped with a new one.
  bool IsTrivial() const {
    return !complement_ && root_->type() == kNull &&
           root_->args<Gate>().empty();
  }

  /// Attempts to make the graph trivial if possible.
  ///
  /// @returns true if the graph is trivial or made trivial.
  bool IsTrivial() noexcept;

  /// @returns Original basic event
  ///          as initialized in this indexed fault tree.
  ///          The Variable indices map directly to the original basic events.
  ///
  /// @pre No new Variable nodes are introduced after the construction.
  const IndexMap<const mef::BasicEvent*>& basic_events() const {
    return basic_events_;
  }

  /// Prints the PDAG in the Aralia format.
  /// This is a helper for logging and debugging.
  /// The output is the standard error.
  ///
  /// @warning Node visits are used.
  void Print();

  /// Writes PDAG properties into logs.
  ///
  /// @pre The graph is valid and well formed.
  /// @pre Logging cutoff level is Debug 4 or higher.
  ///
  /// @post Gate marks are clear.
  ///
  /// @warning Gate marks are manipulated.
  void Log() noexcept;

  /// Removes gates of Null logic with a single argument (maybe constant).
  /// That one child arg is transferred to the parent gate,
  /// and the original argument gate is removed from the parent gate.
  ///
  /// All Boolean constants from the PDAG are removed
  /// according to the Boolean logic of the gates
  /// upon passing these args to parent gates.
  ///
  /// @post If there's still a Null logic gate,
  ///       then it's the root of the graph with a single variable/constant,
  ///       and no further processing is required.
  ///
  /// @post If there's still a constant,
  ///       it belongs to the root gate,
  ///       and the whole graph is constant,
  ///       so no further processing is required.
  ///
  /// @warning Gate marks will get cleared by this function.
  void RemoveNullGates() noexcept;

  /// Clears marks from graph nodes.
  ///
  /// @tparam Mark  The kind of the mark.
  template <NodeMark Mark>
  void Clear() noexcept {
    Clear<kGateMark>();
    Clear<Mark>(root_);
    Clear<kGateMark>();
  }

  /// Determines the proper "clear" state for the graph node mark,
  /// and set the clear mark to nodes starting from the given gate.
  ///
  /// @tparam Mark  The kind of the mark.
  ///
  /// @param[in,out] gate  The root gate to start the traversal and cleaning.
  ///
  /// @pre The gate marks are usable for traversal (clear, continuous, etc.).
  template <NodeMark Mark>
  void Clear(const GatePtr& gate) noexcept;

 private:
  /// Holder for nodes that are created from fault tree events.
  /// This is a helper structure
  /// for functions that transform a fault tree into a PDAG.
  struct ProcessedNodes {  /// @{
    std::unordered_map<const mef::Gate*, GatePtr> gates;
    std::unordered_map<const mef::BasicEvent*, VariablePtr> variables;
  };  /// @}

  /// Gathers and initializes Variables from Basic Events.
  /// The gates are gathered but not initialized
  /// to give the sequential indices for the Variables
  /// for establishing the construction invariant.
  ///
  /// @param[in] formula  The Boolean formula with the source variables.
  /// @param[in] ccf  A flag to gather CCF basic events and gates.
  /// @param[in,out] nodes  The mapping of gathered Variables.
  void GatherVariables(const mef::Formula& formula, bool ccf,
                       ProcessedNodes* nodes) noexcept;

  /// Initializes Variable from a Basic Event or
  /// continues the initialization of CCF Events
  /// belonging to the corresponding CCF gates.
  ///
  /// @param[in] basic_event  A Basic Event belonging to a formula.
  /// @param[in] ccf  A flag to gather CCF basic events and gates.
  /// @param[in,out] nodes  The mapping of gathered Variables.
  void GatherVariables(const mef::BasicEvent& basic_event, bool ccf,
                       ProcessedNodes* nodes) noexcept;

  /// Processes a Boolean formula of a gate into a PDAG.
  ///
  /// @param[in] formula  The Boolean formula to be processed.
  /// @param[in] ccf  A flag to replace basic events with CCF gates.
  /// @param[in,out] nodes  The mapping of processed nodes.
  ///
  /// @returns Pointer to the newly created indexed gate.
  ///
  /// @pre The Operator enum in the MEF is the same as in PDAG.
  GatePtr ConstructGate(const mef::Formula& formula, bool ccf,
                        ProcessedNodes* nodes) noexcept;

  /// Processes a Boolean formula's argument events
  /// into arguments of an indexed gate in the PDAG.
  /// Basic events are saved for reference in analysis.
  ///
  /// @tparam T  The type of MEF event.
  ///
  /// @param[in,out] parent  The parent gate to own the arguments.
  /// @param[in] event  The event argument of the formula.
  /// @param[in] ccf  A flag to replace basic events with CCF gates.
  /// @param[in,out] nodes  The mapping of processed nodes.
  template <typename T>
  void AddArg(const GatePtr& parent, const T& event, bool ccf,
              ProcessedNodes* nodes) noexcept;

  /// Propagate NULL type gates bottom-up.
  /// This is a helper function for algorithms
  /// that may produce and need to remove NULL type gates.
  ///
  /// @param[in,out] gate  The gate that is NULL type.
  ///
  /// @post Null logic gates have no parents.
  void PropagateNullGate(const GatePtr& gate) noexcept;

  int node_index_;  ///< Automatic index of the new node.
  bool complement_;  ///< The indication of a complement graph.
  bool coherent_;  ///< Indication that the graph does not contain negation.
  bool normal_;  ///< Indication for the graph containing only OR and AND gates.
  bool register_null_gates_;  ///< Automatically register pass-through gates.
  GatePtr root_;  ///< The root gate of this graph.
  ConstantPtr constant_;  ///< The single constant TRUE for the whole graph.
  /// Mapping for basic events and their Variable indices.
  IndexMap<const mef::BasicEvent*> basic_events_;
  /// Container for NULL type gates to be tracked and cleaned by algorithms.
  /// NULL type gates are created by gates with only one argument.
  std::vector<GateWeakPtr> null_gates_;
};

/// Traverses and visits gates and nodes in the graph.
///
/// @tparam Mark  The "visited" gate mark.
/// @tparam T  The visitor type.
///
/// @param[in,out] gate  The starting gate.
/// @param[in] visit  The visitor for graph nodes.
///
/// @{
template <bool Mark = true, typename T>
void TraverseGates(const GatePtr& gate, T&& visit) noexcept {
  if (gate->mark() == Mark)
    return;
  gate->mark(Mark);
  visit(gate);
  for (const auto& arg : gate->args<Gate>()) {
    TraverseGates<Mark>(arg.second, visit);
  }
}
template <typename T>
void TraverseNodes(const GatePtr& gate, T&& visit) noexcept {
  if (gate->mark())
    return;
  gate->mark(true);
  visit(gate);
  for (const auto& arg : gate->args<Gate>()) {
    TraverseNodes(arg.second, visit);
  }
  for (const auto& arg : gate->args<Variable>()) {
    visit(arg.second);
  }
}
/// @}

/// Specializations for various node mark clearance operations.
///
/// @param[in,out] gate  The starting gate.
/// @{
template <>
inline void Pdag::Clear<Pdag::kGateMark>(const GatePtr& gate) noexcept {
  TraverseGates<false>(gate, [](auto&&) {});
}
template <>
inline void Pdag::Clear<Pdag::kVisit>(const GatePtr& gate) noexcept {
  TraverseNodes(gate, [](auto&& node) {
    if (node->Visited())
      node->ClearVisits();
  });
}
template <>
inline void Pdag::Clear<Pdag::kOptiValue>(const GatePtr& gate) noexcept {
  TraverseNodes(gate, [](auto&& node) { node->opti_value(0); });
}
template <>
inline void Pdag::Clear<Pdag::kCount>(const GatePtr& gate) noexcept {
  TraverseNodes(gate, [](auto&& node) { node->ResetCount(); });
}
template <>
inline void Pdag::Clear<Pdag::kDescendant>(const GatePtr& gate) noexcept {
  TraverseGates(gate, [](const GatePtr& arg) { arg->descendant(0); });
}
template <>
inline void Pdag::Clear<Pdag::kAncestor>(const GatePtr& gate) noexcept {
  TraverseGates(gate, [](const GatePtr& arg) { arg->ancestor(0); });
}
template <>
inline void Pdag::Clear<Pdag::kOrder>(const GatePtr& gate) noexcept {
  TraverseNodes(gate, [](auto&& node) {
    if (node->order())
      node->order(0);
  });
}
/// @}

/// Specialization to clear the generic mark for the whole graph.
template <>
inline void Pdag::Clear<Pdag::kGateMark>() noexcept {
  Clear<Pdag::kGateMark>(root_);
}

/// Prints PDAG nodes in the Aralia format.
/// @{
std::ostream& operator<<(std::ostream& os, const Constant& constant);
std::ostream& operator<<(std::ostream& os, const Variable& variable);
std::ostream& operator<<(std::ostream& os, const GatePtr& gate);
/// @}

/// Prints the PDAG as a fault tree in the Aralia format.
/// This function is mostly for debugging purposes.
///
/// @param[in,out] os  Output stream.
/// @param[in] graph  The PDAG to be printed.
///
/// @returns The provided output stream in its original state.
///
/// @warning Visits of nodes must be clean.
///          Visit information may get changed.
std::ostream& operator<<(std::ostream& os, Pdag* graph);

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_PDAG_H_
