/// @file indexed_fault_tree.h
/// Classes and facilities to represent simplified fault trees wth event and
/// gate indices instead of ID names. This facility is designed to work with
/// FaultTreeAnalysis class.
#ifndef SCRAM_SRC_INDEXED_FAULT_TREE_H_
#define SCRAM_SRC_INDEXED_FAULT_TREE_H_

#include <map>
#include <set>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace scram {

/// @class Node
/// An abstract base class that represents a node in an indexed fault tree
/// graph. The index of the node is a unique identifier for the node.
/// The node holds a weak pointer to the parent that is managed by the parent.
class Node {
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

  /// @returns parents of this gate.
  inline const std::set<int>& parents() { return parents_; }

  /// Adds a parent of this gate.
  ///
  /// @param[in] index Positive index of the parent.
  inline void AddParent(int index) {
    assert(index > 0);
    parents_.insert(index);
  }

  /// Removes a parent of this gate.
  ///
  /// @param[in] index Positive index of the existing parent.
  inline void EraseParent(int index) {
    assert(index > 0);
    assert(parents_.count(index));
    parents_.erase(index);
  }

  /// Registers the visit time for this node upon tree traversal.
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

  /// @returns The exit time upon traversal of the tree.
  /// @returns 0 if no exit time is registered.
  inline int ExitTime() const { return visits_[1]; }

  /// @returns The last time this node was visited.
  /// @returns 0 if no last time is registered.
  inline int LastVisit() const { return visits_[2] ? visits_[2] : visits_[1]; }

  /// @returns false if this node was only visited once upon tree traversal.
  /// @returns true if this node was revisited at one more time.
  inline bool Revisited() const { return visits_[2] ? true : false; }

  /// @returns true if this node was visited at least once.
  /// @returns false if this node was never visited upon traversal.
  inline bool Visited() const { return visits_[0] ? true : false; }

  /// Clears all the visit information. Resets the visit times to 0s.
  inline void ClearVisits() { return std::fill(visits_, visits_ + 3, 0); }

 private:
  static int next_index_;  ///< Automatic indexation of the next new node.
  int index_;  ///< Index of this node.
  /// This is a traversal array containing first, second, and last visits.
  int visits_[3];
  std::set<int> parents_;  ///< Parents of this node.
};

/// @class Constant
/// Representation of a node that is a Boolean constant with True or False
/// state.
class Constant : public Node {
 public:
  /// Constructs a new constant indexed node.
  ///
  /// @param[in] state Binary state of the Boolean constant.
  explicit Constant(bool state);

  ///@returns The state of the constant.
  inline bool state() { return state_; }

 private:
  bool state_;  ///< The Boolean value for the constant state.
};

/// @class IBasicEvent
/// Indexed basic events in an indexed fault tree.
/// Indexation of the basic events are special. It starts from 1 and ends with
/// the number of the basic events in the fault tree. This indexation technique
/// helps preprocessing and analysis algorithms optimize their work with basic
/// events.
class IBasicEvent : public Node {
 public:
  /// Creates a new indexed basic event with its index assigned sequentially.
  IBasicEvent();

  /// Resets the starting index for basic events.
  inline static void ResetIndex() { next_basic_event_ = 1; }

 private:
  static int next_basic_event_;  ///< The next index for the basic event.
};

/// @enum GateType
/// Types of gates for representation, preprocessing, and analysis purposes.
enum GateType {
  kAndGate,  ///< Simple AND gate.
  kOrGate,  ///< Simple OR gate.
  kAtleastGate,  ///< Combination or Vote gate representation.
  kXorGate,  ///< Exclusive OR gate with two inputs.
  kNotGate,  ///< Boolean negation.
  kNandGate,  ///< NAND gate.
  kNorGate,  ///< NOR gate.
  kNullGate  ///< Special pass-through or NULL gate. This is not NULL set.
};

/// @enum State
/// State of a gate as a set of events with a logical operator.
/// This state helps detect null and unity sets that formed upon Boolean
/// operations.
enum State {
  kNormalState,  ///< The default case with any set that is not null or unity.
  kNullState,  ///< The set is null. This indicates no failure.
  kUnityState  ///< The set is unity. This set guarantees failure.
};

/// @class IGate
/// This indexed gate is for use in IndexedFaultTree.
/// Initially this gate can represent any type of gate or logic; however,
/// this gate can be only of OR and AND type at the end of all simplifications
/// and processing. This gate class helps to process the fault tree before
/// any complex analysis is done.
class IGate : public Node {
 public:
  typedef boost::shared_ptr<Constant> ConstantPtr;
  typedef boost::shared_ptr<IBasicEvent> IBasicEventPtr;
  typedef boost::shared_ptr<IGate> IGatePtr;

  /// Creates an indexed gate with its unique index.
  ///
  /// @param[in] type The type of this gate.
  explicit IGate(const GateType& type);

  /// @returns Type of this gate.
  inline const GateType& type() const { return type_; }

  /// Changes the gate type information. This function is expected to be used
  /// with only simple AND, OR, NOT, NULL gates.
  ///
  /// @param[in] t The type for this gate.
  inline void type(const GateType& t) {
    assert(t == kAndGate || t == kOrGate || t == kNotGate || t == kNullGate);
    type_ = t;
  }

  /// @returns Vote number.
  inline int vote_number() const { return vote_number_; }

  /// Sets the vote number for this gate. The function does not check if
  /// the gate type is ATLEAST; nor does it validate the number.
  ///
  /// @param[in] number The vote number of ATLEAST gate.
  inline void vote_number(int number) { vote_number_ = number; }

  /// @returns Children of this gate.
  inline const std::set<int>& children() const { return children_; }

  /// @returns Children of this gate that are indexed gates.
  inline const boost::unordered_map<int, IGatePtr>& gate_children() const {
    return gate_children_;
  }

  /// @returns Children of this gate that are indexed basic events.
  inline const boost::unordered_map<int, IBasicEventPtr>&
      basic_event_children() const {
    return basic_event_children_;
  }

  /// @returns Children of this gate that are indexed constants.
  inline const boost::unordered_map<int, ConstantPtr>&
      constant_children() const {
    return constant_children_;
  }

  /// Directly copies children from another gate.
  ///
  /// @param[in] gate The gate which children will be copied.
  inline void CopyChildren(const IGatePtr& gate) {
    children_ = gate->children();
  }

  /// @returns The state of this gate.
  inline const State& state() const { return state_; }

  /// @returns true if this gate is set to be a module.
  /// @returns false if it is not yet set to be a module.
  inline bool IsModule() const { return module_; }

  /// This function is used to initiate this gate with children.
  /// It is assumed that children are passed in ascending order from another
  /// children set.
  ///
  /// @param[in] child A positive or negative index of a child.
  void InitiateWithChild(int child);

  /// Adds a child to this gate. Before adding the child, the existing
  /// children are checked for complements. If there is a complement,
  /// the gate changes its state and clears its children. This functionality
  /// only works with OR and AND gates.
  ///
  /// @param[in] child A positive or negative index of a child.
  ///
  /// @returns false if there is a complement of the child being added.
  /// @returns true if the addition of this child is successful.
  ///
  /// @warning This function does not indicate error for future additions in
  ///          case the state is nulled or becomes unity.
  bool AddChild(int child);

  /// Adds a child gate to this gate. Before adding the child, the existing
  /// children are checked for complements. If there is a complement,
  /// the gate changes its state and clears its children.
  ///
  /// @param[in] child A positive or negative index of a child.
  /// @param[in] gate A pointer to the child gate.
  ///
  /// @returns false if there final state of the parent is normal.
  /// @returns true if the parent has become constant due to a complement child.
  ///
  /// @warning This function does not indicate error for future additions in
  ///          case the state is nulled or becomes unity.
  bool AddChild(int child, const IGatePtr& gate);

  /// Adds a child basic event to this gate. Before adding the child, the
  /// existing children are checked for complements. If there is a complement,
  /// the gate changes its state and clears its children.
  ///
  /// @param[in] child A positive or negative index of a child.
  /// @param[in] basic_event A pointer to the child basic_event.
  ///
  /// @returns false if there final state of the parent is normal.
  /// @returns true if the parent has become constant due to a complement child.
  ///
  /// @warning This function does not indicate error for future additions in
  ///          case the state is nulled or becomes unity.
  bool AddChild(int child, const IBasicEventPtr& basic_event);

  /// Adds a constant child to this gate. Before adding the child, the existing
  /// children are checked for complements. If there is a complement,
  /// the gate changes its state and clears its children.
  ///
  /// @param[in] child A positive or negative index of a child.
  /// @param[in] constant A pointer to the child that is a Constant.
  ///
  /// @returns false if there final state of the parent is normal.
  /// @returns true if the parent has become constant due to a complement child.
  ///
  /// @warning This function does not indicate error for future additions in
  ///          case the state is nulled or becomes unity.
  bool AddChild(int child, const ConstantPtr& constant);

  /// Swaps an existing child to a new child. Mainly used for
  /// changing the logic of this gate or complementing the child.
  ///
  /// @param[in] existing_child An existing child to get swapped.
  /// @param[in] new_child A new child.
  ///
  /// @warning If there is an iterator for the children set, then
  ///          it may become unusable because the children set is manipulated.
  bool SwapChild(int existing_child, int new_child);

  /// Makes all children complement of themselves.
  /// This is a helper function to propagate a complement gate and apply
  /// De Morgan's Law.
  void InvertChildren();

  /// Replaces a child with the complement of it.
  /// This is a helper function to propagate a complement gate and apply
  /// De Morgan's Law.
  void InvertChild(int existing_child);

  /// Adds children of a child gate to this gate. This is a helper function for
  /// gate coalescing. The child gate of the same logic is removed from the
  /// children list. The sign of the child gate is expected to be positive.
  ///
  /// @param[in] child_gate The gate which children to be added to this gate.
  ///
  /// @returns false if the final set is null or unity.
  /// @returns true if the addition is successful with a normal final state.
  bool JoinGate(IGate* child_gate);

  /// Swaps a single child of a NULL type child gate. This is separate from
  /// other coalescing functions because this function takes into account the
  /// sign of the child.
  ///
  /// @param[in] index Positive or negative index of the child gate.
  ///
  /// @returns false if the final set is null or unity.
  /// @returns true if the addition is successful with a normal final state.
  bool JoinNullGate(int index);

  /// Clears all the children of this gate.
  inline void EraseAllChildren() { children_.clear(); }

  /// Removes a child from the children container. The passed child index
  /// must be in this gate's children container and initialized.
  ///
  /// @param[in] child The positive or negative index of the existing child.
  inline void EraseChild(int child) {
    assert(children_.count(child));
    children_.erase(child);
  }

  /// Sets the state of this gate to null and clears all its children.
  /// This function is expected to be used only once.
  inline void Nullify() {
    assert(state_ == kNormalState);
    state_ = kNullState;
    children_.clear();
  }

  /// Sets the state of this gate to unity and clears all its children.
  /// This function is expected to be used only once.
  inline void MakeUnity() {
    assert(state_ == kNormalState);
    state_ = kUnityState;
    children_.clear();
  }

  /// Turns this gate's module flag on. This should be one time operation.
  inline void TurnModule() {
    assert(!module_);
    module_ = true;
  }

 private:
  /// Process an addition of a complement of an existing child.
  ///
  /// @param[in] index Positive or negative index of the child.
  ///
  /// @returns false if the final set is null or unity.
  /// @returns true if the addition is successful with a normal final state.
  bool ProcessComplementChild(int index);

  GateType type_;  ///< Type of this gate.
  State state_;  ///< Indication if this gate's state is normal, null, or unity.
  int vote_number_;  ///< Vote number for ATLEAST gate.
  bool module_;  ///< Indication of an independent module gate.
  std::set<int> children_;  ///< Children of the gate.
  /// Children that are gates.
  boost::unordered_map<int, IGatePtr> gate_children_;
  /// Children that are basic events.
  boost::unordered_map<int, IBasicEventPtr> basic_event_children_;
  /// Children that are constant like house events.
  boost::unordered_map<int, ConstantPtr> constant_children_;
};

class BasicEvent;
class Gate;
class Formula;

/// @class IndexedFaultTree
/// This class provides simpler representation of a fault tree
/// that takes into account the indices of events instead of ids and pointers.
class IndexedFaultTree {
 public:
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<IGate> IGatePtr;

  /// Constructs an indexed fault tree starting from the top gate.
  ///
  /// @param[in] root The top gate of the fault tree.
  /// @param[in] ccf Incorporation of ccf gates and events for ccf groups.
  explicit IndexedFaultTree(const GatePtr& root, bool ccf = false);

  /// @returns The index of the top gate of this fault tree.
  inline int top_event_index() const { return top_event_index_; }

  /// Sets the index for the top gate.
  ///
  /// @param[in] index Positive index of the top gate.
  inline void top_event_index(int index) { top_event_index_ = index; }

  /// @returns The current top gate of the fault tree.
  inline const IGatePtr& top_event() const {
    assert(indexed_gates_.count(top_event_index_));
    return indexed_gates_.find(top_event_index_)->second;
  }

  /// @returns Indexed basic event as initialized in this fault tree.
  inline const std::vector<BasicEventPtr>& basic_events() const {
    return basic_events_;
  }

  /// Helper function to map the results of indexation to the original basic
  /// events. This function, for example, helps transform minimal cut sets with
  /// indices into minimal cut sets with IDs or pointers.
  ///
  /// @param[in] index Positive index of the basic event.
  ///
  /// @returns Pointer to the original basic event from its index.
  inline const BasicEventPtr& GetBasicEvent(int index) const {
    assert(index > 0);
    assert(index <= basic_events_.size());
    return basic_events_[index - 1];
  }

  /// Determines the type of the index.
  ///
  /// @param[in] index Positive index.
  ///
  /// @returns true if the given index belongs to an indexed gate.
  ///
  /// @warning The actual existance of the indexed gate is not guaranteed.
  inline bool IsGateIndex(int index) const {
    assert(index > 0);
    return index >= kGateIndex_;
  }

  /// Adds a new indexed gate into the indexed fault tree's gate container.
  ///
  /// @param[in] gate A new indexed gate.
  inline void AddGate(const IGatePtr& gate) {
    assert(!indexed_gates_.count(gate->index()));
    indexed_gates_.insert(std::make_pair(gate->index(), gate));
  }

  /// Commonly used function to get indexed gates from indices.
  ///
  /// @param[in] index Positive index of a gate.
  ///
  /// @returns The pointer to the requested indexed gate.
  inline const IGatePtr& GetGate(int index) const {
    assert(index > 0);
    assert(index >= kGateIndex_);
    assert(indexed_gates_.count(index));
    return indexed_gates_.find(index)->second;
  }

  /// Creates a new indexed gate. The gate is added to the fault tree container.
  /// The index for the new gates are assigned sequentially and guaranteed to
  /// be unique.
  ///
  /// @param[in] type The type of the new gate.
  ///
  /// @returns Pointer to the newly created gate.
  inline IGatePtr CreateGate(const GateType& type) {
    IGatePtr gate(new IGate(type));
    indexed_gates_.insert(std::make_pair(gate->index(), gate));
    return gate;
  }

 private:
  typedef boost::shared_ptr<Formula> FormulaPtr;
  typedef boost::shared_ptr<Node> NodePtr;
  typedef boost::shared_ptr<Constant> ConstantPtr;
  typedef boost::shared_ptr<IBasicEvent> IBasicEventPtr;

  /// Mapping to string gate types to enum gate types.
  static const std::map<std::string, GateType> kStringToType_;

  /// Process a Boolean formula of a gate into an indexed fault tree.
  ///
  /// @param[in] formula The Boolean formula to be processed.
  /// @param[in] ccf To replace basic events with ccf gates.
  /// @param[in,out] id_to_index The mapping of already processed nodes.
  ///
  /// @returns Pointer to the newly created indexed gate.
  IGatePtr ProcessFormula(
      const FormulaPtr& formula,
      bool ccf,
      boost::unordered_map<std::string, NodePtr>* id_to_index);

  int top_event_index_;  ///< The index of the top gate of this tree.
  const int kGateIndex_;  ///< The starting gate index for gate identification.
  /// All gates of this tree including newly created ones.
  boost::unordered_map<int, IGatePtr> indexed_gates_;
  std::vector<BasicEventPtr> basic_events_;  ///< Mapping for basic events.
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_FAULT_TREE_H_
