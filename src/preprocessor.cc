/// @file preprocessor.cc
/// Implementation of preprocessing algorithms.
#include "preprocessor.h"

#include <algorithm>

#include "logger.h"

namespace scram {

Preprocessor::Preprocessor(IndexedFaultTree* fault_tree)
    : fault_tree_(fault_tree),
      top_event_sign_(1) {}

void Preprocessor::ProcessIndexedFaultTree() {
  IGatePtr top = fault_tree_->top_event();
  assert(top);
  LOG(DEBUG2) << "Propagating constants in a fault tree.";
  Preprocessor::PropagateConstants(top);
  LOG(DEBUG2) << "Constant propagation is done.";

  CLOCK(prep_time);
  LOG(DEBUG2) << "Preprocessing the fault tree.";
  LOG(DEBUG2) << "Normalizing gates.";
  assert(top_event_sign_ == 1);
  Preprocessor::NormalizeGates();
  LOG(DEBUG2) << "Finished normalizing gates.";

  Preprocessor::ClearGateVisits();
  Preprocessor::RemoveNullGates(top);

  if (top->type() == kNullGate) {  // Special case of preprocessing.
    assert(top->children().size() == 1);
    if (!top->gate_children().empty()) {
      int signed_index = top->gate_children().begin()->first;
      IGatePtr child = top->gate_children().begin()->second;
      fault_tree_->top_event(child);
      top = child;
      assert(top->type() == kOrGate || top->type() == kAndGate);
      top_event_sign_ *= signed_index > 0 ? 1 : -1;
    }
  }
  if (top->state() != kNormalState) {  // Top has become constant.
    if (top_event_sign_ < 0) {
      State orig_state = top->state();
      top = IGatePtr(new IGate(kNullGate));
      fault_tree_->top_event(top);
      if (orig_state == kNullState) {
        top->MakeUnity();
      } else {
        assert(orig_state == kUnityState);
        top->Nullify();
      }
      top_event_sign_ = 1;
    }
    return;
  }
  if (top_event_sign_ < 0) {
    assert(top->type() == kOrGate || top->type() == kAndGate ||
           top->type() == kNullGate);
    if (top->type() == kOrGate || top->type() == kAndGate)
      top->type(top->type() == kOrGate ? kAndGate : kOrGate);
    top->InvertChildren();
    top_event_sign_ = 1;
  }
  std::map<int, IGatePtr> complements;
  Preprocessor::ClearGateVisits();
  Preprocessor::PropagateComplements(top, &complements);
  Preprocessor::ClearGateVisits();
  Preprocessor::RemoveConstGates(top);
  bool tree_changed = true;
  while (tree_changed) {
    tree_changed = false;  // Break the loop if actions don't change the tree.
    bool ret = false;  // The result of actions of functions.
    Preprocessor::ClearGateVisits();
    ret = Preprocessor::RemoveNullGates(top);
    if (!tree_changed && ret) tree_changed = true;

    Preprocessor::ClearGateVisits();
    ret = Preprocessor::JoinGates(top);
    if (!tree_changed && ret) tree_changed = true;

    Preprocessor::ClearGateVisits();
    ret = Preprocessor::RemoveConstGates(top);
    if (!tree_changed && ret) tree_changed = true;
  }
  // After this point there should not be null AND or unity OR gates,
  // and the tree structure should be repeating OR and AND.
  // All gates are positive, and each gate has at least two children.
  if (top->children().empty()) return;  // This is null or unity.
  // Detect original modules for processing.
  Preprocessor::DetectModules(fault_tree_->basic_events().size());
  LOG(DEBUG2) << "Finished preprocessing the fault tree in " << DUR(prep_time);
}

void Preprocessor::NormalizeGates() {
  // Handle special case for a top event.
  IGatePtr top_gate = fault_tree_->top_event();
  GateType type = top_gate->type();
  switch (type) {
    case kNorGate:
    case kNandGate:
    case kNotGate:
      top_event_sign_ *= -1;
  }
  // Process negative gates. Note that top event's negative gate is processed in
  // the above lines.  All children are assumed to be positive at this point.
  Preprocessor::ClearGateVisits();
  Preprocessor::NotifyParentsOfNegativeGates(top_gate);

  Preprocessor::ClearGateVisits();
  Preprocessor::NormalizeGate(top_gate);
}

void Preprocessor::NotifyParentsOfNegativeGates(const IGatePtr& gate) {
  if (gate->Visited()) return;
  gate->Visit(1);
  std::vector<int> to_negate;  // Children to get the negation.
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    IGatePtr child = it->second;
    Preprocessor::NotifyParentsOfNegativeGates(child);
    switch (child->type()) {
      case kNorGate:
      case kNandGate:
      case kNotGate:
        to_negate.push_back(it->first);
    }
  }
  std::vector<int>::iterator it_neg;
  for (it_neg = to_negate.begin(); it_neg != to_negate.end(); ++it_neg) {
    gate->InvertChild(*it_neg);
  }
}

void Preprocessor::NormalizeGate(const IGatePtr& gate) {
  if (gate->Visited()) return;
  gate->Visit(1);

  // Depth-first traversal before the children may get changed.
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    Preprocessor::NormalizeGate(it->second);
  }

  GateType type = gate->type();
  switch (type) {  // Negation is already processed.
    case kNotGate:
      gate->type(kNullGate);
      break;
    case kNorGate:
    case kOrGate:
      gate->type(kOrGate);
      break;
    case kNandGate:
    case kAndGate:
      gate->type(kAndGate);
      break;
    case kXorGate:
      Preprocessor::NormalizeXorGate(gate);
      break;
    case kAtleastGate:
      Preprocessor::NormalizeAtleastGate(gate);
      break;
    default:
      assert(type == kNullGate);  // Must be dealt outside.
  }
}

void Preprocessor::NormalizeXorGate(const IGatePtr& gate) {
  assert(gate->children().size() == 2);
  IGatePtr gate_one(new IGate(kAndGate));
  IGatePtr gate_two(new IGate(kAndGate));

  gate->type(kOrGate);
  std::set<int>::const_iterator it = gate->children().begin();
  gate->ShareChild(*it, gate_one);
  gate->ShareChild(*it, gate_two);
  gate_two->InvertChild(*it);

  ++it;  // Handling the second child.
  gate->ShareChild(*it, gate_one);
  gate_one->InvertChild(*it);
  gate->ShareChild(*it, gate_two);

  gate->EraseAllChildren();
  gate->AddChild(gate_one->index(), gate_one);
  gate->AddChild(gate_two->index(), gate_two);
}

void Preprocessor::NormalizeAtleastGate(const IGatePtr& gate) {
  assert(gate->type() == kAtleastGate);
  int vote_number = gate->vote_number();

  assert(vote_number > 0);  // Vote number can be 1 for special OR gates.
  assert(gate->children().size() > 1);
  if (gate->children().size() == vote_number) {
    gate->type(kAndGate);
    return;
  } else if (vote_number == 1) {
    gate->type(kOrGate);
    return;
  }

  const std::set<int>& children = gate->children();
  std::set<int>::const_iterator it = children.begin();

  IGatePtr first_child(new IGate(kAndGate));
  gate->ShareChild(*it, first_child);

  IGatePtr grand_child(new IGate(kAtleastGate));
  first_child->AddChild(grand_child->index(), grand_child);
  grand_child->vote_number(vote_number - 1);

  IGatePtr second_child(new IGate(kAtleastGate));
  second_child->vote_number(vote_number);

  for (++it; it != children.end(); ++it) {
    gate->ShareChild(*it, grand_child);
    gate->ShareChild(*it, second_child);
  }

  gate->type(kOrGate);
  gate->EraseAllChildren();
  gate->AddChild(first_child->index(), first_child);
  gate->AddChild(second_child->index(), second_child);

  Preprocessor::NormalizeAtleastGate(grand_child);
  Preprocessor::NormalizeAtleastGate(second_child);
}

void Preprocessor::PropagateConstants(const IGatePtr& gate) {
  if (gate->Visited()) return;
  gate->Visit(1);  // Time does not matter.
  std::vector<int> to_erase;  // Erase children later to keep iterator valid.
  boost::unordered_map<int, ConstantPtr>::const_iterator it_c;
  for (it_c = gate->constant_children().begin();
       it_c != gate->constant_children().end(); ++it_c) {
    bool state = it_c->second->state();
    if (Preprocessor::ProcessConstantChild(gate, it_c->first, state, &to_erase))
      return;  // Early exit because the parent's state turned to NULL or UNITY.
  }
  boost::unordered_map<int, IGatePtr>::const_iterator it_g;
  for (it_g = gate->gate_children().begin();
       it_g != gate->gate_children().end(); ++it_g) {
    IGatePtr child_gate = it_g->second;
    Preprocessor::PropagateConstants(child_gate);
    State gate_state = child_gate->state();
    if (gate_state == kNormalState) continue;
    bool state = gate_state == kNullState ? false : true;
    if (Preprocessor::ProcessConstantChild(gate, it_g->first, state, &to_erase))
      return;  // Early exit because the parent's state turned to NULL or UNITY.
  }
  Preprocessor::RemoveChildren(gate, to_erase);
}

bool Preprocessor::ProcessConstantChild(const IGatePtr& gate,
                                        int child,
                                        bool state,
                                        std::vector<int>* to_erase) {
  GateType parent_type = gate->type();

  if (!state) {  // Null state child.
    switch (parent_type) {
      case kNorGate:
      case kXorGate:
      case kOrGate:
        to_erase->push_back(child);
        return false;
      case kNullGate:
      case kAndGate:
        gate->Nullify();
        break;
      case kNandGate:
      case kNotGate:
        gate->MakeUnity();
        break;
      case kAtleastGate:  // K / (N - 1).
        to_erase->push_back(child);
        int k = gate->vote_number();
        int n = gate->children().size() - to_erase->size();
        if (k == n) gate->type(kAndGate);
        return false;
    }
  } else {  // Unity state child.
    switch (parent_type) {
      case kNullGate:
      case kOrGate:
        gate->MakeUnity();
        break;
      case kNandGate:
      case kAndGate:
        to_erase->push_back(child);
        return false;
      case kNorGate:
      case kNotGate:
        gate->Nullify();
        break;
      case kXorGate:  // Special handling due to its internal negation.
        assert(gate->children().size() == 2);
        if (to_erase->size() == 1) {  // The other child is NULL.
          gate->MakeUnity();
        } else {
          assert(to_erase->empty());
          gate->type(kNotGate);
          to_erase->push_back(child);
          return false;
        }
        break;
      case kAtleastGate:  // (K - 1) / (N - 1).
        int k = gate->vote_number();
        --k;
        if (k == 1) gate->type(kOrGate);
        assert(k > 1);
        gate->vote_number(k);
        to_erase->push_back(child);
        return false;
    }
  }
  return true;  // Becomes constant NULL or UNITY most of the cases.
}

void Preprocessor::RemoveChildren(const IGatePtr& gate,
                                  const std::vector<int>& to_erase) {
  if (to_erase.empty()) return;
  assert(to_erase.size() <= gate->children().size());
  std::vector<int>::const_iterator it_v;
  for (it_v = to_erase.begin(); it_v != to_erase.end(); ++it_v) {
    gate->EraseChild(*it_v);
  }
  GateType type = gate->type();
  if (gate->children().empty()) {
    assert(type != kNotGate && type != kNullGate);  // Constant by design.
    assert(type != kAtleastGate);  // Must get transformed by design.
    switch (type) {
      case kNandGate:
      case kXorGate:
      case kOrGate:
        gate->Nullify();
        break;
      case kNorGate:
      case kAndGate:
        gate->MakeUnity();
        break;
    }
  } else if (gate->children().size() == 1) {
    assert(type != kAtleastGate);  // Cannot have only one child by processing.
    switch (type) {
      case kXorGate:
      case kOrGate:
      case kAndGate:
        gate->type(kNullGate);
        break;
      case kNorGate:
      case kNandGate:
        gate->type(kNotGate);
        break;
      default:
        assert(type == kNotGate || type == kNullGate);
    }
  }
}

void Preprocessor::PropagateComplements(
    const IGatePtr& gate,
    std::map<int, IGatePtr>* gate_complements) {
  if (gate->Visited()) return;
  gate->Visit(1);
  // If the child gate is complement, then create a new gate that propagates
  // its sign to its children and itself becomes non-complement.
  // Keep track of complement gates for optimization of repeated complements.
  std::vector<int> to_swap;  // Children with negation to get swaped.
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    IGatePtr child_gate = it->second;
    if (it->first < 0) {
      to_swap.push_back(it->first);
      if (gate_complements->count(child_gate->index())) continue;
      GateType type = child_gate->type();
      assert(type == kAndGate || type == kOrGate);
      GateType complement_type = type == kOrGate ? kAndGate : kOrGate;
      IGatePtr complement_gate(new IGate(complement_type));
      gate_complements->insert(std::make_pair(child_gate->index(),
                                              complement_gate));
      complement_gate->CopyChildren(child_gate);
      complement_gate->InvertChildren();
      child_gate = complement_gate;  // Needed for further propagation.
    }
    Preprocessor::PropagateComplements(child_gate, gate_complements);
  }

  std::vector<int>::iterator it_ch;
  for (it_ch = to_swap.begin(); it_ch != to_swap.end(); ++it_ch) {
    assert(*it_ch < 0);
    gate->EraseChild(*it_ch);
    IGatePtr complement = gate_complements->find(-*it_ch)->second;
    bool ret = gate->AddChild(complement->index(), complement);
    assert(ret);
  }
}

bool Preprocessor::RemoveConstGates(const IGatePtr& gate) {
  if (gate->Visited()) return false;
  gate->Visit(1);  // Time does not matter.

  if (gate->state() == kNullState || gate->state() == kUnityState) return false;
  bool changed = false;  // Indication if this operation changed the gate.
  std::vector<int> to_erase;  // Keep track of children to erase.
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    assert(it->first > 0);
    IGatePtr child_gate = it->second;
    bool ret = Preprocessor::RemoveConstGates(child_gate);
    if (!changed && ret) changed = true;
    State state = child_gate->state();
    if (state == kNormalState) continue;  // Only three states are possible.
    bool state_flag = state == kNullState ? false : true;
    if (Preprocessor::ProcessConstantChild(gate, it->first, state_flag,
                                           &to_erase))
      return true;  // The parent gate itself has become constant.
  }
  if (!changed && !to_erase.empty()) changed = true;
  Preprocessor::RemoveChildren(gate, to_erase);
  return changed;
}

bool Preprocessor::RemoveNullGates(const IGatePtr& gate) {
  if (gate->Visited()) return false;
  gate->Visit(1);  // Time does not matter.
  std::vector<int> null_children;  // Null type gate children.
  bool changed = false;  // Indication if the tree is changed.
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    IGatePtr child_gate = it->second;
    bool ret = Preprocessor::RemoveNullGates(child_gate);
    if (!changed && ret) changed = true;

    if (child_gate->type() == kNullGate && child_gate->state() == kNormalState)
      null_children.push_back(it->first);
  }

  std::vector<int>::iterator it_swap;
  for (it_swap = null_children.begin(); it_swap != null_children.end();
       ++it_swap) {
    if (!gate->JoinNullGate(*it_swap)) return true;  // Becomes constant.
    if (!changed) changed = true;
  }
  return changed;
}

bool Preprocessor::JoinGates(const IGatePtr& gate) {
  if (gate->Visited()) return false;
  gate->Visit(1);  // Time does not matter.
  GateType parent_type = gate->type();
  std::vector<IGatePtr> to_join;  // Gate children of the same logic.
  bool changed = false;  // Indication if the tree is changed.
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    bool ret = false;  // Indication if the sub-tree has changed.
    IGatePtr child_gate = it->second;
    ret = Preprocessor::JoinGates(child_gate);
    if (!changed && ret) changed = true;
    if (it->first < 0) continue;  // Cannot join a negative child gate.
    if (child_gate->IsModule()) continue;  // Does not coalesce modules.

    GateType child_type = child_gate->type();

    switch (parent_type) {
      case kNandGate:
      case kAndGate:
        if (child_type == kAndGate) to_join.push_back(child_gate);
        break;
      case kNorGate:
      case kOrGate:
        if (child_type == kOrGate) to_join.push_back(child_gate);
        break;
    }
  }

  if (!changed && !to_join.empty()) changed = true;
  std::vector<IGatePtr>::iterator it_ch;
  for (it_ch = to_join.begin(); it_ch != to_join.end(); ++it_ch) {
    if (!gate->JoinGate(*it_ch)) return true;  // The parent is constant.
  }
  return changed;
}

void Preprocessor::DetectModules(int num_basic_events) {
  // First stage, traverse the tree depth-first for gates and indicate
  // visit time for each node.
  LOG(DEBUG2) << "Detecting modules in a fault tree.";

  // First and last visits of basic events.
  // Basic events are indexed 1 to the number of basic events sequentially.
  int visit_basics[num_basic_events + 1][2];
  for (int i = 0; i < num_basic_events + 1; ++i) {
    visit_basics[i][0] = 0;
    visit_basics[i][1] = 0;
  }
  Preprocessor::ClearGateVisits();

  IGatePtr top_gate = fault_tree_->top_event();
  int time = 0;
  Preprocessor::AssignTiming(time, top_gate, visit_basics);

  LOG(DEBUG3) << "Timings are assigned to nodes.";

  std::map<int, std::pair<int, int> > visited_gates;
  Preprocessor::FindModules(top_gate, visit_basics, &visited_gates);
  assert(visited_gates.count(top_gate->index()));
  assert(visited_gates.find(top_gate->index())->second.first == 1);
  assert(!top_gate->Revisited());
  assert(visited_gates.find(top_gate->index())->second.second ==
         top_gate->ExitTime());
}

int Preprocessor::AssignTiming(int time, const IGatePtr& gate,
                               int visit_basics[][2]) {
  if (gate->Visit(++time)) return time;  // Revisited gate.
  assert(gate->constant_children().empty());

  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    time = Preprocessor::AssignTiming(time, it->second, visit_basics);
  }

  boost::unordered_map<int, IBasicEventPtr>::const_iterator it_b;
  for (it_b = gate->basic_event_children().begin();
       it_b != gate->basic_event_children().end(); ++it_b) {
    int index = it_b->second->index();  /// @todo Replace with Node visit times.
    if (!visit_basics[index][0]) {  // The first time the node is visited.
      visit_basics[index][0] = ++time;
      visit_basics[index][1] = time;
    } else {
      visit_basics[index][1] = ++time;  // Revisiting the child node.
    }
  }
  bool re_visited = gate->Visit(++time);  // Exiting the gate in second visit.
  assert(!re_visited);  // No cyclic visiting.
  return time;
}

void Preprocessor::FindModules(
    const IGatePtr& gate,
    const int visit_basics[][2],
    std::map<int, std::pair<int, int> >* visited_gates) {
  if (visited_gates->count(gate->index())) return;
  int enter_time = gate->EnterTime();
  int exit_time = gate->ExitTime();
  int min_time = enter_time;
  int max_time = exit_time;

  std::vector<int> non_shared_children;  // Non-shared module children.
  std::vector<int> modular_children;  // Children that satisfy modularity.
  std::vector<int> non_modular_children;  // Cannot be grouped into a module.
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    int min = 0;  // Minimum time of the visit of the sub-tree.
    int max = 0;  // Maximum time of the visit of the sub-tree.
    IGatePtr child_gate = it->second;
    Preprocessor::FindModules(child_gate, visit_basics, visited_gates);
    min = visited_gates->find(child_gate->index())->second.first;
    max = visited_gates->find(child_gate->index())->second.second;
    if (child_gate->IsModule() && !child_gate->Revisited()) {
      non_shared_children.push_back(it->first);
      continue;
    }
    assert(min > 0);
    assert(max > 0);
    if (min > enter_time && max < exit_time) {
      modular_children.push_back(it->first);
    } else {
      non_modular_children.push_back(it->first);
    }
    min_time = std::min(min_time, min);
    max_time = std::max(max_time, max);
  }
  boost::unordered_map<int, IBasicEventPtr>::const_iterator it_b;
  for (it_b = gate->basic_event_children().begin();
       it_b != gate->basic_event_children().end(); ++it_b) {
    int index = it_b->second->index();
    int min = visit_basics[index][0];
    int max = visit_basics[index][1];
    if (min == max) {
      assert(min > enter_time && max < exit_time);
      non_shared_children.push_back(it_b->first);
      continue;
    }
    assert(min > 0);
    assert(max > 0);
    if (min > enter_time && max < exit_time) {
      modular_children.push_back(it_b->first);
    } else {
      non_modular_children.push_back(it_b->first);
    }
    min_time = std::min(min_time, min);
    max_time = std::max(max_time, max);
  }

  // Determine if this gate is module itself.
  if (min_time == enter_time && max_time == exit_time) {
    LOG(DEBUG3) << "Found original module: " << gate->index();
    assert((modular_children.size() + non_shared_children.size()) ==
           gate->children().size());
    gate->TurnModule();
  }

  max_time = std::max(max_time, gate->LastVisit());
  visited_gates->insert(std::make_pair(gate->index(),
                                       std::make_pair(min_time, max_time)));

  // Attempting to create new modules for specific gate types.
  switch (gate->type()) {
    case kNorGate:
    case kOrGate:
    case kNandGate:
    case kAndGate:
      Preprocessor::CreateNewModule(gate, non_shared_children);

      LOG(DEBUG4) << "Filtering modular children.";
      Preprocessor::FilterModularChildren(visit_basics,
                                          *visited_gates,
                                          &modular_children,
                                          &non_modular_children);
      assert(modular_children.size() != 1);  // One modular child is non-shared.
      std::vector<std::vector<int> > groups;
      LOG(DEBUG4) << "Grouping modular children is Disabled.";
      groups.push_back(modular_children);
      // Preprocessor::GroupModularChildren(visit_basics, *visited_gates,
      //                                    modular_children, &groups);
      LOG(DEBUG4) << "Creating new modules from modular children.";
      Preprocessor::CreateNewModules(gate, modular_children, groups);
  }
}

boost::shared_ptr<IGate> Preprocessor::CreateNewModule(
    const IGatePtr& gate,
    const std::vector<int>& children) {
  IGatePtr module;  // Empty pointer as an indication of a failure.
  if (children.empty()) return module;
  if (children.size() == 1) return module;
  if (children.size() == gate->children().size()) {
    assert(gate->IsModule());
    return module;
  }
  assert(children.size() < gate->children().size());
  switch (gate->type()) {
    case kNandGate:
    case kAndGate:
      module = IGatePtr(new IGate(kAndGate));
      break;
    case kNorGate:
    case kOrGate:
      module = IGatePtr(new IGate(kOrGate));
      break;
    default:
      return module;  // Cannot create sub-modules for other types.
  }
  module->TurnModule();
  std::vector<int>::const_iterator it_g;
  for (it_g = children.begin(); it_g != children.end(); ++it_g) {
    gate->TransferChild(*it_g, module);
  }
  gate->AddChild(module->index(), module);
  assert(gate->children().size() > 1);
  LOG(DEBUG3) << "Created a new module for Gate " << gate->index() << ": "
      << "Gate " << module->index() << " with "  << children.size()
      << " NON-SHARED children.";
  return module;
}

void Preprocessor::FilterModularChildren(
    const int visit_basics[][2],
    const std::map<int, std::pair<int, int> >& visited_gates,
    std::vector<int>* modular_children,
    std::vector<int>* non_modular_children) {
  if (modular_children->empty() || non_modular_children->empty()) return;
  std::vector<int> new_non_modular;
  std::vector<int> still_modular;
  std::vector<int>::iterator it;
  for (it = modular_children->begin(); it != modular_children->end(); ++it) {
    int index = std::abs(*it);
    int min = 0;
    int max = 0;
    if (visited_gates.count(index)) {
      min = visited_gates.find(index)->second.first;
      max = visited_gates.find(index)->second.second;
    } else {
      min = visit_basics[index][0];
      max = visit_basics[index][1];
    }
    std::vector<int>::iterator it_n;
    for (it_n = non_modular_children->begin();
         it_n != non_modular_children->end(); ++it_n) {
      int index = std::abs(*it_n);
      int lower = 0;
      int upper = 0;
      if (visited_gates.count(index)) {
        lower = visited_gates.find(index)->second.first;
        upper = visited_gates.find(index)->second.second;
      } else {
        lower = visit_basics[index][0];
        upper = visit_basics[index][1];
      }
      int a = std::max(min, lower);
      int b = std::min(max, upper);
      if (a <= b) {  // There's some overlap between the ranges.
        new_non_modular.push_back(*it);
      } else {
        still_modular.push_back(*it);
      }
    }
  }
  Preprocessor::FilterModularChildren(visit_basics, visited_gates,
                                      &still_modular, &new_non_modular);
  *modular_children = still_modular;
  non_modular_children->insert(non_modular_children->end(),
                               new_non_modular.begin(), new_non_modular.end());
}

void Preprocessor::GroupModularChildren(
    const int visit_basics[][2],
    const std::map<int, std::pair<int, int> >& visited_gates,
    const std::vector<int>& modular_children,
    std::vector<std::vector<int> >* groups) {
  if (modular_children.empty()) return;
  assert(modular_children.size() > 1);
  std::vector<int> to_check(modular_children);
  while (!to_check.empty()) {
    std::vector<int> group;
    int first_member = to_check.back();
    to_check.pop_back();
    group.push_back(first_member);
    int low = 0;
    int high = 0;
    int index = std::abs(first_member);
    if (visited_gates.count(index)) {
      low = visited_gates.find(index)->second.first;
      high = visited_gates.find(index)->second.second;
    } else {
      low = visit_basics[index][0];
      high = visit_basics[index][1];
    }
    int prev_size = 0;
    std::vector<int> next_check;
    while (prev_size < group.size()) {
      prev_size = group.size();
      std::vector<int>::iterator it;
      for (it = to_check.begin(); it != to_check.end(); ++it) {
        int min = 0;
        int max = 0;
        int index = std::abs(*it);
        if (visited_gates.count(index)) {
          min = visited_gates.find(index)->second.first;
          max = visited_gates.find(index)->second.second;
        } else {
          min = visit_basics[index][0];
          max = visit_basics[index][1];
        }
        int a = std::max(min, low);
        int b = std::min(max, high);
        if (a <= b) {  // There's some overlap between the ranges.
          group.push_back(*it);
          low = std::min(min, low);
          high = std::max(max, low);
        } else {
          next_check.push_back(*it);
        }
      }
      to_check = next_check;
    }
    assert(group.size() > 1);
    groups->push_back(group);
  }
}

void Preprocessor::CreateNewModules(
    const IGatePtr& gate,
    const std::vector<int>& modular_children,
    const std::vector<std::vector<int> >& groups) {
  if (modular_children.empty()) return;
  assert(modular_children.size() > 1);
  assert(!groups.empty());
  if (modular_children.size() == gate->children().size() &&
      groups.size() == 1) {
    assert(gate->IsModule());
    return;
  }
  IGatePtr main_child;

  if (modular_children.size() == gate->children().size()) {
    assert(groups.size() > 1);
    assert(gate->IsModule());
    main_child = gate;
  } else {
    main_child = Preprocessor::CreateNewModule(gate, modular_children);
    assert(main_child);
  }
  std::vector<std::vector<int> >::const_iterator it;
  for (it = groups.begin(); it != groups.end(); ++it) {
    Preprocessor::CreateNewModule(main_child, *it);
  }
}

void Preprocessor::ClearGateVisits() {
  Preprocessor::ClearGateVisits(fault_tree_->top_event());
}

void Preprocessor::ClearGateVisits(const IGatePtr& gate) {
  gate->ClearVisits();
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    Preprocessor::ClearGateVisits(it->second);
  }
  boost::unordered_map<int, IBasicEventPtr>::const_iterator it_b;
  for (it_b = gate->basic_event_children().begin();
       it_b != gate->basic_event_children().end(); ++it_b) {
    it_b->second->ClearVisits();
  }
  boost::unordered_map<int, ConstantPtr>::const_iterator it_c;
  for (it_c = gate->constant_children().begin();
       it_c != gate->constant_children().end(); ++it_c) {
    it_c->second->ClearVisits();
  }
}

}  // namespace scram
