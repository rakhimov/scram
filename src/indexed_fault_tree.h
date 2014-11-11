/// @file indexed_fault_tree.h
/// A fault tree analysis facility with event and gate indices instead
/// of id names.
#ifndef SCRAM_SRC_INDEXED_FAULT_TREE_H_
#define SCRAM_SRC_INDEXED_FAULT_TREE_H_

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace scram {

/// @class SimpleGate
/// A helper class to be used in indexed fault tree. This gate represents
/// only positive OR or AND gates with basic event indices and pointers to
/// other simple gates.
class SimpleGate {
 public:
  typedef boost::shared_ptr<SimpleGate> SimpleGatePtr;

  /// @param[in] type The type of this gate. 1 is OR; 2 is AND.
  explicit SimpleGate(int type) {
    assert(type == 1 || type == 2);
    type_ = type;
  }

  /// @returns The type of this gate. Either 1 or 2 for OR or AND, respectively.
  inline int type() {
    return type_;
  }

  /// Adds a basic event index at the end of a container.
  /// This function is specificly given to initiate the gate.
  /// @param[in] index The index of a basic event.
  inline void InitiateWithBasic(int index) {
    basic_events_.insert(basic_events_.end(), index);
  }

  /// Adds a module index at the end of a container.
  /// This function is specificly given to initiate the gate.
  /// @param[in] index The index of a module.
  inline void InitiateWithModule(int index) {
    modules_.insert(modules_.end(), index);
  }

  /// Checks if there is a complement of a given basic event.
  /// If not, adds a basic event index into children.
  /// If the resulting set is null with AND gate, gives indication without
  /// actual addition.
  /// @returns true If the final gate is not null.
  /// @returns false If the final set would be null upon addition.
  inline bool AddBasic(int index) {
    if (type_ == 2 && basic_events_.count(-index)) return false;
    basic_events_.insert(index);
    return true;
  }

  /// Adds a module event index into children.
  /// All modules assumed to be positive.
  inline void AddModule(int index) {
    assert(index > 0);
    modules_.insert(index);
  }

  /// Add a pointer to a child gate.
  /// This function assumes that the tree does not have complement gates.
  /// @param[in] gate The pointer to the child gate.
  inline void AddChildGate(const SimpleGatePtr& gate) {
    gates_.insert(gate);
  }

  /// Merges two gate of the same kind. This is designed for two AND gates.
  /// Gates containers asserted to contain only positive indices.
  /// @returns true If the final gate is not null.
  /// @returns false If the final set would be null upon addition.
  inline bool MergeGate(const SimpleGatePtr& gate) {
    assert(type_ == 2 && gate->type() == 2);
    std::set<int>::const_iterator it;
    for (it = gate->basic_events_.begin(); it != gate->basic_events_.end();
         ++it) {
      if (basic_events_.count(-*it)) return false;
    }
    basic_events_.insert(gate->basic_events_.begin(),
                         gate->basic_events_.end());
    modules_.insert(gate->modules_.begin(), gate->modules_.end());
    gates_.insert(gate->gates_.begin(), gate->gates_.end());
    return true;
  }

  /// If this gate emulates minimal cut sets, and there is a guarantee of
  /// mutual exclusivity of joining of another minimal cut set, then this
  /// function does the joining operation more efficiently.
  inline void JoinAsMcs(const SimpleGatePtr& gate) {
    assert(gate->type() == 2);
    assert(type_ == 2);
    assert(gate->gates_.empty());
    assert(gates_.empty());
    /// @todo Optimize with lazy ordering and constant time insertion.
    basic_events_.insert(gate->basic_events_.begin(),
                         gate->basic_events_.end());
    modules_.insert(gate->modules_.begin(), gate->modules_.end());
  }

  /// @returns The basic events of this gate.
  inline const std::set<int>& basic_events() const { return basic_events_; }

  /// Assigns a container of basic events for this gate.
  /// @param[in] basic_events The basic events for this gate.
  inline void basic_events(const std::set<int>& basic_events) {
    basic_events_ = basic_events;
  }

  /// @returns The modules for this gate.
  inline const std::set<int>&  modules() { return modules_; }

  /// Assigns a container of modules for this gate.
  /// @param[in] modules The modules for this gate.
  inline void modules(const std::set<int>& modules) {
    modules_ = modules;
  }

  /// @returns The child gates container of this gate.
  inline std::set<SimpleGatePtr>& gates() { return gates_; }

 private:
  int type_;  ///< Type of this gate.
  std::set<int> basic_events_;  ///< Container of basic events' indices.
  std::set<int> modules_;  ///< Container for modules.
  std::set<SimpleGatePtr> gates_;  ///< Containter of child gates.
};

typedef boost::shared_ptr<SimpleGate> SimpleGatePtr;
/// @class SetPtrComp
/// Functor for cut set pointer comparison.
struct SetPtrComp : public std::binary_function<const SimpleGatePtr,
                                                const SimpleGatePtr, bool> {
  /// Operator overload.
  /// Compares cut sets for sorting.
  bool operator()(const SimpleGatePtr& lhs, const SimpleGatePtr& rhs) const {
    if (lhs->basic_events() < rhs->basic_events()) {
      return true;
    } else if (lhs->basic_events() > rhs->basic_events()) {
      return false;
    } else if (lhs->modules() < rhs->modules()) {
      return true;
    }
    return false;
  }
};

class Gate;
class IndexedGate;

/// @class IndexedFaultTree
/// This class should provide simpler representation of a fault tree
/// that takes into account the indices of events instead of ids and pointers.
class IndexedFaultTree {
  // This class should:
  // 1. Use indices of events.
  // 2. Propagate fault tree constants (House events).
  // 3. Unroll complex gates like XOR and VOTE.
  // 4. Pre-process the tree to simplify the structure.
  // 5. Take into account non-coherent logic for the fault tree.
  // Ideally, the resulting tree should have only OR and AND gates with
  // basic event and gate indices only.
  // The indices of gates may change, but the indices of basic events must
  // not change.
 public:
  typedef boost::shared_ptr<Gate> GatePtr;

  /// Constructs a simplified fault tree.
  /// @param[in] top_event_id The index of the top event of this tree.
  IndexedFaultTree(int top_event_id, int limit_order);

  /// Creates indexed gates with basic and house event indices as children.
  /// This function also simplifies the tree to simple gates.
  /// @param[in] int_to_inter Container of gates and thier indices including
  ///                         the top gate.
  /// @param[in] all_to_int Container of all events in this tree to index
  ///                       children of the gates.
  void InitiateIndexedFaultTree(
      const boost::unordered_map<int, GatePtr>& int_to_inter,
      const boost::unordered_map<std::string, int>& all_to_int);

  /// Remove all house events by propagating them as constants in Boolean
  /// equation.
  /// @param[in] true_house_events House events with true state.
  /// @param[in] false_house_events House events with false state.
  void PropagateConstants(const std::set<int>& true_house_events,
                          const std::set<int>& false_house_events);

  /// Performs processing of a fault tree.
  /// @param[in] num_basic_events The number of basic events. This information
  ///                             is needed to optimize the tree traversel
  ///                             with certain expectation.
  void ProcessIndexedFaultTree(int num_basic_events);

  /// Finds minimal cut sets.
  void FindMcs();

  /// @returns Generated minimal cut sets with basic event indices.
  inline const std::vector< std::set<int> >& GetGeneratedMcs() {
    return imcs_;
  }

 private:
  typedef boost::shared_ptr<SimpleGate> SimpleGatePtr;
  typedef boost::shared_ptr<IndexedGate> IndexedGatePtr;

  /// Starts unrolling gates to simplify gates to OR, AND gates.
  /// NOT and NUll are dealt with specificly.
  /// This function uses parent information of each gate, so the tree must
  /// be initialized before a call of this function.
  /// New gates are created upon unrolling complex gates.
  void UnrollGates();

  /// Traverses the tree to gather information about parents of indexed gates.
  /// @param[in] parent_gate The parent to start information gathering.
  /// @param[out] processed_gates The gates that has already been processed.
  void GatherParentInformation(const IndexedGatePtr& parent_gate,
                               std::set<int>* processed_gates);

  /// Notifies all parents of negative gates, such as NOR and NAND before
  /// transforming this gates into basic gates of OR and AND.
  /// The parent information should be available. This function does not
  /// change the type of the given gate.
  /// @param[in] gate The gate to be start processing.
  void NotifyParentsOfNegativeGates(const IndexedGatePtr& gate);

  /// Unrolls a gate to make OR, AND gates. The parents of the
  /// gate are not notified. This means that negative gates must be dealt
  /// separately. However, NOT and NULL gates are left untouched for later
  /// special processing.
  /// @param[out] gate The gate to be processed.
  void UnrollGate(IndexedGatePtr& gate);

  /// Unrolls a gate with XOR logic.
  /// @param[out] gate The gate to unroll.
  void UnrollXorGate(IndexedGatePtr& gate);

  /// Unrolls a gate with "atleast" gate and vote number.
  /// @param[out] gate The atleast gate to unroll.
  void UnrollAtleastGate(IndexedGatePtr& gate);

  /// Remove all house events from a given gate.
  /// After this function, there should not be any unity or null gates because
  /// of house events.
  /// @param[in] true_house_events House events with true state.
  /// @param[in] false_house_events House events with false state.
  /// @param[out] gate The final resultant processed gate.
  /// @param[out] processed_gates The gates that has already been processed.
  void PropagateConstants(const std::set<int>& true_house_events,
                          const std::set<int>& false_house_events,
                          IndexedGatePtr& gate,
                          std::set<int>* processed_gates);

  /// Propagates complements of child gates down to basic events
  /// in order to remove any NOR or NAND logic from the tree.
  /// This function also processes NOT and NULL gates.
  /// @param[out] gate The starting gate to traverse the tree. This is for
  ///                 recursive purposes. The sign of this passed gate
  ///                 is unknown for the function, so it must be sanitized
  ///                 for a top event to function correctly.
  /// @param[out] gate_complements The complements of gates already processed.
  /// @param[out] processed_gates The gates that has already been processed.
  void PropagateComplements(IndexedGatePtr& gate,
                            std::map<int, int>* gate_complements,
                            std::set<int>* processed_gates);

  /// Processes null and unity gates.
  /// There should not be negative gate children.
  /// After this function, there should not be null or unity gates resulting
  /// from previous processing steps.
  /// @param[out] gate The starting gate to traverse the tree. This is for
  ///                  recursive purposes.
  /// @param[out] processed_gates The gates that has already been processed.
  void ProcessConstGates(IndexedGatePtr& gate, std::set<int>* processed_gates);

  /// Pre-processes the tree by doing Boolean algebra.
  /// At this point all gates are expected to be either OR or AND.
  /// There should not be negative gate children.
  /// This function merges similar gates and may produce null or unity gates.
  /// @param[out] gate The starting gate to traverse the tree. This is for
  ///                 recursive purposes. This gate must be AND or OR.
  /// @param[out] processed_gates The gates that has already been processed.
  void JoinGates(IndexedGatePtr& gate, std::set<int>* processed_gates);

  /// Traverses the indexed fault tree to detect modules.
  /// @param[in] num_basic_events The number of basic events in the tree.
  void DetectModules(int num_basic_events);

  /// Traverses the given gate and assigns time of visit to nodes.
  /// @param[in] time The current time.
  /// @param[out] gate The gate to traverse and assign time to.
  /// @param[out] visit_basics The recordings for basic events.
  /// @returns The time final time of traversing.
  int AssignTiming(int time, IndexedGatePtr& gate, int visit_basics[][2]);

  /// Determines modules from original gates that have been already timed.
  /// @param[in] gate The gate to test for modularity.
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[out] visited_gates Container of already visited gates.
  /// @param[out] min_time The min time of visit for gate and its children.
  /// @param[out] max_time The max time of visit for gate and its children.
  void FindOriginalModules(IndexedGatePtr& gate,
                           const int visit_basics[][2],
                           std::map<int, std::pair<int, int> >* visited_gates,
                           int* min_time, int* max_time);

  /// Creates new modules in a fault tree. The information about original
  /// existing modules should be available.
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[in] gate The gate to test its children for modularity.
  /// @param[out] visited_gates Container of already visited gates.
  void CreateNewModules(const int visit_basics[][2],
                        IndexedGatePtr& gate,
                        std::set<int>* visited_gates);

  /// Finds minimal cut sets of a module.
  /// Module gate can only be AND or OR.
  /// @param[in] index The positive or negative index of a module.
  /// @param[out] min_gates Simple gates containing minimal cut sets.
  void FindMcsFromModule(int index, std::vector<SimpleGatePtr>* min_gates);

  /// Traverses the fault tree to convert gates into simple gates.
  /// @param[in] gate_index The index of a gate to start with.
  /// @param[out] processed_gates Currently processed gates.
  /// @returns The top simple gate.
  SimpleGatePtr CreateSimpleTree(int gate_index,
                                 std::map<int, SimpleGatePtr>* processed_gates);

  /// Finds minimal cut sets of a simple gate.
  /// @param[out] gate The simple gate as a parent for processing.
  /// @param[out] min_gates Simple gates containing minimal cut sets.
  void FindMcsFromSimpleGate(SimpleGatePtr& gate,
                             std::vector<SimpleGatePtr>* min_gates);

  /// Expands OR layer in preprocessed fault tree.
  /// @param[out] gate The OR gate to be processed.
  /// @param[out] cut_sts Cut sets found while traversing the tree.
  void ExpandOrLayer(SimpleGatePtr& gate, std::vector<SimpleGatePtr>* cut_sets);

  /// Expands AND layer in preprocessed fault tree.
  /// @param[out] gate The AND gate to be processed into OR gate.
  void ExpandAndLayer(SimpleGatePtr& gate);

  /// Finds minimal cut sets from cut sets.
  /// Applys rule 4 to reduce unique cut sets to minimal cut sets.
  /// @param[in] cut_sets Cut sets with primary events.
  /// @param[in] mcs_lower_order Reference minimal cut sets of some order.
  /// @param[in] min_order The order of sets to become minimal.
  /// @param[out] min_gates Min cut sets in simple gates.
  /// @note T_avg(N^3 + N^2*logN + N*logN) = O_avg(N^3)
  void MinimizeCutSets(const std::vector<SimpleGatePtr>& cut_sets,
                       const std::vector<SimpleGatePtr>& mcs_lower_order,
                       int min_order,
                       std::vector<SimpleGatePtr>* min_gates);

  int top_event_index_;  ///< The index of the top gate of this tree.
  int gate_index_;  ///< The starting gate index for gate identification.
  /// All gates of this tree including newly created ones.
  boost::unordered_map<int, IndexedGatePtr> indexed_gates_;
  std::set<int> modules_;  ///< Modules in the tree.
  int top_event_sign_;  ///< The negative or positive sign of the top event.
  int new_gate_index_;  ///< Index for a new gate.
  std::vector< std::set<int> > imcs_;  // Min cut sets with indexed events.
  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;
  /// Indicator if the tree has been changed due to operations on it.
  bool changed_tree_;
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_FAULT_TREE_H_
