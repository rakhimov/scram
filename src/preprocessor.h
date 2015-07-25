/// @file preprocessor.h
/// A collection of fault tree preprocessing algorithms that simplify
/// fault trees for analysis.
#ifndef SCRAM_SRC_PREPROCESSOR_H_
#define SCRAM_SRC_PREPROCESSOR_H_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "indexed_fault_tree.h"

class PreprocessorTest;

namespace scram {

/// @class Preprocessor
/// The class provides main preprocessing operations over a fault tree
/// to generate minimal cut sets more efficiently.
class Preprocessor {
  friend class ::PreprocessorTest;

 public:
  typedef boost::shared_ptr<Gate> GatePtr;

  /// Constructs a preprocessor of an indexed fault tree.
  ///
  /// @param[in] fault_tree The fault tree to be preprocessed.
  explicit Preprocessor(IndexedFaultTree* fault_tree);

  /// Performs processing of a fault tree to simplify the structure to
  /// normalized (OR/AND gates only), modular, positive-gate-only indexed fault
  /// tree.
  void ProcessIndexedFaultTree();

 private:
  typedef boost::shared_ptr<IGate> IGatePtr;
  typedef boost::shared_ptr<IBasicEvent> IBasicEventPtr;
  typedef boost::shared_ptr<Constant> ConstantPtr;

  /// Starts normalizing gates to simplify gates to OR, AND gates.
  /// This function uses parent information of each gate, so the tree must
  /// be initialized before a call of this function.
  /// New gates are created upon normalizing complex gates, such as XOR.
  ///
  /// @warning NUll gates are not handled except for the top gate.
  void NormalizeGates();

  /// Notifies all parents of negative gates, such as NOT, NOR, and NAND before
  /// transforming these gates into basic gates of OR and AND. The child gates
  /// are swaped with a negative sign.
  ///
  /// @param[in] gate The gate to start processing.
  ///
  /// @warning This function does not change the types of gates.
  /// @warning The top gate does not have parents, so it is not handled here.
  void NotifyParentsOfNegativeGates(const IGatePtr& gate);

  /// Normalizes complex gates into OR, AND gates.
  ///
  /// @param[in,out] gate The gate to be processed.
  ///
  /// @warning The parents of negative gates are assumed to be notified about
  ///          the change of their children types.
  /// @warning NULL gates are not handled.
  void NormalizeGate(const IGatePtr& gate);

  /// Normalizes a gate with XOR logic. This is a helper function for the main
  /// gate normalization function.
  ///
  /// @param[in,out] gate The gate to normalize.
  void NormalizeXorGate(const IGatePtr& gate);

  /// Normalizes an ATLEAST gate with a vote number. The gate is turned into
  /// an OR gate of recursively normalized ATLEAST and AND child gates according
  /// to the formula K/N(x, y_i) = OR(AND(x, K-1/N-1(y_i)), K/N-1(y_i))) with
  /// y_i being the rest of formula variables, which exclude x.
  /// This representation is more friendly to other preprocessing and analysis
  /// techniques than the alternative, which is OR of AND gates of combinations.
  ///
  /// @param[in,out] gate The atleast gate to normalize.
  void NormalizeAtleastGate(const IGatePtr& gate);

  /// Remove all house events from a given gate according to the Boolean logic.
  /// The structure of the tree should not be pre-processed before this
  /// operation; that is, this is the first operation that is done after
  /// creation of an indexed fault tree. There should not be any negative nodes.
  /// After this function, there should not be any unity or null gates because
  /// of house events.
  ///
  /// @param[in,out] gate The final resultant processed gate.
  void PropagateConstants(const IGatePtr& gate);

  /// Changes the state of a gate or passes a constant child to be removed
  /// later. The function determines its actions depending on the type of
  /// a gate and state of a child; however, the sign of the index is ignored.
  /// The caller of this function must ensure that the state corresponds to the
  /// sign of the child index.
  /// The type of the gate may change, but it will only be valid after the
  /// to-be-erased children are handled properly.
  ///
  /// @param[in,out] gate The parent gate that contains the children.
  /// @param[in] child The constant child under consideration.
  /// @param[in] state False or True constant state of the child.
  /// @param[in,out] to_erase The set of children to erase from the parent gate.
  ///
  /// @returns true if the passed gate has become constant due to its child.
  /// @returns false if the parent still valid for further operations.
  bool ProcessConstantChild(const IGatePtr& gate, int child,
                            bool state, std::vector<int>* to_erase);

  /// Removes a set of children from a gate taking into account the logic.
  /// This is a helper function for NULL and UNITY propagation on the tree.
  /// If the final gate is empty, its state is turned into NULL or UNITY
  /// depending on the logic of the gate and the logic of the constant
  /// propagation.
  /// The parent information is not updated for the child.
  ///
  /// @param[in,out] gate The gate that contains the children to be removed.
  /// @param[in] to_erase The set of children to erase from the parent gate.
  void RemoveChildren(const IGatePtr& gate, const std::vector<int>& to_erase);

  /// Removes NULL and UNITY state child gates. The parent gate can be of any
  /// type. Because of the constant children, the parent gate itself may turn
  /// constant in some cases.
  ///
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  ///
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  ///
  /// @warning There should not be negative gate children.
  /// @warning There still may be only one constant state gate which is the root
  ///          of the tree. This must be handled separately.
  bool RemoveConstGates(const IGatePtr& gate);

  /// Propagates complements of child gates down to basic events
  /// in order to remove any negative logic from the fault tree's gates.
  /// The tree must contain only OR and AND type gates. All NULL type gates must
  /// be removed from the tree. Other complex gates must be preprocessed.
  /// The resulting tree will contain only positive gates, OR and AND.
  ///
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes. The sign of this passed gate
  ///                     is unknown for the function, so it must be sanitized
  ///                     for a top event to function correctly.
  /// @param[in,out] gate_complements The processed complements of gates.
  void PropagateComplements(const IGatePtr& gate,
                            std::map<int, IGatePtr>* gate_complements);

  /// Removes child gates of NULL type, which means these child gates have
  /// only one child. That one grandchild is transfered to the parent gate,
  /// and the original child gate is removed from the parent gate.
  ///
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  ///
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  ///
  /// @warning There still may be only one NULL type gate which is the root
  ///          of the tree. This must be handled separately.
  bool RemoveNullGates(const IGatePtr& gate);

  /// Pre-processes the fault tree by doing the simplest Boolean algebra.
  /// Positive children with the same OR or AND gates as parents are coalesced.
  /// This function merges similar logic gates of NAND and NOR as well.
  ///
  /// @param[in,out] gate The starting gate to traverse the tree. This is for
  ///                     recursive purposes.
  ///
  /// @returns true if the given tree has been changed by this function.
  /// @returns false if no change has been made.
  ///
  /// @warning NULL or UNITY state gates may emerge because of this processing.
  /// @warning NULL type gates are not handled by this function.
  /// @warning Module child gates are omitted from coalescing.
  bool JoinGates(const IGatePtr& gate);

  /// Traverses the indexed fault tree to detect modules. Modules are
  /// independent sub-trees without common nodes with the rest of the tree.
  ///
  /// @param[in] num_basic_events The number of basic events in the tree.
  void DetectModules(int num_basic_events);

  /// Traverses the given gate and assigns time of visit to nodes.
  ///
  /// @param[in] time The current time.
  /// @param[in,out] gate The gate to traverse and assign time to.
  /// @param[in,out] visit_basics The recordings for basic events.
  ///
  /// @returns The final time of traversing.
  int AssignTiming(int time, const IGatePtr& gate, int visit_basics[][2]);

  /// Determines modules from original gates that have been already timed.
  /// This function can also create new modules from the existing tree.
  ///
  /// @param[in,out] gate The gate to test for modularity.
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[in,out] visited_gates Container of visited gates with
  ///                              min and max time of visits of the subtree.
  void FindModules(const IGatePtr& gate, const int visit_basics[][2],
                   std::map<int, std::pair<int, int> >* visited_gates);

  /// Creates a new module as a child of an existing gate if the logic of the
  /// existing parent gate allows a sub-module. The existing
  /// children of the original gate are used to create the new module.
  /// If the new module must contain all the children, the original gate is
  /// asserted to be a module, and no operation is performed.
  ///
  /// @param[in,out] gate The parent gate for a module.
  /// @param[in] children Modular children to be added into the new module.
  ///
  /// @returns Pointer to the new module if it is created.
  IGatePtr CreateNewModule(const IGatePtr& gate,
                           const std::vector<int>& children);

  /// Checks if a group of modular children share anything with non-modular
  /// children. If so, then the modular children are not actually modular, and
  /// that children are removed from modular containers.
  /// This is due to chain of events that are shared between modular and
  /// non-modular children.
  ///
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[in] visited_gates Visit max and min time recordings for gates.
  /// @param[in,out] modular_children Candidates for modular grouping.
  /// @param[in,out] non_modular_children Non modular children.
  void FilterModularChildren(
      const int visit_basics[][2],
      const std::map<int, std::pair<int, int> >& visited_gates,
      std::vector<int>* modular_children,
      std::vector<int>* non_modular_children);

  /// Groups modular children by their common elements. The gates created with
  /// these modular children are guaranteed to be independent modules.
  ///
  /// @param[in] visit_basics The recordings for basic events.
  /// @param[in] visited_gates Visit max and min time recordings for gates.
  /// @param[in] modular_children Candidates for modular grouping.
  /// @param[out] groups Grouped modular children.
  void GroupModularChildren(
      const int visit_basics[][2],
      const std::map<int, std::pair<int, int> >& visited_gates,
      const std::vector<int>& modular_children,
      std::vector<std::vector<int> >* groups);

  /// Creates new module gates from groups of modular children if the logic of
  /// the parent gate allows sub-modules. The existing
  /// children of the original gate are used to create the new modules.
  /// If all the parent gate children are modular and within one group,
  /// the parent gate is asserted to be a module gate, and no operation is
  /// performed.
  ///
  /// @param[in,out] gate The parent gate for a module.
  /// @param[in] modular_children All the modular children.
  /// @param[in] groups Grouped modular children.
  void CreateNewModules(const IGatePtr& gate,
                        const std::vector<int>& modular_children,
                        const std::vector<std::vector<int> >& groups);

  /// Clears visit time information from all indexed gates that have been
  /// visited top-down. Any member function updating and using the visit
  /// information of gates must ensure to clean visit times before running
  /// algorithms. However, cleaning after finishing algorithms is not mandatory.
  void ClearGateVisits();

  /// Clears visit information from descendant gates starting from the given
  /// gate as a root.
  ///
  /// @param[in/out] gate The root gate to be traversed and cleaned.
  void ClearGateVisits(const IGatePtr& gate);

  IndexedFaultTree* fault_tree_;  ///< The fault tree to preprocess.
  int top_event_sign_;  ///< The negative or positive sign of the top event.
};

}  // namespace scram

#endif  // SCRAM_SRC_PREPROCESSOR_H_
