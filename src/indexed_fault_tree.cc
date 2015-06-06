/// @file indexed_fault_tree.cc
/// Implementation of IndexedFaultTree class and helper functions to
/// efficiently find minimal cut sets from a fault tree.
#include "indexed_fault_tree.h"

#include <algorithm>

#include "event.h"
#include "indexed_gate.h"
#include "logger.h"

namespace scram {

int SimpleGate::limit_order_ = 20;

void SimpleGate::GenerateCutSets(const SetPtr& cut_set,
                                 std::set<SetPtr, SetPtrComp>* new_cut_sets) {
  assert(cut_set->size() <= limit_order_);
  if (type_ == 1) {  // OR gate operations.
    SimpleGate::OrGateCutSets(cut_set, new_cut_sets);

  } else {  // AND gate operations.
    SimpleGate::AndGateCutSets(cut_set, new_cut_sets);
  }
}

void SimpleGate::AndGateCutSets(const SetPtr& cut_set,
                                std::set<SetPtr, SetPtrComp>* new_cut_sets) {
  assert(cut_set->size() <= limit_order_);
  // Check for null case.
  std::vector<int>::iterator it;
  for (it = basic_events_.begin(); it != basic_events_.end(); ++it) {
    if (cut_set->count(-*it)) return;
  }
  // Limit order checks before other expensive operations.
  int order = cut_set->size();
  for (it = basic_events_.begin(); it != basic_events_.end(); ++it) {
    if (!cut_set->count(*it)) ++order;
    if (order > limit_order_) return;
  }
  for (it = modules_.begin(); it != modules_.end(); ++it) {
    if (!cut_set->count(*it)) ++order;
    if (order > limit_order_) return;
  }
  SetPtr cut_set_copy(new std::set<int>(*cut_set));
  // Include all basic events and modules into the set.
  for (it = basic_events_.begin(); it != basic_events_.end(); ++it) {
    cut_set_copy->insert(*it);
  }
  for (it = modules_.begin(); it != modules_.end(); ++it) {
    cut_set_copy->insert(*it);
  }

  // Deal with many OR gate children.
  SetPtrComp comp;
  std::set<SetPtr, SetPtrComp> arguments;  // Input to OR gates.
  arguments.insert(cut_set_copy);
  std::vector<SimpleGatePtr>::iterator it_g;
  for (it_g = gates_.begin(); it_g != gates_.end(); ++it_g) {
    std::set<SetPtr, SetPtrComp>::iterator it_s;
    std::set<SetPtr, SetPtrComp> results(comp);
    for (it_s = arguments.begin(); it_s != arguments.end(); ++it_s) {
      (*it_g)->OrGateCutSets(*it_s, &results);
    }
    arguments = results;
  }
  if (!arguments.empty() &&
      (*arguments.begin())->size() == cut_set_copy->size()) {
    new_cut_sets->insert(cut_set_copy);
  } else {
    new_cut_sets->insert(arguments.begin(), arguments.end());
  }
}

void SimpleGate::OrGateCutSets(const SetPtr& cut_set,
                               std::set<SetPtr, SetPtrComp>* new_cut_sets) {
  assert(cut_set->size() <= limit_order_);
  // Check for local minimality.
  std::vector<int>::iterator it;
  for (it = basic_events_.begin(); it != basic_events_.end(); ++it) {
    if (cut_set->count(*it)) {
      new_cut_sets->insert(cut_set);
      return;
    }
  }
  for (it = modules_.begin(); it != modules_.end(); ++it) {
    if (cut_set->count(*it)) {
      new_cut_sets->insert(cut_set);
      return;
    }
  }
  // There is a guarantee of a size increase of a cut set.
  if (cut_set->size() < limit_order_) {
    // Create new cut sets from basic events and modules.
    for (it = basic_events_.begin(); it != basic_events_.end(); ++it) {
      if (!cut_set->count(-*it)) {
        SetPtr new_set(new std::set<int>(*cut_set));
        new_set->insert(*it);
        new_cut_sets->insert(new_set);
      }
    }
    for (it = modules_.begin(); it != modules_.end(); ++it) {
      // No check for complements. The modules are assumed to be positive.
      SetPtr new_set(new std::set<int>(*cut_set));
      new_set->insert(*it);
      new_cut_sets->insert(new_set);
    }
  }

  // Generate cut sets from child gates of AND type.
  std::vector<SimpleGatePtr>::iterator it_g;
  SetPtrComp comp;
  std::set<SetPtr, SetPtrComp> local_sets(comp);
  for (it_g = gates_.begin(); it_g != gates_.end(); ++it_g) {
    (*it_g)->AndGateCutSets(cut_set, &local_sets);
    if (!local_sets.empty() &&
        (*local_sets.begin())->size() == cut_set->size()) {
      new_cut_sets->insert(cut_set);
      return;
    }
  }
  new_cut_sets->insert(local_sets.begin(), local_sets.end());
}

IndexedFaultTree::IndexedFaultTree(int top_event_id, int limit_order)
    : top_event_index_(top_event_id),
      gate_index_(top_event_id),
      new_gate_index_(0),
      limit_order_(limit_order),
      top_event_sign_(1) {
  SimpleGate::limit_order(limit_order_);
}

void IndexedFaultTree::InitiateIndexedFaultTree(
    const boost::unordered_map<int, GatePtr>& int_to_inter,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  boost::unordered_map<int, GatePtr>::const_iterator it;
  for (it = int_to_inter.begin(); it != int_to_inter.end(); ++it) {
    IndexedGatePtr gate(new IndexedGate(it->first));
    gate->string_type(it->second->type());  // Get the original gate type.
    if (gate->string_type() == "atleast")
      gate->vote_number(it->second->vote_number());

    typedef boost::shared_ptr<Event> EventPtr;

    const std::map<std::string, EventPtr>* children =
        &it->second->children();
    std::map<std::string, EventPtr>::const_iterator it_children;
    for (it_children = children->begin();
         it_children != children->end(); ++it_children) {
      int child_index = all_to_int.find(it_children->first)->second;
      // Replace CCF basic events with the corresponding events.
      if (ccf_basic_to_gates.count(it_children->first))
        child_index = ccf_basic_to_gates.find(it_children->first)->second;
      gate->InitiateWithChild(child_index);
    }
    indexed_gates_.insert(std::make_pair(it->first, gate));
    if (gate->index() > new_gate_index_) new_gate_index_ = gate->index() + 1;
  }

  LOG(DEBUG2) << "Unrolling gates.";
  assert(top_event_sign_ == 1);
  IndexedFaultTree::UnrollGates();
  LOG(DEBUG2) << "Finished unrolling gates.";
}

void IndexedFaultTree::PropagateConstants(
    const std::set<int>& true_house_events,
    const std::set<int>& false_house_events) {
  IndexedGatePtr top = indexed_gates_.find(top_event_index_)->second;
  std::set<int> processed_gates;
  LOG(DEBUG2) << "Propagating constants in a fault tree.";
  IndexedFaultTree::PropagateConstants(true_house_events, false_house_events,
                                       top, &processed_gates);
  LOG(DEBUG2) << "Constant propagation is done.";
}

void IndexedFaultTree::ProcessIndexedFaultTree(int num_basic_events) {
  IndexedGatePtr top = indexed_gates_.find(top_event_index_)->second;
  if (top_event_sign_ < 0) {
    top->type(top->type() == 1 ? 2 : 1);
    top->InvertChildren();
    top_event_sign_ = 1;
  }
  std::map<int, int> complements;
  std::set<int> processed_gates;
  IndexedFaultTree::PropagateComplements(top, &complements, &processed_gates);
  processed_gates.clear();
  IndexedFaultTree::ProcessConstGates(top, &processed_gates);
  do {
    processed_gates.clear();
    if (!IndexedFaultTree::JoinGates(top, &processed_gates)) break;
    // Cleanup null and unity gates. There is no negative gate.
    processed_gates.clear();
  } while (IndexedFaultTree::ProcessConstGates(top, &processed_gates));
  // After this point there should not be null AND or unity OR gates,
  // and the tree structure should be repeating OR and AND.
  // All gates are positive, and each gate has at least two children.
  if (top->children().empty()) return;  // This is null or unity.
  // Detect original modules for processing.
  IndexedFaultTree::DetectModules(num_basic_events);
}

void IndexedFaultTree::FindMcs() {
  // It is assumed that the tree is layered with OR and AND gates on each
  // level. That is, one level contains only AND or OR gates.
  // The function assumes the tree contains only positive gates.
  //
  // The description of the algorithm.
  //
  // Turn all existing gates in the tree into simple gates
  // with pointers to the children gates but not modules.
  // Leave minimal cut set modules to the last moment till all the gates
  // are operated. Those modules' minimal cut sets can be joined without
  // additional check for minimality.
  //
  // Operate on each module starting from the top gate.
  // For now we assume that a module cannot be unity, this means that
  // a module will at least add a new event into a cut set, so size of
  // a cut set with modules is a minimum number of members in the set.
  // This will fail if there is unity case but will hold if the module is
  // null because the cut set will be deleted anyway.
  //
  // Upon walking from top to children gates, there are two types: OR and AND.
  // The generated sets are passed to child gates, which use the passed set
  // to generate new sets. AND gate will simply add its basic events and
  // modules to the set and pass the resultant sets into its OR child, which
  // will generate a lot more sets. These generated sets are passed to the
  // next gate child to generate even more.
  //
  // For OR gates, the passed set is checked to have basic events of the gate.
  // If so, this is a local minimum cut set, so generation of the sets stops
  // on this gate. No new sets should be generated in this case. This condition
  // is also applicable if the child AND gate keeps the input set as output and
  // generates only additional supersets.
  //
  // The generated sets are kept unique by storing them in a set.
  CLOCK(mcs_time);
  LOG(DEBUG2) << "Start minimal cut set generation.";

  // Special case of empty top gate.
  IndexedGatePtr top = indexed_gates_.find(top_event_index_)->second;
  if (top->children().empty()) {
    std::string state = top->state();
    assert(state == "null" || state == "unity");
    assert(top_event_sign_ > 0);
    if (state == "unity") {
      std::set<int> empty_set;
      imcs_.push_back(empty_set);
    }  // Other cases are null.
    return;
  }

  // Create simple gates from indexed gates.
  std::map<int, SimpleGatePtr> simple_gates;
  IndexedFaultTree::CreateSimpleTree(top_event_index_, &simple_gates);

  LOG(DEBUG3) << "Finding MCS from top module: " << top_event_index_;
  std::vector<std::set<int> > mcs;
  IndexedFaultTree::FindMcsFromSimpleGate(
      simple_gates.find(top_event_index_)->second, &mcs);

  LOG(DEBUG3) << "Top gate cut sets are generated.";

  // The next is to join all other modules.
  LOG(DEBUG3) << "Joining modules.";
  // Save minimal cut sets of analyzed modules.
  std::map<int, std::vector< std::set<int> > > module_mcs;
  std::vector< std::set<int> >::iterator it;
  while (!mcs.empty()) {
    std::set<int> member = mcs.back();
    mcs.pop_back();
    if (*member.rbegin() < gate_index_) {
      imcs_.push_back(member);
    } else {
      std::set<int>::iterator it_s = member.end();
      --it_s;
      int module_index = *it_s;
      member.erase(it_s);
      std::vector< std::set<int> > sub_mcs;
      if (module_mcs.count(module_index)) {
        sub_mcs = module_mcs.find(module_index)->second;
      } else {
        LOG(DEBUG3) << "Finding MCS from module index: " << module_index;
        IndexedFaultTree::FindMcsFromSimpleGate(
            simple_gates.find(module_index)->second, &sub_mcs);
        module_mcs.insert(std::make_pair(module_index, sub_mcs));
      }
      std::vector< std::set<int> >::iterator it;
      for (it = sub_mcs.begin(); it != sub_mcs.end(); ++it) {
        if (it->size() + member.size() <= limit_order_) {
          it->insert(member.begin(), member.end());
          mcs.push_back(*it);
        }
      }
    }
  }

  // Special case of unity with empty sets.
  /// @todo Detect unity in modules.
  std::string state = indexed_gates_.find(top_event_index_)->second->state();
  assert(state != "unity");
  LOG(DEBUG2) << "The number of MCS found: " << imcs_.size();
  LOG(DEBUG2) << "Minimal cut set finding time: " << DUR(mcs_time);
}

void IndexedFaultTree::UnrollGates() {
  // Handle special case for a top event.
  IndexedGatePtr top_gate = indexed_gates_.find(top_event_index_)->second;
  std::string type = top_gate->string_type();
  assert(type != "undefined");
  if (type == "nor" || type == "or") {
    top_event_sign_ *= type == "nor" ? -1 : 1;  // For negative gates.
    top_gate->string_type("or");
    top_gate->type(1);
  } else if (type == "nand" || type == "and") {
    top_event_sign_ *= type == "nand" ? -1 : 1;
    top_gate->string_type("and");
    top_gate->type(2);
  } else if (type == "not" || type == "null") {
    // Change the top event to the negative child.
    assert(top_gate->children().size() == 1);
    int child_index = *top_gate->children().begin();
    assert(child_index > 0);
    top_gate = indexed_gates_.find(std::abs(child_index))->second;
    indexed_gates_.erase(top_event_index_);
    top_event_index_ = top_gate->index();
    top_event_sign_ *= type == "not" ? -1 : 1;  // The change for sign.
    IndexedFaultTree::UnrollGates();  // This should handle NOT->NOT cases.
    return;
  }
  // Gather parent information for negative gate processing.
  std::set<int> processed_gates;
  IndexedFaultTree::GatherParentInformation(top_gate, &processed_gates);
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
    IndexedFaultTree::UnrollGate(it->second);
  }
  // Note that parent information is invalid from this point.
}

void IndexedFaultTree::GatherParentInformation(
    const IndexedGatePtr& parent_gate,
    std::set<int>* processed_gates) {
  if (processed_gates->count(parent_gate->index())) return;
  processed_gates->insert(parent_gate->index());

  std::set<int>::const_iterator it;
  for (it = parent_gate->children().begin();
       it != parent_gate->children().end(); ++it) {
    int index = std::abs(*it);
    if (index > gate_index_) {
      IndexedGatePtr child = indexed_gates_.find(index)->second;
      child->AddParent(parent_gate->index());
      IndexedFaultTree::GatherParentInformation(child, processed_gates);
    }
  }
}

void IndexedFaultTree::NotifyParentsOfNegativeGates(
    const IndexedGatePtr& gate) {
  std::string type = gate->string_type();
  assert(type != "undefined");
  // Deal with negative gate.
  if (type == "nor" || type == "nand") {
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

void IndexedFaultTree::UnrollGate(const IndexedGatePtr& gate) {
  std::string type = gate->string_type();
  assert(type != "undefined");
  if (type == "or" || type == "nor") {
    gate->string_type("or");  // Negative is already processed.
    gate->type(1);
  } else if (type == "and" || type == "nand") {
    gate->type(2);
    gate->string_type("and");  // Negative is already processed.
  } else if (type == "xor") {
    IndexedFaultTree::UnrollXorGate(gate);
  } else if (type == "atleast") {
    IndexedFaultTree::UnrollAtleastGate(gate);
  } else {
    assert(type == "not" || type == "null");  // Dealt in gate joining.
  }
}

void IndexedFaultTree::UnrollXorGate(const IndexedGatePtr& gate) {
  assert(gate->children().size() == 2);
  std::set<int>::const_iterator it = gate->children().begin();
  IndexedGatePtr gate_one(new IndexedGate(++new_gate_index_));
  IndexedGatePtr gate_two(new IndexedGate(++new_gate_index_));

  gate->type(1);
  gate->string_type("or");
  gate_one->type(2);
  gate_two->type(2);
  gate_one->string_type("and");
  gate_two->string_type("and");
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

void IndexedFaultTree::UnrollAtleastGate(const IndexedGatePtr& gate) {
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

  gate->type(1);
  gate->string_type("or");
  gate->EraseAllChildren();
  std::set< std::set<int> >::iterator it_sets;
  for (it_sets = all_sets.begin(); it_sets != all_sets.end(); ++it_sets) {
    IndexedGatePtr gate_one(new IndexedGate(++new_gate_index_));
    gate_one->type(2);
    gate_one->string_type("and");
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
    const IndexedGatePtr& gate,
    std::set<int>* processed_gates) {
  if (processed_gates->count(gate->index())) return;
  processed_gates->insert(gate->index());
  // True house event in AND gate is removed.
  // False house event in AND gate makes the gate NULL.
  // True house event in OR gate makes the gate Unity, and it shouldn't appear
  // in minimal cut sets.
  // False house event in OR gate is removed.
  // Unity may occur due to House event.
  // Null can be due to house events or complement elments.
  std::set<int>::const_iterator it;
  std::vector<int> to_erase;  // Children to erase.
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    bool state = false;  // Null or Unity case. Null indication by default.
    if (std::abs(*it) > gate_index_) {  // Processing a gate.
      IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
      IndexedFaultTree::PropagateConstants(true_house_events,
                                           false_house_events,
                                           child_gate,
                                           processed_gates);
      std::string string_state = child_gate->state();
      assert(string_state == "normal" || string_state == "null" ||
             string_state == "unity");
      if (string_state == "normal") continue;
      state = string_state == "null" ? false : true;
    } else {  // Processing a primary event.
      if (false_house_events.count(std::abs(*it))) {
        state = false;
      } else if (true_house_events.count(std::abs(*it))) {
        state = true;
      } else {
        continue;  // This must be a basic event.
      }
    }
    if (*it < 0) state = !state;  // Complement event.

    if (IndexedFaultTree::ProcessConstantChild(gate, *it, state, &to_erase))
      return;
  }
  IndexedFaultTree::RemoveChildren(gate, to_erase);
}

bool IndexedFaultTree::ProcessConstantChild(const IndexedGatePtr& gate,
                                            int child,
                                            bool state,
                                            std::vector<int>* to_erase) {
  std::string parent_type = gate->string_type();
  assert(parent_type == "or" || parent_type == "and" ||
         parent_type == "not" || parent_type == "null");

  if (!state) {  // Null state.
    if (parent_type == "or") {
      to_erase->push_back(child);
      return false;

    } else if (parent_type == "and" || parent_type == "null") {
      // AND gate with null child.
      gate->Nullify();

    } else if (parent_type == "not") {
      gate->MakeUnity();
    }
  } else {  // Unity state.
    if (parent_type == "or") {
      gate->MakeUnity();

    } else if (parent_type == "and" || parent_type == "null") {
      to_erase->push_back(child);
      return false;

    } else if (parent_type == "not") {
      gate->Nullify();
    }
  }
  return true;  // Becomes constant most of the time or cases.
}

void IndexedFaultTree::RemoveChildren(const IndexedGatePtr& gate,
                                      const std::vector<int>& to_erase) {
  std::vector<int>::const_iterator it_v;
  for (it_v = to_erase.begin(); it_v != to_erase.end(); ++it_v) {
    gate->EraseChild(*it_v);
  }
  if (gate->children().empty()) {
    if (gate->string_type() == "or") {
      gate->Nullify();
    } else {  // The default operation for AND gate.
      gate->MakeUnity();
    }
  }
}

void IndexedFaultTree::PropagateComplements(
    const IndexedGatePtr& gate,
    std::map<int, int>* gate_complements,
    std::set<int>* processed_gates) {
  // If the child gate is complement, then create a new gate that propagates
  // its sign to its children and itself becomes non-complement.
  // Keep track of complement gates for optimization of repeated complements.
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end();) {
    if (std::abs(*it) > gate_index_) {
      // Deal with NOT and NULL gates.
      IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
      if (child_gate->string_type() == "not" ||
          child_gate->string_type() == "null") {
        assert(child_gate->children().size() == 1);
        int mult = child_gate->string_type() == "not" ? -1 : 1;
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
          IndexedGatePtr complement_gate(new IndexedGate(++new_gate_index_));
          indexed_gates_.insert(std::make_pair(complement_gate->index(),
                                               complement_gate));
          gate_complements->insert(std::make_pair(-*it,
                                                  complement_gate->index()));
          int existing_type = indexed_gates_.find(-*it)->second->type();
          complement_gate->type(existing_type == 1 ? 2 : 1);
          complement_gate->children(
              indexed_gates_.find(-*it)->second->children());
          complement_gate->InvertChildren();
          gate->SwapChild(*it, complement_gate->index());
          processed_gates->insert(complement_gate->index());
          IndexedFaultTree::PropagateComplements(complement_gate,
                                                 gate_complements,
                                                 processed_gates);
        }
        // Note that the iterator is invalid now.
        it = gate->children().begin();  // The negative gates at the start.
        continue;
      } else if (!processed_gates->count(*it)) {
        // Continue with the positive gate children.
        processed_gates->insert(*it);
        IndexedFaultTree::PropagateComplements(indexed_gates_.find(*it)->second,
                                               gate_complements,
                                               processed_gates);
      }
    }
    ++it;
  }
}

bool IndexedFaultTree::ProcessConstGates(const IndexedGatePtr& gate,
                                         std::set<int>* processed_gates) {
  // Null state gates' parent: OR->Remove the child and AND->NULL the parent.
  // Unity state gates' parent: OR->Unity the parent and AND->Remove the child.
  // The tree structure is only positive AND and OR gates.
  if (processed_gates->count(gate->index())) return false;
  processed_gates->insert(gate->index());

  if (gate->state() == "null" || gate->state() == "unity") return false;
  bool changed = false;  // Indication if this operation changed the gate.
  std::vector<int> to_erase;  // Keep track of children to erase.
  int type = gate->type();  // Only two types are possible, 1 or 2.
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    if (std::abs(*it) > gate_index_) {
      assert(*it > 0);
      IndexedGatePtr child_gate = indexed_gates_.find(*it)->second;
      bool ret  =
          IndexedFaultTree::ProcessConstGates(child_gate, processed_gates);
      if (!changed && ret) changed = true;
      std::string state = child_gate->state();
      if (state == "normal") continue;  // Only three states are possible.
      if (IndexedFaultTree::ProcessConstantChild(
              gate,
              *it,
              state == "null" ? false : true,
              &to_erase))
        return true;
    }
  }
  if (!changed && !to_erase.empty()) changed = true;
  IndexedFaultTree::RemoveChildren(gate, to_erase);
  return changed;
}

bool IndexedFaultTree::JoinGates(const IndexedGatePtr& gate,
                                 std::set<int>* processed_gates) {
  if (processed_gates->count(gate->index())) return false;
  processed_gates->insert(gate->index());
  int parent = gate->type();
  std::set<int>::const_iterator it;
  bool changed = false;  // Indication if the tree is changed.
  for (it = gate->children().begin(); it != gate->children().end();) {
    if (std::abs(*it) > gate_index_) {
      assert(*it > 0);
      IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
      int child = child_gate->type();
      if (parent == child) {  // Parent is not NULL or NOT.
        if (!changed) changed = true;
        if (!gate->MergeGate(&*indexed_gates_.find(*it)->second)) {
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
        bool ret = IndexedFaultTree::JoinGates(child_gate, processed_gates);
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

  IndexedGatePtr top_gate = indexed_gates_.find(top_event_index_)->second;
  int time = 0;
  IndexedFaultTree::AssignTiming(time, top_gate, visit_basics);

  LOG(DEBUG3) << "Timings are assigned to nodes.";

  std::map<int, std::pair<int, int> > visited_gates;
  IndexedFaultTree::FindOriginalModules(top_gate, visit_basics,
                                        &visited_gates);
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
  IndexedGatePtr new_module(new IndexedGate(++new_gate_index_));
  indexed_gates_.insert(std::make_pair(new_gate_index_, new_module));
  modules_.insert(new_gate_index_);
  new_module->type(gate->type());
  new_module->string_type(gate->string_type());
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

void IndexedFaultTree::CreateSimpleTree(
    int gate_index,
    std::map<int, SimpleGatePtr>* processed_gates) {
  assert(gate_index > 0);
  if (processed_gates->count(gate_index)) return;
  IndexedGatePtr gate = indexed_gates_.find(gate_index)->second;
  SimpleGatePtr simple_gate(new SimpleGate(gate->type()));
  processed_gates->insert(std::make_pair(gate_index, simple_gate));

  std::set<int>::iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    if (*it > gate_index_) {
      if (modules_.count(*it)) {
        simple_gate->InitiateWithModule(*it);
        IndexedFaultTree::CreateSimpleTree(*it, processed_gates);
      } else {
        IndexedFaultTree::CreateSimpleTree(*it, processed_gates);
        simple_gate->AddChildGate(processed_gates->find(*it)->second);
      }
    } else {
      assert(std::abs(*it) < gate_index_);  // No negative gates.
      simple_gate->InitiateWithBasic(*it);
    }
  }
}

void IndexedFaultTree::FindMcsFromSimpleGate(
    const SimpleGatePtr& gate,
    std::vector< std::set<int> >* mcs) {
  CLOCK(gen_time);

  SetPtrComp comp;
  std::set<SetPtr, SetPtrComp> cut_sets(comp);
  SetPtr cut_set(new std::set<int>);  // Initial empty cut set.
  // Generate main minimal cut set gates from top module.
  gate->GenerateCutSets(cut_set, &cut_sets);

  LOG(DEBUG4) << "Unique cut sets generated: " << cut_sets.size();
  LOG(DEBUG4) << "Cut set generation time: " << DUR(gen_time);

  CLOCK(min_time);
  LOG(DEBUG4) << "Minimizing the cut sets.";

  std::vector<const std::set<int>* > cut_sets_vector;
  cut_sets_vector.reserve(cut_sets.size());
  std::set<SetPtr, SetPtrComp>::iterator it;
  for (it = cut_sets.begin(); it != cut_sets.end(); ++it) {
    assert(!(*it)->empty());
    if ((*it)->size() == 1) {
      mcs->push_back(**it);
    } else {
      cut_sets_vector.push_back(&**it);
    }
  }
  IndexedFaultTree::MinimizeCutSets(cut_sets_vector, *mcs, 2, mcs);

  LOG(DEBUG4) << "The number of local MCS: " << mcs->size();
  LOG(DEBUG4) << "Cut set minimization time: " << DUR(min_time);
}

void IndexedFaultTree::MinimizeCutSets(
    const std::vector<const std::set<int>* >& cut_sets,
    const std::vector<std::set<int> >& mcs_lower_order,
    int min_order,
    std::vector<std::set<int> >* mcs) {
  if (cut_sets.empty()) return;

  std::vector<const std::set<int>* > temp_sets;  // For mcs of a level above.
  std::vector<std::set<int> > temp_min_sets;  // For mcs of this level.

  std::vector<const std::set<int>* >::const_iterator it_uniq;
  for (it_uniq = cut_sets.begin(); it_uniq != cut_sets.end(); ++it_uniq) {
    bool include = true;  // Determine to keep or not.

    std::vector<std::set<int> >::const_iterator it_min;
    for (it_min = mcs_lower_order.begin(); it_min != mcs_lower_order.end();
         ++it_min) {
      if (std::includes((*it_uniq)->begin(), (*it_uniq)->end(),
                        it_min->begin(), it_min->end())) {
        // Non-minimal cut set is detected.
        include = false;
        break;
      }
    }
    // After checking for non-minimal cut sets,
    // all minimum sized cut sets are guaranteed to be minimal.
    if (include) {
      if ((*it_uniq)->size() == min_order) {
        temp_min_sets.push_back(**it_uniq);
      } else {
        temp_sets.push_back(*it_uniq);
      }
    }
    // Ignore the cut set because include = false.
  }
  mcs->insert(mcs->end(), temp_min_sets.begin(), temp_min_sets.end());
  min_order++;
  IndexedFaultTree::MinimizeCutSets(temp_sets, temp_min_sets, min_order, mcs);
}

}  // namespace scram
