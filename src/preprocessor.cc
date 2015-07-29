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
/// @file preprocessor.cc
/// Implementation of preprocessing algorithms.
#include "preprocessor.h"

#include <algorithm>
#include <queue>

#include "logger.h"

namespace scram {

Preprocessor::Preprocessor(IndexedFaultTree* fault_tree)
    : fault_tree_(fault_tree),
      top_event_sign_(1) {}

void Preprocessor::ProcessIndexedFaultTree() {
  IGatePtr top = fault_tree_->top_event();
  assert(top);
  assert(top->parents().empty());
  assert(!top->mark());
  LOG(DEBUG2) << "Propagating constants in a fault tree.";
  Preprocessor::PropagateConstants(top);
  LOG(DEBUG2) << "Constant propagation is done.";

  CLOCK(prep_time);
  LOG(DEBUG2) << "Preprocessing the fault tree.";
  LOG(DEBUG2) << "Normalizing gates.";
  assert(top_event_sign_ == 1);
  Preprocessor::NormalizeGates();
  LOG(DEBUG2) << "Finished normalizing gates.";

  Preprocessor::ClearGateMarks();
  Preprocessor::RemoveNullGates(top);

  if (top->type() == kNullGate) {  // Special case of preprocessing.
    assert(top->children().size() == 1);
    if (!top->gate_children().empty()) {
      int signed_index = top->gate_children().begin()->first;
      IGatePtr child = top->gate_children().begin()->second;
      fault_tree_->top_event(child);
      top = child;
      assert(top->parents().empty());
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

  std::map<int, IGatePtr> complements; // Must be cleaned!
  Preprocessor::ClearGateMarks();
  Preprocessor::PropagateComplements(top, &complements);
  complements.clear();  // Cleaning to get rid of extra reference counts.

  Preprocessor::ClearGateMarks();
  Preprocessor::RemoveConstGates(top);

  if (fault_tree_->coherent()) {
    Preprocessor::ClearGateMarks();
    Preprocessor::RemoveNullGates(top);
    Preprocessor::BooleanOptimization();
  }

  bool tree_changed = true;
  while (tree_changed) {
    tree_changed = false;  // Break the loop if actions don't change the tree.
    bool ret = false;  // The result of actions of functions.
    Preprocessor::ClearGateMarks();
    ret = Preprocessor::RemoveNullGates(top);
    if (!tree_changed && ret) tree_changed = true;

    Preprocessor::ClearGateMarks();
    ret = Preprocessor::JoinGates(top);
    if (!tree_changed && ret) tree_changed = true;

    Preprocessor::ClearGateMarks();
    ret = Preprocessor::RemoveConstGates(top);
    if (!tree_changed && ret) tree_changed = true;
  }
  // After this point there should not be null AND or unity OR gates,
  // and the tree structure should be repeating OR and AND.
  // All gates are positive, and each gate has at least two children.
  if (top->children().empty()) return;  // This is null or unity.
  // Detect original modules for processing.
  Preprocessor::DetectModules();
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
  Preprocessor::ClearGateMarks();
  Preprocessor::NotifyParentsOfNegativeGates(top_gate);

  Preprocessor::ClearGateMarks();
  Preprocessor::NormalizeGate(top_gate);
}

void Preprocessor::NotifyParentsOfNegativeGates(const IGatePtr& gate) {
  if (gate->mark()) return;
  gate->mark(true);
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
  if (gate->mark()) return;
  gate->mark(true);

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
  gate_one->mark(true);
  gate_two->mark(true);

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

  first_child->mark(true);
  second_child->mark(true);
  grand_child->mark(true);

  gate->type(kOrGate);
  gate->EraseAllChildren();
  gate->AddChild(first_child->index(), first_child);
  gate->AddChild(second_child->index(), second_child);

  Preprocessor::NormalizeAtleastGate(grand_child);
  Preprocessor::NormalizeAtleastGate(second_child);
}

void Preprocessor::PropagateConstants(const IGatePtr& gate) {
  if (gate->mark()) return;
  gate->mark(true);
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

bool Preprocessor::RemoveConstGates(const IGatePtr& gate) {
  if (gate->mark()) return false;
  gate->mark(true);

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

void Preprocessor::PropagateComplements(
    const IGatePtr& gate,
    std::map<int, IGatePtr>* gate_complements) {
  if (gate->mark()) return;
  gate->mark(true);
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
      IGatePtr complement_gate;
      if (child_gate->parents().size() == 1) {
        child_gate->type(complement_type);
        child_gate->InvertChildren();
        complement_gate = child_gate;
      } else {
        complement_gate = IGatePtr(new IGate(complement_type));
        complement_gate->CopyChildren(child_gate);
        complement_gate->InvertChildren();
      }
      gate_complements->insert(std::make_pair(child_gate->index(),
                                              complement_gate));
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

bool Preprocessor::RemoveNullGates(const IGatePtr& gate) {
  if (gate->mark()) return false;
  gate->mark(true);
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
  if (gate->mark()) return false;
  gate->mark(true);
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

void Preprocessor::DetectModules() {
  // First stage, traverse the tree depth-first for gates and indicate
  // visit time for each node.
  LOG(DEBUG2) << "Detecting modules in a fault tree.";

  Preprocessor::ClearNodeVisits();

  IGatePtr top_gate = fault_tree_->top_event();
  int time = 0;
  Preprocessor::AssignTiming(time, top_gate);

  LOG(DEBUG3) << "Timings are assigned to nodes.";

  Preprocessor::ClearGateMarks();
  Preprocessor::FindModules(top_gate);

  assert(!top_gate->Revisited());
  assert(top_gate->min_time() == 1);
  assert(top_gate->max_time() == top_gate->ExitTime());
}

int Preprocessor::AssignTiming(int time, const IGatePtr& gate) {
  if (gate->Visit(++time)) return time;  // Revisited gate.
  assert(gate->constant_children().empty());

  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    time = Preprocessor::AssignTiming(time, it->second);
  }

  boost::unordered_map<int, IBasicEventPtr>::const_iterator it_b;
  for (it_b = gate->basic_event_children().begin();
       it_b != gate->basic_event_children().end(); ++it_b) {
    it_b->second->Visit(++time);  // Enter the leaf.
    it_b->second->Visit(time);  // Exit at the same time.
  }
  bool re_visited = gate->Visit(++time);  // Exiting the gate in second visit.
  assert(!re_visited);  // No cyclic visiting.
  return time;
}

void Preprocessor::FindModules(const IGatePtr& gate) {
  if (gate->mark()) return;
  gate->mark(true);
  int enter_time = gate->EnterTime();
  int exit_time = gate->ExitTime();
  int min_time = enter_time;
  int max_time = exit_time;

  std::vector<std::pair<int, NodePtr> > non_shared_children;
  std::vector<std::pair<int, NodePtr> > modular_children;
  std::vector<std::pair<int, NodePtr> > non_modular_children;
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    IGatePtr child_gate = it->second;
    Preprocessor::FindModules(child_gate);
    if (child_gate->IsModule() && !child_gate->Revisited()) {
      assert(child_gate->parents().size() == 1);
      assert(child_gate->parents().count(&*gate));

      non_shared_children.push_back(*it);
      continue;  // Sub-tree's visit times are within the Enter and Exit time.
    }
    int min = child_gate->min_time();
    int max = child_gate->max_time();
    assert(min > 0);
    assert(max > 0);
    assert(max > min);
    if (min > enter_time && max < exit_time) {
      modular_children.push_back(*it);
    } else {
      non_modular_children.push_back(*it);
    }
    min_time = std::min(min_time, min);
    max_time = std::max(max_time, max);
  }

  boost::unordered_map<int, IBasicEventPtr>::const_iterator it_b;
  for (it_b = gate->basic_event_children().begin();
       it_b != gate->basic_event_children().end(); ++it_b) {
    IBasicEventPtr child = it_b->second;
    int min = child->EnterTime();
    int max = child->LastVisit();
    assert(min > 0);
    assert(max > 0);
    if (min == max) {
      assert(min > enter_time && max < exit_time);
      assert(child->parents().size() == 1);
      assert(child->parents().count(&*gate));

      non_shared_children.push_back(*it_b);
      continue;  // The single parent child.
    }
    assert(max > min);
    if (min > enter_time && max < exit_time) {
      modular_children.push_back(*it_b);
    } else {
      non_modular_children.push_back(*it_b);
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
  gate->min_time(min_time);
  gate->max_time(max_time);

  // Attempting to create new modules for specific gate types.
  switch (gate->type()) {
    case kNorGate:
    case kOrGate:
    case kNandGate:
    case kAndGate:
      Preprocessor::CreateNewModule(gate, non_shared_children);

      Preprocessor::FilterModularChildren(&modular_children,
                                          &non_modular_children);
      assert(modular_children.size() != 1);  // One modular child is non-shared.
      std::vector<std::vector<std::pair<int, NodePtr> > > groups;
      Preprocessor::GroupModularChildren(modular_children, &groups);
      Preprocessor::CreateNewModules(gate, modular_children, groups);
  }
}

boost::shared_ptr<IGate> Preprocessor::CreateNewModule(
    const IGatePtr& gate,
    const std::vector<std::pair<int, NodePtr> >& children) {
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
  module->mark(true);
  std::vector<std::pair<int, NodePtr> >::const_iterator it;
  for (it = children.begin(); it != children.end(); ++it) {
    gate->TransferChild(it->first, module);
  }
  gate->AddChild(module->index(), module);
  assert(gate->children().size() > 1);
  LOG(DEBUG3) << "Created a new module for Gate " << gate->index() << ": "
      << "Gate " << module->index() << " with "  << children.size()
      << " NON-SHARED children.";
  return module;
}

void Preprocessor::FilterModularChildren(
    std::vector<std::pair<int, NodePtr> >* modular_children,
    std::vector<std::pair<int, NodePtr> >* non_modular_children) {
  if (modular_children->empty() || non_modular_children->empty()) return;
  std::vector<std::pair<int, NodePtr> > new_non_modular;
  std::vector<std::pair<int, NodePtr> > still_modular;
  std::vector<std::pair<int, NodePtr> >::iterator it;
  for (it = modular_children->begin(); it != modular_children->end(); ++it) {
    int min = it->second->min_time();
    int max = it->second->max_time();
    bool non_module = false;
    std::vector<std::pair<int, NodePtr> >::iterator it_n;
    for (it_n = non_modular_children->begin();
         it_n != non_modular_children->end(); ++it_n) {
      int lower = it_n->second->min_time();
      int upper = it_n->second->max_time();
      int a = std::max(min, lower);
      int b = std::min(max, upper);
      if (a <= b) {  // There's some overlap between the ranges.
        non_module = true;
        break;
      }
    }
    if (non_module) {
      new_non_modular.push_back(*it);
    } else{
      still_modular.push_back(*it);
    }
  }
  Preprocessor::FilterModularChildren(&still_modular, &new_non_modular);
  *modular_children = still_modular;
  non_modular_children->insert(non_modular_children->end(),
                               new_non_modular.begin(), new_non_modular.end());
}

void Preprocessor::GroupModularChildren(
    const std::vector<std::pair<int, NodePtr> >& modular_children,
    std::vector<std::vector<std::pair<int, NodePtr> > >* groups) {
  if (modular_children.empty()) return;
  assert(modular_children.size() > 1);
  std::vector<std::pair<int, NodePtr> > to_check(modular_children);
  while (!to_check.empty()) {
    std::vector<std::pair<int, NodePtr> > group;
    NodePtr first_member = to_check.back().second;
    group.push_back(to_check.back());
    to_check.pop_back();
    int low = first_member->min_time();
    int high = first_member->max_time();

    int prev_size = 0;
    std::vector<std::pair<int, NodePtr> > next_check;
    while (prev_size < group.size()) {
      prev_size = group.size();
      std::vector<std::pair<int, NodePtr> >::iterator it;
      for (it = to_check.begin(); it != to_check.end(); ++it) {
        int min = it->second->min_time();
        int max = it->second->max_time();
        int a = std::max(min, low);
        int b = std::min(max, high);
        if (a <= b) {  // There's some overlap between the ranges.
          group.push_back(*it);
          low = std::min(min, low);
          high = std::max(max, high);
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
    const std::vector<std::pair<int, NodePtr> >& modular_children,
    const std::vector<std::vector<std::pair<int, NodePtr> > >& groups) {
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
  std::vector<std::vector<std::pair<int, NodePtr> > >::const_iterator it;
  for (it = groups.begin(); it != groups.end(); ++it) {
    Preprocessor::CreateNewModule(main_child, *it);
  }
}

void Preprocessor::BooleanOptimization() {
  Preprocessor::ClearNodeVisits();
  Preprocessor::ClearGateMarks();

  std::vector<boost::weak_ptr<IGate> > common_gates;
  std::vector<boost::weak_ptr<IBasicEvent> > common_basic_events;
  Preprocessor::GatherCommonNodes(&common_gates, &common_basic_events);

  Preprocessor::ClearNodeVisits();
  std::vector<boost::weak_ptr<IGate> >::iterator it;
  for (it = common_gates.begin(); it != common_gates.end(); ++it) {
    Preprocessor::ProcessCommonNode(*it);
  }

  std::vector<boost::weak_ptr<IBasicEvent> >::iterator it_b;
  for (it_b = common_basic_events.begin(); it_b != common_basic_events.end();
       ++it_b) {
    Preprocessor::ProcessCommonNode(*it_b);
  }
}

void Preprocessor::GatherCommonNodes(
      std::vector<boost::weak_ptr<IGate> >* common_gates,
      std::vector<boost::weak_ptr<IBasicEvent> >* common_basic_events) {
  std::queue<IGatePtr> gates_queue;
  gates_queue.push(fault_tree_->top_event());
  while (!gates_queue.empty()) {
    IGatePtr gate = gates_queue.front();
    gates_queue.pop();
    boost::unordered_map<int, IGatePtr>::const_iterator it;
    for (it = gate->gate_children().begin(); it != gate->gate_children().end();
         ++it) {
      IGatePtr child_gate = it->second;
      assert(child_gate->state() == kNormalState);
      if (child_gate->Visited()) continue;
      child_gate->Visit(1);
      gates_queue.push(child_gate);
      if (child_gate->parents().size() > 1) common_gates->push_back(child_gate);
    }

    boost::unordered_map<int, IBasicEventPtr>::const_iterator it_b;
    for (it_b = gate->basic_event_children().begin();
         it_b != gate->basic_event_children().end(); ++it_b) {
      IBasicEventPtr child = it_b->second;
      if (child->Visited()) continue;
      child->Visit(1);
      if (child->parents().size() > 1) common_basic_events->push_back(child);
    }
  }
}

template<class N>
void Preprocessor::ProcessCommonNode(const boost::weak_ptr<N>& common_node) {
  if (common_node.expired()) return;  // The node has been deleted.

  boost::shared_ptr<N> node = common_node.lock();

  if (node->parents().size() == 1) return;  // The parent is deleted.

  IGatePtr top = fault_tree_->top_event();
  Preprocessor::ClearOptiValues(top);

  assert(node->opti_value() == 0);
  node->opti_value(1);
  int mult_tot = node->parents().size();  // Total multiplicity.
  assert(mult_tot > 1);
  mult_tot += Preprocessor::PropagateFailure(&*node);
  // The results of the failure propagation.
  std::map<int, boost::weak_ptr<IGate> > destinations;
  int num_dest = 0;  // This is not the same as the size of destinations.
  if (top->opti_value() == 1) {  // The top gate failed.
    destinations.insert(std::make_pair(top->index(), top));
    num_dest = 1;
  } else {
    assert(top->opti_value() == 0);
    num_dest = Preprocessor::CollectFailureDestinations(top, node->index(),
                                                        &destinations);
  }

  if (num_dest == 0) return;  // No failure destination detected.
  assert(!destinations.empty());
  if (num_dest < mult_tot) {  // Redundancy detection.
    bool created_constant =
        Preprocessor::ProcessRedundantParents(node, &destinations);
    Preprocessor::ProcessFailureDestinations(node, destinations);
    if (created_constant) {
      Preprocessor::ClearGateMarks();
      Preprocessor::RemoveConstGates(top);
      Preprocessor::ClearGateMarks();
      Preprocessor::RemoveNullGates(top);
    }
  }
}

int Preprocessor::PropagateFailure(Node* node) {
  assert(node->opti_value() == 1);
  std::set<IGate*>::iterator it;
  int mult_tot = 0;
  for (it = node->parents().begin(); it != node->parents().end(); ++it) {
    IGate* parent = *it;
    if (parent->opti_value() == 1) continue;
    parent->ChildFailed();  // Send a notification.
    if (parent->opti_value() == 1) {  // The parent failed.
      int mult = parent->parents().size();  // Multiplicity of the parent.
      if (mult > 1) mult_tot += mult;  // Total multiplicity.
      mult_tot += Preprocessor::PropagateFailure(parent);
    }
  }
  return mult_tot;
}

int Preprocessor::CollectFailureDestinations(
    const IGatePtr& gate,
    int index,
    std::map<int, boost::weak_ptr<IGate> >* destinations) {
  assert(gate->opti_value() == 0);
  if (gate->children().count(index)) {  // Child may be non-gate.
    gate->opti_value(3);
  } else {
    gate->opti_value(2);
  }
  int num_dest = 0;
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    IGatePtr child = it->second;
    if (child->opti_value() == 0) {
      num_dest +=
          Preprocessor::CollectFailureDestinations(child, index, destinations);
    } else if (child->opti_value() == 1 && child->index() != index) {
      ++num_dest;
      destinations->insert(std::make_pair(child->index(), child));
    } // Ignore gates with optimization values of 2 or 3.
  }
  return num_dest;
}

bool Preprocessor::ProcessRedundantParents(
    const NodePtr& node,
    std::map<int, boost::weak_ptr<IGate> >* destinations) {
  std::vector<boost::weak_ptr<IGate> > redundant_parents;
  std::set<IGate*>::const_iterator it;
  for (it = node->parents().begin(); it != node->parents().end(); ++it) {
    IGate* parent = *it;
    if (parent->opti_value() < 3) {
      // Special cases for the redundant parent and the destination parent.
      switch (parent->type()) {
        case kOrGate:
          if (destinations->count(parent->index())) {
            destinations->erase(parent->index());
            continue;  // No need to add into the redundancy list.
          }
      }
      redundant_parents.push_back(Preprocessor::RawToWeakPointer(parent));
    }
  }
  // The node behaves like a constant False for redundant parents.
  bool created_constant = false;  // Parents turned into constants.
  std::vector<boost::weak_ptr<IGate> >::iterator it_r;
  for (it_r = redundant_parents.begin(); it_r != redundant_parents.end();
       ++it_r) {
    if (it_r->expired()) continue;
    IGatePtr parent = it_r->lock();
    switch (parent->type()) {
      case kAndGate:
        parent->Nullify();
        if (!created_constant) created_constant = true;
        break;
      case kOrGate:
        assert(parent->children().size() > 1);
        parent->EraseChild(node->index());
        if (parent->children().size() == 1) parent->type(kNullGate);
        break;
      case kAtleastGate:
        assert(parent->children().size() > 2);
        parent->EraseChild(node->index());
        if (parent->children().size() == parent->vote_number())
          parent->type(kAndGate);
        break;
      default:
        assert(false);
    }
  }
  return created_constant;
}

template<class N>
void Preprocessor::ProcessFailureDestinations(
    const boost::shared_ptr<N>& node,
    const std::map<int, boost::weak_ptr<IGate> >& destinations) {
  std::map<int, boost::weak_ptr<IGate> >::const_iterator it_d;
  for (it_d = destinations.begin(); it_d != destinations.end(); ++it_d) {
    if (it_d->second.expired()) continue;
    IGatePtr target = it_d->second.lock();
    assert(target->type() != kNullGate);
    switch (target->type()) {
      case kOrGate:
        target->AddChild(node->index(), node);
        break;
      case kAndGate:
      case kAtleastGate:
        IGatePtr new_gate(new IGate(target->type()));
        new_gate->vote_number(target->vote_number());
        new_gate->CopyChildren(target);
        target->EraseAllChildren();
        target->type(kOrGate);
        target->AddChild(new_gate->index(), new_gate);
        target->AddChild(node->index(), node);
        break;
    }
  }
}

boost::weak_ptr<IGate> Preprocessor::RawToWeakPointer(const IGate* parent) {
  if (parent->index() == fault_tree_->top_event()->index())
    return fault_tree_->top_event();
  assert(!parent->parents().empty());
  const IGate* grand_parent = *parent->parents().begin();
  return grand_parent->gate_children().find(parent->index())->second;
}

void Preprocessor::ClearGateMarks() {
  Preprocessor::ClearGateMarks(fault_tree_->top_event());
}

void Preprocessor::ClearGateMarks(const IGatePtr& gate) {
  if (!gate->mark()) return;
  gate->mark(false);
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    Preprocessor::ClearGateMarks(it->second);
  }
}

void Preprocessor::ClearNodeVisits() {
  Preprocessor::ClearNodeVisits(fault_tree_->top_event());
}

void Preprocessor::ClearNodeVisits(const IGatePtr& gate) {
  gate->ClearVisits();
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    Preprocessor::ClearNodeVisits(it->second);
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

void Preprocessor::ClearOptiValues(const IGatePtr& gate) {
  gate->opti_value(0);
  gate->ResetChildrenFailure();
  boost::unordered_map<int, IGatePtr>::const_iterator it;
  for (it = gate->gate_children().begin(); it != gate->gate_children().end();
       ++it) {
    Preprocessor::ClearOptiValues(it->second);
  }
  boost::unordered_map<int, IBasicEventPtr>::const_iterator it_b;
  for (it_b = gate->basic_event_children().begin();
       it_b != gate->basic_event_children().end(); ++it_b) {
    it_b->second->opti_value(0);
  }
  assert(gate->constant_children().empty());
}

}  // namespace scram
