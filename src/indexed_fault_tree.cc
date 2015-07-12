/// @file indexed_fault_tree.cc
/// Implementation of IndexedFaultTree class and helper functions to
/// efficiently preprocess and find minimal cut sets from a fault tree.
#include "indexed_fault_tree.h"

#include <algorithm>

#include <boost/assign.hpp>

#include "event.h"
#include "logger.h"

namespace scram {

const std::map<std::string, GateType> IndexedFaultTree::kStringToType_ =
    boost::assign::map_list_of("and", kAndGate) ("or", kOrGate)
                              ("atleast", kAtleastGate) ("xor", kXorGate)
                              ("not", kNotGate) ("nand", kNandGate)
                              ("nor", kNorGate) ("null", kNullGate);

IndexedFaultTree::IndexedFaultTree(int top_event_id)
    : top_event_index_(top_event_id),
      gate_index_(top_event_id),
      new_gate_index_(0),
      top_event_sign_(1) {}

void IndexedFaultTree::InitiateIndexedFaultTree(
    const boost::unordered_map<int, GatePtr>& int_to_inter,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  // Assume that new ccf_gates are not re-added into general index container.
  new_gate_index_ = all_to_int.size() + ccf_basic_to_gates.size() + 1;

  boost::unordered_map<int, GatePtr>::const_iterator it;
  for (it = int_to_inter.begin(); it != int_to_inter.end(); ++it) {
    IndexedFaultTree::ProcessFormula(it->first, it->second->formula(),
                                     ccf_basic_to_gates, all_to_int);
  }
}

void IndexedFaultTree::PropagateConstants(
    const std::set<int>& true_house_events,
    const std::set<int>& false_house_events) {
  if (true_house_events.empty() && false_house_events.empty()) return;
  IndexedFaultTree::ClearGateVisits();
  IndexedGatePtr top = indexed_gates_.find(top_event_index_)->second;
  LOG(DEBUG2) << "Propagating constants in a fault tree.";
  IndexedFaultTree::PropagateConstants(true_house_events, false_house_events,
                                       top);
  LOG(DEBUG2) << "Constant propagation is done.";
}

void IndexedFaultTree::ProcessIndexedFaultTree(int num_basic_events) {
  LOG(DEBUG2) << "Normalizing gates.";
  assert(top_event_sign_ == 1);
  IndexedFaultTree::NormalizeGates();
  LOG(DEBUG2) << "Finished normalizing gates.";

  IndexedGatePtr top = indexed_gates_.find(top_event_index_)->second;
  if (top_event_sign_ < 0) {
    assert(top->type() == kOrGate || top->type() == kAndGate);
    top->type(top->type() == kOrGate ? kAndGate : kOrGate);
    top->InvertChildren();
    top_event_sign_ = 1;
  }
  std::map<int, int> complements;
  IndexedFaultTree::ClearGateVisits();
  IndexedFaultTree::PropagateComplements(top, &complements);
  IndexedFaultTree::ClearGateVisits();
  IndexedFaultTree::ProcessConstGates(top);
  do {
    IndexedFaultTree::ClearGateVisits();
    if (!IndexedFaultTree::JoinGates(top)) break;
    // Cleanup null and unity gates. There is no negative gate.
    IndexedFaultTree::ClearGateVisits();
  } while (IndexedFaultTree::ProcessConstGates(top));
  // After this point there should not be null AND or unity OR gates,
  // and the tree structure should be repeating OR and AND.
  // All gates are positive, and each gate has at least two children.
  if (top->children().empty()) return;  // This is null or unity.
  // Detect original modules for processing.
  IndexedFaultTree::DetectModules(num_basic_events);
}

void IndexedFaultTree::ProcessFormula(
    int index,
    const FormulaPtr& formula,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  assert(!indexed_gates_.count(index));
  GateType type = kStringToType_.find(formula->type())->second;
  IndexedGatePtr gate(new IndexedGate(index, type));
  if (type == kAtleastGate) gate->vote_number(formula->vote_number());

  typedef boost::shared_ptr<Event> EventPtr;

  const std::map<std::string, EventPtr>* children = &formula->event_args();
  std::map<std::string, EventPtr>::const_iterator it_children;
  for (it_children = children->begin(); it_children != children->end();
       ++it_children) {
    int child_index = all_to_int.find(it_children->first)->second;
    // Replace CCF basic events with the corresponding events.
    if (ccf_basic_to_gates.count(it_children->first))
      child_index = ccf_basic_to_gates.find(it_children->first)->second;
    gate->InitiateWithChild(child_index);
  }
  const std::set<FormulaPtr>* formulas = &formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas->begin(); it_f != formulas->end(); ++it_f) {
    int child_index = ++new_gate_index_;
    IndexedFaultTree::ProcessFormula(child_index, *it_f, ccf_basic_to_gates,
                                     all_to_int);
    gate->InitiateWithChild(child_index);
  }
  indexed_gates_.insert(std::make_pair(gate->index(), gate));
}

void IndexedFaultTree::NormalizeGates() {
  // Handle special case for a top event.
  IndexedGatePtr top_gate = indexed_gates_.find(top_event_index_)->second;
  GateType type = top_gate->type();
  switch (type) {
    case kNorGate:
      top_event_sign_ *= -1;  // For negative gates. Fall-through to OR case.
    case kOrGate:
      top_gate->type(kOrGate);
      break;
    case kNandGate:
      top_event_sign_ *= -1;  // For negative gates. Fall-through to AND case.
    case kAndGate:
      top_gate->type(kAndGate);
      break;
    case kNotGate:
      top_event_sign_ *= -1;  // Change the sign. Fall-through to NULL case.
    case kNullGate:
      assert(top_gate->children().size() == 1);
      int child_index = *top_gate->children().begin();
      assert(child_index > 0);
      top_gate = indexed_gates_.find(std::abs(child_index))->second;
      indexed_gates_.erase(top_event_index_);
      top_event_index_ = top_gate->index();
      IndexedFaultTree::NormalizeGates();  // This should handle NOT->NOT cases.
      return;
  }
  // Gather parent information for negative gate processing.
  IndexedFaultTree::ClearGateVisits();
  IndexedFaultTree::GatherParentInformation(top_gate);
  // Process negative gates except for NOT. Note that top event's negative
  // gate is processed in the above lines.
  // All children are assumed to be positive at this point.
  boost::unordered_map<int, IndexedGatePtr>::iterator it;
  for (it = indexed_gates_.begin(); it != indexed_gates_.end(); ++it) {
    if (it->first == top_event_index_) continue;
    IndexedFaultTree::NotifyParentsOfNegativeGates(it->second);
  }

  // Assumes that all gates are in indexed_gates_ container.
  boost::unordered_map<int, IndexedGatePtr> original_gates(indexed_gates_);
  for (it = original_gates.begin(); it != original_gates.end(); ++it) {
    IndexedFaultTree::NormalizeGate(it->second);
  }
  // Note that parent information is invalid from this point.
}

void IndexedFaultTree::GatherParentInformation(
    const IndexedGatePtr& parent_gate) {
  if (parent_gate->Visited()) return;
  parent_gate->Visit(1);  // Time does not matter.

  std::set<int>::const_iterator it;
  for (it = parent_gate->children().begin();
       it != parent_gate->children().end(); ++it) {
    int index = std::abs(*it);
    if (index > gate_index_) {
      IndexedGatePtr child = indexed_gates_.find(index)->second;
      child->AddParent(parent_gate->index());
      IndexedFaultTree::GatherParentInformation(child);
    }
  }
}

void IndexedFaultTree::NotifyParentsOfNegativeGates(
    const IndexedGatePtr& gate) {
  GateType type = gate->type();
  // Deal with negative gate.
  if (type == kNorGate || type == kNandGate) {
    int child_index = gate->index();
    std::set<int>::const_iterator it;
    for (it = gate->parents().begin(); it != gate->parents().end(); ++it) {
      IndexedGatePtr parent = indexed_gates_.find(*it)->second;
      assert(parent->children().count(child_index));  // Positive child.
      bool ret = parent->SwapChild(child_index, -child_index);
      assert(ret);
    }
  }
}

void IndexedFaultTree::NormalizeGate(const IndexedGatePtr& gate) {
  GateType type = gate->type();
  switch (type) {  // Negation is already processed.
    case kNorGate:
    case kOrGate:
      gate->type(kOrGate);
      break;
    case kNandGate:
    case kAndGate:
      gate->type(kAndGate);
      break;
    case kXorGate:
      IndexedFaultTree::NormalizeXorGate(gate);
      break;
    case kAtleastGate:
      IndexedFaultTree::NormalizeAtleastGate(gate);
      break;
    default:
      assert(type == kNotGate || type == kNullGate);  // Dealt in coalescing.
  }
}

void IndexedFaultTree::NormalizeXorGate(const IndexedGatePtr& gate) {
  assert(gate->children().size() == 2);
  std::set<int>::const_iterator it = gate->children().begin();
  IndexedGatePtr gate_one(new IndexedGate(++new_gate_index_, kAndGate));
  IndexedGatePtr gate_two(new IndexedGate(++new_gate_index_, kAndGate));

  gate->type(kOrGate);
  indexed_gates_.insert(std::make_pair(gate_one->index(), gate_one));
  indexed_gates_.insert(std::make_pair(gate_two->index(), gate_two));

  gate_one->AddChild(*it);
  gate_two->AddChild(-*it);
  ++it;
  gate_one->AddChild(-*it);
  gate_two->AddChild(*it);
  gate->EraseAllChildren();
  gate->AddChild(gate_one->index());
  gate->AddChild(gate_two->index());
}

void IndexedFaultTree::NormalizeAtleastGate(const IndexedGatePtr& gate) {
  int vote_number = gate->vote_number();

  assert(vote_number > 1);
  assert(gate->children().size() > vote_number);
  std::set< std::set<int> > all_sets;
  const std::set<int>* children = &gate->children();

  std::set<int>::iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    std::set<int> set;
    set.insert(*it);
    all_sets.insert(set);
  }
  for (int i = 1; i < vote_number; ++i) {
    std::set< std::set<int> > tmp_sets;
    std::set< std::set<int> >::iterator it_sets;
    for (it_sets = all_sets.begin(); it_sets != all_sets.end(); ++it_sets) {
      for (it = children->begin(); it != children->end(); ++it) {
        std::set<int> set(*it_sets);
        set.insert(*it);
        if (set.size() > i) {
          tmp_sets.insert(set);
        }
      }
    }
    all_sets = tmp_sets;
  }

  gate->type(kOrGate);
  gate->EraseAllChildren();
  std::set< std::set<int> >::iterator it_sets;
  for (it_sets = all_sets.begin(); it_sets != all_sets.end(); ++it_sets) {
    IndexedGatePtr gate_one(new IndexedGate(++new_gate_index_, kAndGate));
    std::set<int>::iterator it;
    for (it = it_sets->begin(); it != it_sets->end(); ++it) {
      bool ret = gate_one->AddChild(*it);
      assert(ret);
    }
    gate->AddChild(gate_one->index());
    indexed_gates_.insert(std::make_pair(gate_one->index(), gate_one));
  }
}

void IndexedFaultTree::PropagateConstants(
    const std::set<int>& true_house_events,
    const std::set<int>& false_house_events,
    const IndexedGatePtr& gate) {
  if (gate->Visited()) return;
  gate->Visit(1);  // Time does not matter.
  std::set<int>::const_iterator it;
  std::vector<int> to_erase;  // Erase children later to keep iterator is valid.
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    assert(*it > 0);
    bool state = false;  // Null or Unity case. Null indication by default.
    if (*it > gate_index_) {  // Processing a gate child.
      // Depth-first traversal.
      IndexedGatePtr child_gate = indexed_gates_.find(*it)->second;
      IndexedFaultTree::PropagateConstants(true_house_events,
                                           false_house_events, child_gate);
      State gate_state = child_gate->state();
      if (gate_state == kNormalState) continue;
      state = gate_state == kNullState ? false : true;

    } else {  // Processing a primary event child.
      if (false_house_events.count(*it)) {
        state = false;
      } else if (true_house_events.count(*it)) {
        state = true;
      } else {
        continue;  // This must be a basic event child. It is not constant.
      }
    }
    if (IndexedFaultTree::ProcessConstantChild(gate, *it, state, &to_erase))
      return;  // Early exit because the parent's state turned to NULL or UNITY.
  }
  IndexedFaultTree::RemoveChildren(gate, to_erase);
}

bool IndexedFaultTree::ProcessConstantChild(const IndexedGatePtr& gate,
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

void IndexedFaultTree::RemoveChildren(const IndexedGatePtr& gate,
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

void IndexedFaultTree::PropagateComplements(
    const IndexedGatePtr& gate,
    std::map<int, int>* gate_complements) {
  // If the child gate is complement, then create a new gate that propagates
  // its sign to its children and itself becomes non-complement.
  // Keep track of complement gates for optimization of repeated complements.
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end();) {
    if (std::abs(*it) > gate_index_) {
      // Deal with NOT and NULL gates.
      IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
      if (child_gate->type() == kNotGate || child_gate->type() == kNullGate) {
        assert(child_gate->children().size() == 1);
        int mult = child_gate->type() == kNotGate ? -1 : 1;
        mult *= *it > 0 ? 1 : -1;
        if (!gate->SwapChild(*it, *child_gate->children().begin() * mult))
          return;
        it = gate->children().begin();
        continue;
      }
      if (*it < 0) {
        if (gate_complements->count(-*it)) {
          gate->SwapChild(*it, gate_complements->find(-*it)->second);
        } else {
          GateType type = indexed_gates_.find(-*it)->second->type();
          assert(type == kAndGate || type == kOrGate);
          GateType complement_type = type == kOrGate ? kAndGate : kOrGate;
          IndexedGatePtr complement_gate(new IndexedGate(++new_gate_index_,
                                                         complement_type));
          indexed_gates_.insert(std::make_pair(complement_gate->index(),
                                               complement_gate));
          gate_complements->insert(std::make_pair(-*it,
                                                  complement_gate->index()));
          complement_gate->children(
              indexed_gates_.find(-*it)->second->children());
          complement_gate->InvertChildren();
          gate->SwapChild(*it, complement_gate->index());
          complement_gate->Visit(1);
          IndexedFaultTree::PropagateComplements(complement_gate,
                                                 gate_complements);
        }
        // Note that the iterator is invalid now.
        it = gate->children().begin();  // The negative gates at the start.
        continue;
      } else if (!child_gate->Visited()) {
        // Continue with the positive gate children.
        child_gate->Visit(1);  // Time does not matter.
        IndexedFaultTree::PropagateComplements(child_gate, gate_complements);
      }
    }
    ++it;
  }
}

bool IndexedFaultTree::ProcessConstGates(const IndexedGatePtr& gate) {
  // Null state gates' parent: OR->Remove the child and AND->NULL the parent.
  // Unity state gates' parent: OR->Unity the parent and AND->Remove the child.
  // The tree structure is only positive AND and OR gates.
  if (gate->Visited()) return false;
  gate->Visit(1);  // Time does not matter.

  if (gate->state() == kNullState || gate->state() == kUnityState) return false;
  bool changed = false;  // Indication if this operation changed the gate.
  std::vector<int> to_erase;  // Keep track of children to erase.
  GateType type = gate->type();
  assert(type == kAndGate || type == kOrGate);  // Only two types are possible.
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    if (std::abs(*it) > gate_index_) {
      assert(*it > 0);
      IndexedGatePtr child_gate = indexed_gates_.find(*it)->second;
      bool ret = IndexedFaultTree::ProcessConstGates(child_gate);
      if (!changed && ret) changed = true;
      State state = child_gate->state();
      if (state == kNormalState) continue;  // Only three states are possible.
      bool state_flag = state == kNullState ? false : true;
      if (IndexedFaultTree::ProcessConstantChild(gate, *it, state_flag,
                                                 &to_erase))
        return true;
    }
  }
  if (!changed && !to_erase.empty()) changed = true;
  IndexedFaultTree::RemoveChildren(gate, to_erase);
  return changed;
}

bool IndexedFaultTree::JoinGates(const IndexedGatePtr& gate) {
  if (gate->Visited()) return false;
  gate->Visit(1);  // Time does not matter.
  GateType parent_type = gate->type();
  assert(parent_type == kAndGate || parent_type == kOrGate);
  std::set<int>::const_iterator it;
  bool changed = false;  // Indication if the tree is changed.
  for (it = gate->children().begin(); it != gate->children().end();) {
    if (std::abs(*it) > gate_index_) {
      assert(*it > 0);
      IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
      GateType child_type = child_gate->type();
      assert(child_type == kAndGate || child_type == kOrGate);
      if (parent_type == child_type) {  // Parent is not NULL or NOT.
        if (!changed) changed = true;
        if (!gate->JoinGate(&*indexed_gates_.find(*it)->second)) {
          break;
        } else {
          it = gate->children().begin();
          continue;
        }
      } else if (child_gate->children().size() == 1) {
        // This must be from some reduced gate after constant propagation.
        if (!changed) changed = true;
        if (!gate->SwapChild(*it, *child_gate->children().begin()))
          break;
        it = gate->children().begin();
        continue;
      } else {
        bool ret = IndexedFaultTree::JoinGates(child_gate);
        if (!changed && ret) changed = true;
      }
    }
    ++it;
  }
  return changed;
}

void IndexedFaultTree::DetectModules(int num_basic_events) {
  // At this stage only AND/OR gates are present.
  // All one element gates and non-coherent gates are converted and processed.
  // All constants are propagated and there are only gates and basic events.
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
  IndexedFaultTree::ClearGateVisits();

  IndexedGatePtr top_gate = indexed_gates_.find(top_event_index_)->second;
  int time = 0;
  IndexedFaultTree::AssignTiming(time, top_gate, visit_basics);

  LOG(DEBUG3) << "Timings are assigned to nodes.";

  std::map<int, std::pair<int, int> > visited_gates;
  IndexedFaultTree::FindOriginalModules(top_gate, visit_basics, &visited_gates);
  assert(visited_gates.count(top_event_index_));
  assert(visited_gates.find(top_event_index_)->second.first == 1);
  assert(!top_gate->Revisited());
  assert(visited_gates.find(top_event_index_)->second.second ==
         top_gate->ExitTime());

  int orig_mod = modules_.size();
  LOG(DEBUG2) << "Detected number of original modules: " << modules_.size();
}

int IndexedFaultTree::AssignTiming(int time, const IndexedGatePtr& gate,
                                   int visit_basics[][2]) {
  if (gate->Visit(++time)) return time;  // Revisited gate.

  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    int index = std::abs(*it);
    if (index < top_event_index_) {
      if (!visit_basics[index][0]) {
        visit_basics[index][0] = ++time;
        visit_basics[index][1] = time;
      } else {
        visit_basics[index][1] = ++time;
      }
    } else {
      time = IndexedFaultTree::AssignTiming(time,
                                            indexed_gates_.find(index)->second,
                                            visit_basics);
    }
  }
  bool re_visited = gate->Visit(++time);  // Exiting the gate in second visit.
  assert(!re_visited);  // No cyclic visiting.
  return time;
}

void IndexedFaultTree::FindOriginalModules(
    const IndexedGatePtr& gate,
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
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    int index = std::abs(*it);
    int min = 0;
    int max = 0;
    if (index < top_event_index_) {
      min = visit_basics[index][0];
      max = visit_basics[index][1];
      if (min == max) {
        assert(min > enter_time && max < exit_time);
        non_shared_children.push_back(*it);
        continue;
      }
    } else {
      assert(*it > 0);
      IndexedGatePtr child_gate = indexed_gates_.find(index)->second;
      IndexedFaultTree::FindOriginalModules(child_gate, visit_basics,
                                            visited_gates);
      min = visited_gates->find(index)->second.first;
      max = visited_gates->find(index)->second.second;
      if (modules_.count(index) && !child_gate->Revisited()) {
        non_shared_children.push_back(*it);
        continue;
      }
    }
    assert(min != 0);
    assert(max != 0);
    if (min > enter_time && max < exit_time) {
      modular_children.push_back(*it);
    } else {
      non_modular_children.push_back(*it);
    }
    min_time = std::min(min_time, min);
    max_time = std::max(max_time, max);
  }

  // Determine if this gate is module itself.
  if (min_time == enter_time && max_time == exit_time) {
    LOG(DEBUG3) << "Found original module: " << gate->index();
    assert((modular_children.size() + non_shared_children.size()) ==
           gate->children().size());
    modules_.insert(gate->index());
  }
  if (non_shared_children.size() > 1) {
    IndexedFaultTree::CreateNewModule(gate, non_shared_children);
    LOG(DEBUG3) << "New module of " << gate->index() << ": " << new_gate_index_
        << " with NON-SHARED children number " << non_shared_children.size();
  }
  // There might be cases when in one level couple of child gates can be
  // grouped into a module but they may share an event with another non-module
  // gate which in turn shares an event with the outside world. This leads
  // to a chain that needs to be considered. Formula rewriting might be helpful
  // in this case.
  IndexedFaultTree::FilterModularChildren(visit_basics,
                                          *visited_gates,
                                          &modular_children,
                                          &non_modular_children);
  if (modular_children.size() > 0) {
    assert(modular_children.size() != 1);  // One modular child is non-shared.
    IndexedFaultTree::CreateNewModule(gate, modular_children);
    LOG(DEBUG3) << "New module of gate " << gate->index() << ": "
        << new_gate_index_
        << " with children number " << modular_children.size();
  }

  max_time = std::max(max_time, gate->LastVisit());
  visited_gates->insert(std::make_pair(gate->index(),
                                       std::make_pair(min_time, max_time)));
}

void IndexedFaultTree::CreateNewModule(const IndexedGatePtr& gate,
                                       const std::vector<int>& children) {
  assert(children.size() > 1);
  assert(children.size() <= gate->children().size());
  if (children.size() == gate->children().size()) {
    if (modules_.count(gate->index())) return;
    modules_.insert(gate->index());
    return;
  }
  assert(gate->type() == kAndGate || gate->type() == kOrGate);
  IndexedGatePtr new_module(new IndexedGate(++new_gate_index_, gate->type()));
  indexed_gates_.insert(std::make_pair(new_gate_index_, new_module));
  modules_.insert(new_gate_index_);
  std::vector<int>::const_iterator it_g;
  for (it_g = children.begin(); it_g != children.end(); ++it_g) {
    gate->EraseChild(*it_g);
    new_module->InitiateWithChild(*it_g);
  }
  assert(!gate->children().empty());
  gate->InitiateWithChild(new_module->index());
}

void IndexedFaultTree::FilterModularChildren(
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
    if (index < gate_index_) {
      min = visit_basics[index][0];
      max = visit_basics[index][1];
    } else {
      assert(*it > 0);
      min = visited_gates.find(index)->second.first;
      max = visited_gates.find(index)->second.second;
    }
    bool modular = true;
    std::vector<int>::iterator it_n;
    for (it_n = non_modular_children->begin();
         it_n != non_modular_children->end(); ++it_n) {
      int index = std::abs(*it_n);
      int lower = 0;
      int upper = 0;
      if (index < gate_index_) {
        lower = visit_basics[index][0];
        upper = visit_basics[index][1];
      } else {
        assert(*it_n > 0);
        lower = visited_gates.find(index)->second.first;
        upper = visited_gates.find(index)->second.second;
      }
      int a = std::max(min, lower);
      int b = std::min(max, upper);
      if (a <= b) {  // There's some overlap between the ranges.
        new_non_modular.push_back(*it);
        modular = false;
        break;
      }
    }
    if (modular) still_modular.push_back(*it);
  }
  IndexedFaultTree::FilterModularChildren(visit_basics, visited_gates,
                                          &still_modular, &new_non_modular);
  *modular_children = still_modular;
  non_modular_children->insert(non_modular_children->end(),
                               new_non_modular.begin(), new_non_modular.end());
}

void IndexedFaultTree::ClearGateVisits() {
  boost::unordered_map<int, IndexedGatePtr>::iterator it;
  for (it = indexed_gates_.begin(); it != indexed_gates_.end(); ++it) {
    it->second->ClearVisits();
  }
}

}  // namespace scram
