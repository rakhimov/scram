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
/// @file boolean_graph.h
/// Classes and facilities to represent simplified fault trees
/// as Boolean graphs with event and gate indices instead of ID names.
/// This facilities are designed to work
/// with FaultTreeAnalysis and Preprocessor classes.
///
/// The terminologies of the graphs and Boolean logic are mixed
/// to represent the Boolean graph;
/// however, if there is a conflict,
/// the Boolean terminology is preferred.
/// For example, instead of "children", "arguments" are preferred.
#ifndef SCRAM_SRC_BOOLEAN_GRAPH_H_
#define SCRAM_SRC_BOOLEAN_GRAPH_H_

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/weak_ptr.hpp>

namespace scram {

class IGate;  // Indexed gate parent of nodes.

/// @class Node
/// An abstract base class that represents a node in a Boolean graph.
/// The index of the node is a unique identifier for the node.
/// The node holds weak pointers to the parents
/// that are managed by the parents.
class Node {
  friend class IGate;  // To manage parent information.

 public:
  /// Creates a graph node with its index assigned sequentially.
  Node();

  /// Creates a graph node with its index.
  ///
  /// @param[in] index An unique positive index of this node.
  ///
  /// @warning The index is not validated upon instantiation.
  explicit Node(int index);

  virtual ~Node() = 0;  ///< Abstract class.

  /// @returns The index of this node.
  inline int index() const { return index_; }

  /// Resets the starting index.
  inline static void ResetIndex() { next_index_ = 1e6; }

  /// @returns Parents of this gate.
  inline const boost::unordered_map<int, boost::weak_ptr<IGate> >&
      parents() const {
    return parents_;
  }

  /// @returns Optimization value for failure propagation.
  inline int opti_value() const { return opti_value_; }

  /// Sets the optimization value for failure propagation.
  ///
  /// @param[in] val Value that makes sense to the caller.
  inline void opti_value(int val) { opti_value_ = val; }

  /// Registers the visit time for this node upon graph traversal.
  /// This information can be used to detect dependencies.
  ///
  /// @param[in] time The current visit time of this node. It must be positive.
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
  inline int EnterTime() const { return visits_[0]; }

  /// @returns The exit time upon traversal of the graph.
  /// @returns 0 if no exit time is registered.
  inline int ExitTime() const { return visits_[1]; }

  /// @returns The last time this node was visited.
  /// @returns 0 if no last time is registered.
  inline int LastVisit() const { return visits_[2] ? visits_[2] : visits_[1]; }

  /// @returns The minimum time of the visit.
  /// @returns 0 if no time is registered.
  inline virtual int min_time() const { return visits_[0]; }

  /// @returns The maximum time of the visit.
  /// @returns 0 if no time is registered.
  inline virtual int max_time() const {
    return visits_[2] ? visits_[2] : visits_[1] ? visits_[1] : visits_[0];
  }

  /// @returns false if this node was only visited once upon graph traversal.
  /// @returns true if this node was revisited at one more time.
  inline bool Revisited() const { return visits_[2] ? true : false; }

  /// @returns true if this node was visited at least once.
  /// @returns false if this node was never visited upon traversal.
  inline bool Visited() const { return visits_[0] ? true : false; }

  /// Clears all the visit information. Resets the visit times to 0s.
  inline void ClearVisits() { return std::fill(visits_, visits_ + 3, 0); }

 private:
  Node(const Node&);  ///< Restrict copy construction.
  Node& operator=(const Node&);  ///< Restrict copy assignment.

  static int next_index_;  ///< Automatic indexation of the next new node.
  int index_;  ///< Index of this node.
  /// This is a traversal array containing first, second, and last visits.
  int visits_[3];
  boost::unordered_map<int, boost::weak_ptr<IGate> > parents_;  ///< Parents.
  int opti_value_;  ///< Failure propagation optimization value.
};

/// @class Constant
/// Representation of a node that is a Boolean constant
/// with True or False state.
class Constant : public Node {
 public:
  /// Constructs a new constant indexed node.
  ///
  /// @param[in] state Binary state of the Boolean constant.
  explicit Constant(bool state);

  /// @returns The state of the constant.
  inline bool state() const { return state_; }

 private:
  Constant(const Constant&);  ///< Restrict copy construction.
  Constant& operator=(const Constant&);  ///< Restrict copy assignment.

  bool state_;  ///< The Boolean value for the constant state.
};

/// @class Variable
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
  Variable();

  /// Resets the starting index for variables.
  inline static void ResetIndex() { next_variable_ = 1; }

 private:
  Variable(const Variable&);  ///< Restrict copy construction.
  Variable& operator=(const Variable&);  ///< Restrict copy assignment.

  static int next_variable_;  ///< The next index for a new variable.
};

/// @enum Operator
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
  kAndGate = 0,  ///< Simple AND gate.
  kOrGate,  ///< Simple OR gate.
  kAtleastGate,  ///< Combination, K/N, or Vote gate representation.
  kXorGate,  ///< Exclusive OR gate with two inputs.
  kNotGate,  ///< Boolean negation.
  kNandGate,  ///< NAND gate.
  kNorGate,  ///< NOR gate.
  kNullGate  ///< Special pass-through or NULL gate. This is not NULL set.
};

/// The number of operators in the enum.
/// This number is useful for optimizations and algorithms.
static const int kNumOperators = 8;  // Update this number if operators change.

/// @enum State
/// State of a gate as a set of Boolean variables.
/// This state helps detect null and unity sets
/// that are formed upon Boolean operations.
enum State {
  kNormalState,  ///< The default case with any set that is not null or unity.
  kNullState,  ///< The set is null. This indicates no failure.
  kUnityState  ///< The set is unity. This set guarantees failure.
};

/// @class IGate
/// Indexed gate for use in BooleanGraph.
/// Initially this gate can represent any type of gate or logic;
/// however, this gate can be only of OR and AND type
/// at the end of all simplifications and processing.
/// This gate class helps process the fault tree
/// before any complex analysis is done.
class IGate : public Node, public boost::enable_shared_from_this<IGate> {
 public:
  typedef boost::shared_ptr<Node> NodePtr;
  typedef boost::shared_ptr<Constant> ConstantPtr;
  typedef boost::shared_ptr<Variable> VariablePtr;
  typedef boost::shared_ptr<IGate> IGatePtr;

  /// Creates an indexed gate with its unique index.
  /// It is assumed that smart pointers are used to manage the graph,
  /// and one shared pointer exists for this gate
  /// to manage parent-child hierarchy.
  ///
  /// @param[in] type The type of this gate.
  explicit IGate(const Operator& type);

  /// Destructs parent information from the arguments.
  ~IGate() {
    assert(Node::parents().empty());
    IGate::EraseAllArgs();
  }

  /// @returns Type of this gate.
  inline const Operator& type() const { return type_; }

  /// Changes the gate type information.
  /// This function is expected to be used
  /// with only simple AND, OR, NOT, NULL gates.
  ///
  /// @param[in] t The type for this gate.
  inline void type(const Operator& t) {
    assert(t == kAndGate || t == kOrGate || t == kNotGate || t == kNullGate);
    type_ = t;
  }

  /// @returns Vote number.
  ///
  /// @warning The function does not validate the vote number,
  ///          nor does it check for the ATLEAST type of the gate.
  inline int vote_number() const { return vote_number_; }

  /// Sets the vote number for this gate.
  /// This function is used for K/N gates.
  ///
  /// @param[in] number The vote number of ATLEAST gate.
  ///
  /// @warning The function does not validate the vote number,
  ///          nor does it check for the ATLEAST type of the gate.
  inline void vote_number(int number) { vote_number_ = number; }

  /// @returns The state of this gate.
  inline const State& state() const { return state_; }

  /// @returns Arguments of this gate.
  inline const std::set<int>& args() const { return args_; }

  /// @returns Arguments of this gate that are indexed gates.
  inline const boost::unordered_map<int, IGatePtr>& gate_args() const {
    return gate_args_;
  }

  /// @returns Arguments of this gate that are variables.
  inline const boost::unordered_map<int, VariablePtr>& variable_args() const {
    return variable_args_;
  }

  /// @returns Arguments of this gate that are indexed constants.
  inline const boost::unordered_map<int, ConstantPtr>& constant_args() const {
    return constant_args_;
  }

  /// Marks are used for linear traversal of graphs.
  /// This can be an alternative
  /// to visit information provided by the base Node class.
  ///
  /// @returns The mark of this gate.
  inline bool mark() const { return mark_; }

  /// Sets the mark of this gate.
  ///
  /// @param[in] flag Marking with the meaning for the marker.
  inline void mark(bool flag) { mark_ = flag; }

  /// @returns The minimum time of visits of the gate's sub-graph.
  /// @returns 0 if no time assignment was performed.
  inline int min_time() const { return min_time_; }

  /// Sets the queried minimum visit time of the sub-graph.
  ///
  /// @param[in] time The positive min time of this gate's sub-graph.
  inline void min_time(int time) {
    assert(time > 0);
    min_time_ = time;
  }

  /// @returns The maximum time of the visits of the gate's sub-graph.
  /// @returns 0 if no time assignment was performed.
  inline int max_time() const { return max_time_; }

  /// Sets the queried maximum visit time of the sub-graph.
  ///
  /// @param[in] time The positive max time of this gate's sub-graph.
  inline void max_time(int time) {
    assert(time > 0);
    max_time_ = time;
  }

  /// @returns true if this gate is set to be a module.
  /// @returns false if it is not yet set to be a module.
  inline bool IsModule() const { return module_; }

  /// Adds an argument gate to this gate.
  /// Before adding the argument,
  /// the existing arguments are checked for complements and duplicates.
  /// If there is a complement,
  /// the gate may change its state (erasing its arguments) or type.
  /// The duplicates are handled according to the logic of the gate.
  /// The caller must be aware of possible changes
  /// due to the logic of the gate.
  ///
  /// @param[in] arg A positive or negative index of an argument.
  /// @param[in] gate A pointer to the argument gate.
  ///
  /// @returns false if there final state of the parent is normal.
  /// @returns true if the parent has become constant.
  ///
  /// @warning The function does not indicate invalid state.
  ///          For example, a second argument for NOT or NULL type gates
  ///          is not going to be reported in any way.
  /// @warning This function does not indicate error
  ///          for future additions
  ///          in case the state is nulled or becomes unity.
  /// @warning Complex logic gates like ATLEAST and XOR
  ///          are handled specially
  ///          if the argument is duplicate.
  ///          The caller must be very cautious of
  ///          the side effects of the manipulations.
  bool AddArg(int arg, const IGatePtr& gate);

  /// Adds an argument variable to this gate.
  /// Before adding the argument,
  /// the existing arguments are checked for complements and duplicates.
  /// If there is a complement,
  /// the gate may change its state (erasing its arguments) or type.
  /// The duplicates are handled according to the logic of the gate.
  /// The caller must be aware of possible changes
  /// due to the logic of the gate.
  ///
  /// @param[in] arg A positive or negative index of an argument.
  /// @param[in] variable A pointer to the argument variable.
  ///
  /// @returns false if there final state of the parent is normal.
  /// @returns true if the parent has become constant.
  ///
  /// @warning The function does not indicate invalid state.
  ///          For example, a second argument for NOT or NULL type gates
  ///          is not going to be reported in any way.
  /// @warning This function does not indicate error
  ///          for future additions
  ///          in case the state is nulled or becomes unity.
  /// @warning Complex logic gates like ATLEAST and XOR
  ///          are handled specially
  ///          if the argument is duplicate.
  ///          The caller must be very cautious of
  ///          the side effects of the manipulations.
  bool AddArg(int arg, const VariablePtr& variable);

  /// Adds a constant argument to this gate.
  /// Before adding the argument,
  /// the existing arguments are checked for complements and duplicates.
  /// If there is a complement,
  /// the gate may change its state (erasing its arguments) or type.
  /// The duplicates are handled according to the logic of the gate.
  /// The caller must be aware of possible changes
  /// due to the logic of the gate.
  ///
  /// @param[in] arg A positive or negative index of an argument.
  /// @param[in] constant A pointer to the argument that is a Constant.
  ///
  /// @returns false if there final state of the parent is normal.
  /// @returns true if the parent has become constant.
  ///
  /// @warning The function does not indicate invalid state.
  ///          For example, a second argument for NOT or NULL type gates
  ///          is not going to be reported in any way.
  /// @warning This function does not indicate error
  ///          for future additions
  ///          in case the state is nulled or becomes unity.
  /// @warning Complex logic gates like ATLEAST and XOR
  ///          are handled specially
  ///          if the argument is duplicate.
  ///          The caller must be very cautious of
  ///          the side effects of the manipulations.
  bool AddArg(int arg, const ConstantPtr& constant);

  /// Transfers this gates's argument to another gate.
  ///
  /// @param[in] arg Positive or negative index of the argument.
  /// @param[in,out] recipient A new parent for the argument.
  ///
  /// @returns false if there final state of the recipient is normal.
  /// @returns true if the recipient becomes constant.
  bool TransferArg(int arg, const IGatePtr& recipient);

  /// Shares this gates's argument with another gate.
  ///
  /// @param[in] arg Positive or negative index of the argument.
  /// @param[in,out] recipient Another parent for the argument.
  ///
  /// @returns false if there final state of the recipient is normal.
  /// @returns true if the recipient becomes constant.
  bool ShareArg(int arg, const IGatePtr& recipient);

  /// Makes all arguments complements of themselves.
  /// This is a helper function to propagate a complement gate
  /// and apply De Morgan's Law.
  void InvertArgs();

  /// Replaces an argument with the complement of it.
  /// This is a helper function to propagate a complement gate
  /// and apply De Morgan's Law.
  ///
  /// @param[in] existing_arg Positive or negative index of the argument.
  void InvertArg(int existing_arg);

  /// Adds arguments of an argument gate to this gate.
  /// This is a helper function for gate coalescing.
  /// The argument gate of the same logic is removed
  /// from the arguments list.
  /// The sign of the argument gate is expected to be positive.
  ///
  /// @param[in] arg_gate The gate which arguments to be added to this gate.
  ///
  /// @returns false if the final set is null or unity.
  /// @returns true if the addition is successful with a normal final state.
  ///
  /// @warning This function does not test
  ///          if the parent and argument logics are
  ///          correct for coalescing.
  bool JoinGate(const IGatePtr& arg_gate);

  /// Swaps a single argument of a NULL type argument gate.
  /// This is separate from other coalescing functions
  /// because this function takes into account the sign of the argument.
  ///
  /// @param[in] index Positive or negative index of the argument gate.
  ///
  /// @returns false if the final set is null or unity.
  /// @returns true if the addition is successful with a normal final state.
  bool JoinNullGate(int index);

  /// Directly copies arguments from another gate.
  /// This is a helper function for initialization of gates' copies.
  ///
  /// @param[in] gate The gate which arguments will be copied.
  inline void CopyArgs(const IGatePtr& gate) {
    assert(args_.empty());
    IGate::AddArg(gate->index(), gate);  // This is a hack to keep the parent
    IGate::JoinGate(gate);               // information updated.
  }

  /// Removes an argument from the arguments container.
  /// The passed argument index must be
  /// in this gate's arguments container and initialized.
  ///
  /// @param[in] arg The positive or negative index of the existing argument.
  ///
  /// @warning The parent gate may become empty or one-argument gate,
  ///          which must be handled by the caller.
  inline void EraseArg(int arg) {
    assert(arg != 0);
    assert(args_.count(arg));
    args_.erase(arg);
    NodePtr node;
    if (gate_args_.count(arg)) {
      node = gate_args_.find(arg)->second;
      gate_args_.erase(arg);
    } else if (constant_args_.count(arg)) {
      node = constant_args_.find(arg)->second;
      constant_args_.erase(arg);
    } else {
      node = variable_args_.find(arg)->second;
      assert(variable_args_.count(arg));
      variable_args_.erase(arg);
    }
    assert(node->parents_.count(Node::index()));
    node->parents_.erase(Node::index());
  }

  /// Clears all the arguments of this gate.
  inline void EraseAllArgs() {
    while (!args_.empty()) IGate::EraseArg(*args_.rbegin());
  }

  /// Sets the state of this gate to null
  /// and clears all its arguments.
  /// This function is expected to be used only once.
  inline void Nullify() {
    assert(state_ == kNormalState);
    state_ = kNullState;
    IGate::EraseAllArgs();
  }

  /// Sets the state of this gate to unity
  /// and clears all its arguments.
  /// This function is expected to be used only once.
  inline void MakeUnity() {
    assert(state_ == kNormalState);
    state_ = kUnityState;
    IGate::EraseAllArgs();
  }

  /// Turns this gate's module flag on.
  /// This should be one time operation.
  inline void TurnModule() {
    assert(!module_);
    module_ = true;
  }

  /// Registers a failure of an argument.
  /// Depending on the logic of the gate,
  /// sets the failure of this gate.
  ///
  /// @note The actual failure or existence of the argument is not checked.
  void ArgFailed();

  /// Resets this gates failure value
  /// and information about the number of failed arguments.
  inline void ResetArgFailure() { num_failed_args_ = 0; }

 private:
  IGate(const IGate&);  ///< Restrict copy construction.
  IGate& operator=(const IGate&);  ///< Restrict copy assignment.

  /// Process an addition of an argument
  /// that already exists in this gate.
  ///
  /// @param[in] index Positive or negative index of the existing argument.
  ///
  /// @returns false if the final set is null or unity.
  /// @returns true if the addition is successful with a normal final state.
  ///
  /// @warning The addition of a duplicate argument
  ///          has a complex set of possible outcomes
  ///          depending on the context.
  ///          The complex corner cases must be handled by the caller.
  bool ProcessDuplicateArg(int index);

  /// Process an addition of a complement of an existing argument.
  ///
  /// @param[in] index Positive or negative index of the argument.
  ///
  /// @returns false if the final set is null or unity.
  /// @returns true if the addition is successful with a normal final state.
  bool ProcessComplementArg(int index);

  Operator type_;  ///< Type of this gate.
  State state_;  ///< Indication if this gate's state is normal, null, or unity.
  int vote_number_;  ///< Vote number for ATLEAST gate.
  bool mark_;  ///< Marking for linear traversal of a graph.
  int min_time_;  ///< Minimum time of visits of the sub-graph of the gate.
  int max_time_;  ///< Maximum time of visits of the sub-graph of the gate.
  bool module_;  ///< Indication of an independent module gate.
  std::set<int> args_;  ///< Arguments of the gate.
  /// Arguments that are gates.
  boost::unordered_map<int, IGatePtr> gate_args_;
  /// Arguments that are variables.
  boost::unordered_map<int, VariablePtr> variable_args_;
  /// Arguments that are constant like house events.
  boost::unordered_map<int, ConstantPtr> constant_args_;
  /// The number of arguments failed upon failure propagation.
  int num_failed_args_;
};

class BasicEvent;
class HouseEvent;
class Gate;
class Formula;

/// @class BooleanGraph
/// BooleanGraph is a propositional directed acyclic graph (PDAG).
/// This class provides a simpler representation of a fault tree
/// that takes into account the indices of events
/// instead of IDs and pointers.
/// This graph can also be called an indexed fault tree.
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
 public:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<IGate> IGatePtr;

  /// Constructs a BooleanGraph
  /// starting from the top gate of a fault tree.
  /// Upon construction,
  /// features of the fault tree are recorded
  /// to help preprocessing and analysis functions.
  ///
  /// @param[in] root The top gate of the fault tree.
  /// @param[in] ccf Incorporation of CCF gates and events for CCF groups.
  explicit BooleanGraph(const GatePtr& root, bool ccf = false);

  /// @returns true if the fault tree is coherent.
  inline bool coherent() const { return coherent_; }

  /// @returns true if the initialized fault tree has constants.
  inline bool constants() const { return constants_; }

  /// @returns true if the initialized fault tree has only OR and AND gates.
  inline bool normal() const { return normal_; }

  /// @returns The current root gate of the graph.
  inline const IGatePtr& root() const { return root_; }

  /// Sets the the root gate.
  /// This function is helpful for preprocessing.
  ///
  /// @param[in] gate Replacement root gate.
  inline void root(const IGatePtr& gate) { root_ = gate; }

  /// @returns Original basic event
  ///          as initialized in this indexed fault tree.
  ///          The position of a basic event equals (its index - 1).
  inline const std::vector<BasicEventPtr>& basic_events() const {
    return basic_events_;
  }

  /// Helper function to map the results of the indexation
  /// to the original basic events.
  /// This function, for example, helps transform
  /// minimal cut sets with indices into
  /// minimal cut sets with IDs or pointers.
  ///
  /// @param[in] index Positive index of the basic event.
  ///
  /// @returns Pointer to the original basic event from its index.
  inline const BasicEventPtr& GetBasicEvent(int index) const {
    assert(index > 0);
    assert(index <= basic_events_.size());
    return basic_events_[index - 1];
  }

 private:
  typedef boost::shared_ptr<Formula> FormulaPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<Node> NodePtr;
  typedef boost::shared_ptr<Constant> ConstantPtr;
  typedef boost::shared_ptr<Variable> VariablePtr;

  /// Mapping to string gate types to enum gate types.
  static const std::map<std::string, Operator> kStringToType_;

  /// Processes a Boolean formula of a gate into a Boolean graph.
  ///
  /// @param[in] formula The Boolean formula to be processed.
  /// @param[in] ccf A flag to replace basic events with CCF gates.
  /// @param[in,out] id_to_node The mapping of already processed nodes.
  ///
  /// @returns Pointer to the newly created indexed gate.
  IGatePtr ProcessFormula(
      const FormulaPtr& formula,
      bool ccf,
      boost::unordered_map<std::string, NodePtr>* id_to_node);

  /// Processes a Boolean formula's basic events
  /// into variable arguments of an indexed gate of the Boolean graph.
  ///
  /// @param[in,out] parent The parent gate to own the arguments.
  /// @param[in] basic_events The collection of basic events of the formula.
  /// @param[in] ccf A flag to replace basic events with CCF gates.
  /// @param[in,out] id_to_node The mapping of already processed nodes.
  void ProcessBasicEvents(
      const IGatePtr& parent,
      const std::vector<BasicEventPtr>& basic_events,
      bool ccf,
      boost::unordered_map<std::string, NodePtr>* id_to_node);

  /// Processes a Boolean formula's house events
  /// into constant arguments of an indexed gate of the Boolean graph.
  ///
  /// @param[in,out] parent The parent gate to own the arguments.
  /// @param[in] house_events The collection of house events of the formula.
  /// @param[in,out] id_to_node The mapping of already processed nodes.
  void ProcessHouseEvents(
      const IGatePtr& parent,
      const std::vector<HouseEventPtr>& house_events,
      boost::unordered_map<std::string, NodePtr>* id_to_node);

  /// Processes a Boolean formula's gates
  /// into gate arguments of an indexed gate of the Boolean graph.
  ///
  /// @param[in,out] parent The parent gate to own the arguments.
  /// @param[in] gates The collection of gates of the formula.
  /// @param[in] ccf A flag to replace basic events with CCF gates.
  /// @param[in,out] id_to_node The mapping of already processed nodes.
  void ProcessGates(const IGatePtr& parent,
                    const std::vector<GatePtr>& gates,
                    bool ccf,
                    boost::unordered_map<std::string, NodePtr>* id_to_node);

  IGatePtr root_;  ///< The root gate of this graph.
  std::vector<BasicEventPtr> basic_events_;  ///< Mapping for basic events.
  bool coherent_;  ///< Indication that the graph does not contain negation.
  bool constants_;  ///< Indication that the original graph contains constants.
  bool normal_;  ///< Indication for the graph containing only OR and AND gates.
};

/// Prints indexed house events or constants in the shorthand format.
///
/// @param[in,out] os Output stream.
/// @param[in] constant The constant to be printed.
///
/// @warning Visit information may get changed.
std::ostream& operator<<(std::ostream& os,
                         const boost::shared_ptr<Constant>& constant);

/// Prints indexed variables as basic events in the shorthand format.
///
/// @param[in,out] os Output stream.
/// @param[in] variable The basic event to be printed.
///
/// @warning Visit information may get changed.
std::ostream& operator<<(std::ostream& os,
                         const boost::shared_ptr<Variable>& variable);

/// Prints indexed gates in the shorthand format.
/// The gates that have become a constant are named "GC".
/// The gates that are modules are named "GM".
///
/// @param[in,out] os Output stream.
/// @param[in] gate The gate to be printed.
///
/// @warning Visit information may get changed.
std::ostream& operator<<(std::ostream& os,
                         const boost::shared_ptr<IGate>& gate);

/// Prints the BooleanGraph as a fault tree in the shorthand format.
/// This function is mostly for debugging purposes.
/// The output is not meant to be human readable.
///
/// @param[in,out] os Output stream.
/// @param[in] ft The fault tree to be printed.
///
/// @warning Visits of nodes must be clean.
///          Visit information may get changed.
std::ostream& operator<<(std::ostream& os, const BooleanGraph* ft);

}  // namespace scram

#endif  // SCRAM_SRC_BOOLEAN_GRAPH_H_
