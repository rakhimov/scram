/// @file indexed_fault_tree.h
/// A fault tree analysis facility with event and gate indices instead
/// of id names.
#ifndef SCRAM_SRC_INDEXED_FAULT_TREE_H_
#define SCRAM_SRC_INDEXED_FAULT_TREE_H_

#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace scram {

typedef boost::shared_ptr<std::set<int> > SetPtr;

/// @class SetPtrComp
/// Functor for set pointer comparison efficiency.
struct SetPtrComp
    : public std::binary_function<const SetPtr, const SetPtr, bool> {
  /// Operator overload.
  /// Compares sets for sorting.
  bool operator()(const SetPtr& lhs, const SetPtr& rhs) const {
    return *lhs < *rhs;
  }
};

/// @class SimpleGate
/// A helper class to be used in indexed fault tree. This gate represents
/// only positive OR or AND gates with basic event indices and pointers to
/// other simple gates.
/// All the gate children of this gate must of opposite type.
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
    assert(type_ == 1 || type_ == 2);
    return type_;
  }

  /// Adds a basic event index at the end of a container.
  /// This function is specificly given to initiate the gate.
  /// @param[in] index The index of a basic event.
  inline void InitiateWithBasic(int index) {
    basic_events_.push_back(index);
  }

  /// Adds a module index at the end of a container.
  /// This function is specificly given to initiate the gate.
  /// Note that modules are treated just like basic events.
  /// @param[in] index The index of a module.
  inline void InitiateWithModule(int index) {
    assert(index > 0);
    modules_.push_back(index);
  }

  /// Add a pointer to a child gate.
  /// This function assumes that the tree does not have complement gates.
  /// @param[in] gate The pointer to the child gate.
  inline void AddChildGate(const SimpleGatePtr& gate) {
    assert(gate->type() != type_);
    gates_.push_back(gate);
  }

  /// Generates cut sets by using a provided set.
  /// @param[in] cut_set The base cut set to work with.
  /// @param[out] new_cut_sets Generated cut sets by adding the gate's children.
  void GenerateCutSets(const SetPtr& cut_set,
                       std::set<SetPtr, SetPtrComp>* new_cut_sets);

  /// Sets the limit order for all analysis with simple gates.
  /// @param[in] limit The limit order for minimal cut sets.
  static void limit_order(int limit) { limit_order_ = limit; }

 private:
  /// Generates cut sets for AND gate children using already generated sets.
  /// The tree is assumed to be layered with OR children of AND gates.
  /// @param[in] cut_set The base cut set to work with.
  /// @param[out] new_cut_sets Generated cut sets by using the gate's children.
  void AndGateCutSets(const SetPtr& cut_set,
                      std::set<SetPtr, SetPtrComp>* new_cut_sets);

  /// Generates cut sets for OR gate children using already generated sets.
  /// The tree is assumed to be layered with AND children of OR gates.
  /// @param[in] cut_set The base cut set to work with.
  /// @param[out] new_cut_sets Generated cut sets by using the gate's children.
  void OrGateCutSets(const SetPtr& cut_set,
                      std::set<SetPtr, SetPtrComp>* new_cut_sets);

  int type_;  ///< Type of this gate.
  std::vector<int> basic_events_;  ///< Container of basic events' indices.
  std::vector<int> modules_;  ///< Container of modules' indices.
  std::vector<SimpleGatePtr> gates_;  ///< Containter of child gates.
  static int limit_order_;  ///< The limit on the order of minimal cut sets.
};

class Gate;
class IndexedGate;

/// @class IndexedFaultTree
/// This class provides simpler representation of a fault tree
/// that takes into account the indices of events instead of ids and pointers.
/// The class provides main operations over a fault tree to generate minmal
/// cut sets.
class IndexedFaultTree {
 public:
  typedef boost::shared_ptr<Gate> GatePtr;

  /// Constructs a simplified fault tree with indices of nodes.
  /// @param[in] top_event_id The index of the top event of this tree.
  /// @param[in] limit_order The limit on the size of minimal cut sets.
  IndexedFaultTree(int top_event_id, int limit_order);

  /// Creates indexed gates with basic and house event indices as children.
  /// This function also simplifies the tree to simple gates.
  /// @param[in] int_to_inter Container of gates and thier indices including
  ///                         the top gate.
  /// @param[in] ccf_basic_to_gates CCF basic events that are converted to
  ///                               gates.
  /// @param[in] all_to_int Container of all events in this tree to index
  ///                       children of the gates.
  void InitiateIndexedFaultTree(
      const boost::unordered_map<int, GatePtr>& int_to_inter,
      const std::map<std::string, int>& ccf_basic_to_gates,
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

  /// Finds minimal cut sets from the initiated fault tree with indices.
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
  /// New gates are created upon unrolling complex gates, such as XOR.
  void UnrollGates();

  /// Traverses the tree to gather information about parents of indexed gates.
  /// This information might be needed for other algorithms because
  /// due to processing of the tree, the shape and nodes may change.
  /// @param[in] parent_gate The parent to start information gathering.
  /// @param[in,out] processed_gates The gates that has already been processed.
  void GatherParentInformation(const IndexedGatePtr& parent_gate,
                               std::set<int>* processed_gates);

  /// Notifies all parents of negative gates, such as NOR and NAND before
  /// transforming these gates into basic gates of OR and AND.
  /// The parent information should be available. This function does not
  /// change the type of the given gate.
  /// @param[in] gate The gate to be start processing.
  void NotifyParentsOfNegativeGates(const IndexedGatePtr& gate);

  /// Unrolls a gate to make OR, AND gates. The parents of the
  /// gate are not notified. This means that negative gates must be dealt
  /// separately. However, NOT and NULL gates are left untouched for later
  /// special processing.
  /// @param[in,out] gate The gate to be processed.
  void UnrollGate(const IndexedGatePtr& gate);

  /// Unrolls a gate with XOR logic.
  /// @param[in,out] gate The gate to unroll.
  void UnrollXorGate(const IndexedGatePtr& gate);

  /// Unrolls an ATLEAST gate with a vote number.
  /// @param[in,out] gate The atleast gate to unroll.
  void UnrollAtleastGate(const IndexedGatePtr& gate);

  /// Remove all house events from a given gate.
  /// After this function, there should not be any unity or null gates because
  ///  of house events.
  /// @param[in] true_house_events House events with true state.
  /// @param[in] false_house_events House events with false state.
  /// @param[in,out] gate The final resultant processed gate.
  /// @param[in,out] processed_gates The gates that has already been processed.
  void PropagateConstants(const std::set<int>& true_house_events,
                          const std::set<int>& false_house_events,
                          const IndexedGatePtr& gate,
                          std::set<int>* processed_gates);

  /// Removes null and unity gates. There should not be negative gate children.
  /// After this function, there should not be null or unity gates resulting
  /// from previous processing steps.
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  /// @param[in,out] processed_gates The gates that has already been processed.
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  bool ProcessConstGates(const IndexedGatePtr& gate,
                         std::set<int>* processed_gates);

  /// Changes the state of a gate or passes a constant child to be removed
  /// later. The function determines its actions depending on the type of
  /// a gate and state of a child,
  /// @param[in,out] gate The parent gate that contains the children.
  /// @param[in] child The constant child under consideration.
  /// @param[in] state False or True constant state of the child.
  /// @param[out] to_erase The set of children to erase from the above gate.
  /// @returns true if the passed gate has become a constant due to its child.
  /// @returns false if the parent still valid for kurther operations.
  bool ProcessConstantChild(const IndexedGatePtr& gate,
                            int child,
                            bool state,
                            std::vector<int>* to_erase);

  /// Propagates complements of child gates down to basic events
  /// in order to remove any NOR or NAND logic from the tree.
  /// This function also processes NOT and NULL gates.
  /// The resulting tree will contain only positive gates, OR and AND.
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes. The sign of this passed gate
  ///                     is unknown for the function, so it must be sanitized
  ///                     for a top event to function correctly.
  /// @param[in,out] gate_complements The complements of gates already processed.
  /// @param[in,out] processed_gates The gates that has already been processed.
  void PropagateComplements(const IndexedGatePtr& gate,
                            std::map<int, int>* gate_complements,
                            std::set<int>* processed_gates);

  /// Removes a set of children from an OR/AND gate.
  /// This is a helper function for NULL and UNITY propagation on the tree.
  /// If the final gate is empty, it is turned into NULL for OR gates and
  /// UNITY for AND and other gates.
  /// This function may also update the children parent information if needed.
  /// @param[in,out] gate The gate that contains the children to be removed.
  /// @param[in] to_erase The set of children to erase from the above gate.
  void RemoveChildren(const IndexedGatePtr& gate,
                      const std::vector<int>& to_erase);

  /// Pre-processes the tree by doing simple Boolean algebra.
  /// At this point all gates are expected to be either OR or AND.
  /// There should not be negative gate children.
  /// This function merges similar gates and may produce null or unity gates.
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes. This gate must be AND or OR.
  /// @param[in,out] processed_gates The gates that has already been processed.
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  bool JoinGates(const IndexedGatePtr& gate, std::set<int>* processed_gates);

  /// Traverses the indexed fault tree to detect modules.
  /// @param[in] num_basic_events The number of basic events in the tree.
  void DetectModules(int num_basic_events);

  /// Traverses the given gate and assigns time of visit to nodes.
  /// @param[in] time The current time.
  /// @param[in,out] gate The gate to traverse and assign time to.
  /// @param[in,out] visit_basics The recordings for basic events.
  /// @returns The time final time of traversing.
  int AssignTiming(int time, const IndexedGatePtr& gate, int visit_basics[][2]);

  /// Determines modules from original gates that have been already timed.
  /// This function can also create new modules from the existing tree.
  /// @param[in,out] gate The gate to test for modularity.
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[in,out] visited_gates Container of visited gates with
  ///                              min and max time of visits of the subtree.
  void FindOriginalModules(const IndexedGatePtr& gate,
                           const int visit_basics[][2],
                           std::map<int, std::pair<int, int> >* visited_gates);

  /// Creates a new module as a child of an existing gate. The existing
  /// children of the original gate are used to create the new module.
  /// The module is added in the module and gate databases.
  /// If the new module must contain all the children, the original gate is
  /// turned into a module.
  /// @param[in,out] gate The parent gate for a module.
  /// @param[in] children Modular children to be added into the new module.
  void CreateNewModule(const IndexedGatePtr& gate,
                       const std::vector<int>& children);

  /// Checks if a group of modular children share anything with non-modular
  /// children. If so, then the modular children are not actually modular, and
  /// that children are removed from modular containers.
  /// This is due to chain of events that are shared between modular and
  /// non-modular children.
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[in] visited_gates Vist max and min time recordings for gates.
  /// @param[in,out] modular_children Candidates for modular grouping.
  /// @param[in,out] non_modular_children Non modular children.
  void FilterModularChildren(
      const int visit_basics[][2],
      const std::map<int, std::pair<int, int> >& visited_gates,
      std::vector<int>* modular_children,
      std::vector<int>* non_modular_children);


  /// Traverses the fault tree to convert gates into simple gates.
  /// @param[in] gate_index The index of a gate to start with.
  /// @param[in,out] processed_gates Gates turned into simple gates.
  void CreateSimpleTree(int gate_index,
                        std::map<int, SimpleGatePtr>* processed_gates);

  /// Finds minimal cut sets of a simple gate.
  /// @param[in] gate The simple gate as a parent for processing.
  /// @param[out] mcs Minimal cut sets.
  void FindMcsFromSimpleGate(const SimpleGatePtr& gate,
                             std::vector< std::set<int> >* mcs);

  /// Finds minimal cut sets from cut sets.
  /// Reduces unique cut sets to minimal cut sets.
  /// The performance is highly dependent on the passed sets. If the sets
  /// share many events, it takes more time to remove supersets.
  /// @param[in] cut_sets Cut sets with primary events.
  /// @param[in] mcs_lower_order Reference minimal cut sets of some order.
  /// @param[in] min_order The order of sets to become minimal.
  /// @param[out] mcs Min cut sets.
  /// @note T_avg(N^3 + N^2*logN + N*logN) = O_avg(N^3)
  void MinimizeCutSets(
      const std::vector<const std::set<int>* >& cut_sets,
      const std::vector<std::set<int> >& mcs_lower_order,
      int min_order,
      std::vector<std::set<int> >* mcs);

  int top_event_index_;  ///< The index of the top gate of this tree.
  int gate_index_;  ///< The starting gate index for gate identification.
  /// All gates of this tree including newly created ones.
  boost::unordered_map<int, IndexedGatePtr> indexed_gates_;
  std::set<int> modules_;  ///< Modules in the tree.
  int top_event_sign_;  ///< The negative or positive sign of the top event.
  int new_gate_index_;  ///< Index for a new gate.
  std::vector< std::set<int> > imcs_;  ///< Min cut sets with indexed events.
  /// Limit on the size of the minimal cut sets for performance reasons.
  int limit_order_;
};

}  // namespace scram

#endif  // SCRAM_SRC_INDEXED_FAULT_TREE_H_
