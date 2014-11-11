/// @file indexed_fault_tree.cc
/// Implementation of IndexedFaultTree class.
#include "indexed_fault_tree.h"

#include <ctime>

#include "event.h"
#include "indexed_gate.h"
#include "logger.h"

namespace scram {

IndexedFaultTree::IndexedFaultTree(int top_event_id, int limit_order)
    : top_event_index_(top_event_id),
      gate_index_(top_event_id),
      new_gate_index_(0),
      limit_order_(limit_order),
      changed_tree_(false),
      top_event_sign_(1) {}

void IndexedFaultTree::InitiateIndexedFaultTree(
    const boost::unordered_map<int, GatePtr>& int_to_inter,
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
      gate->InitiateWithChild(all_to_int.find(it_children->first)->second);
    }
    indexed_gates_.insert(std::make_pair(it->first, gate));
    if (gate->index() > new_gate_index_) new_gate_index_ = gate->index() + 1;
  }

  LOG() << "Unrolling basic gates.";
  assert(top_event_sign_ == 1);
  IndexedFaultTree::UnrollGates();
  LOG() << "Finished unrolling gates.";
}

void IndexedFaultTree::PropagateConstants(
    const std::set<int>& true_house_events,
    const std::set<int>& false_house_events) {
  IndexedGatePtr top = indexed_gates_.find(top_event_index_)->second;
  std::set<int> processed_gates;
  LOG() << "Propagating constants in a fault tree.";
  IndexedFaultTree::PropagateConstants(true_house_events, false_house_events,
                                       top, &processed_gates);
  LOG() << "Constant propagation is done.";
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
  do {
    changed_tree_ = false;
    // Cleanup null and unity gates. There is no negative gate.
    processed_gates.clear();
    IndexedFaultTree::ProcessConstGates(top, &processed_gates);
    // After this point there should not be any negative gates.
    processed_gates.clear();
    IndexedFaultTree::JoinGates(top, &processed_gates);
  } while (changed_tree_);
  // After this point there should not be null AND or unity OR gates,
  // and the tree structure should be repeating OR and AND.
  // All gates are positive, and each gate has atleast two children.
  if (top->children().empty()) return;  // This is null or unity.
  // Detect original modules for processing.
  IndexedFaultTree::DetectModules(num_basic_events);
}

void IndexedFaultTree::FindMcs() {
  // It is assumed that the tree is layered with OR and AND gates on each
  // level. That is, one level contains only AND or OR gates.
  // AND gates are operated; whereas, OR gates are left for later minimal
  // cut set finding. This operations make a big tree consisting of
  // only OR gates. The function assumes the tree contains only positive gates.
  std::clock_t start_time;
  start_time = std::clock();

  LOG() << "Start minimal cut set generation.";

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

  std::vector<SimpleGatePtr> min_gates;  // This AND gates are minimal.
  // Generate main minimal cut set gates from top module.
  IndexedFaultTree::FindMcsFromModule(top_event_index_, &min_gates);

  // Container of already processed modules. Note that the sign of
  // indices matter because positive module is not the same as negative.
  // Top module is not expected to re-occur in minimal cut sets.
  std::map<int, std::vector<SimpleGatePtr> > processed_modules;

  while(!min_gates.empty()) {
    SimpleGatePtr cut_set = min_gates.back();
    min_gates.pop_back();
    assert(cut_set->gates().empty());

    if (cut_set->modules().empty()) {
      imcs_.push_back(cut_set->basic_events());

    } else {
      /// @todo Optimize to detect already expanded modules to get in first.
      /// @todo Optimize to handle faster modules with basic events only.
      std::vector<SimpleGatePtr> new_sets;
      SimpleGatePtr proto_set(new SimpleGate(2));
      proto_set->basic_events(cut_set->basic_events());
      new_sets.push_back(proto_set);
      std::set<int>::const_iterator it;
      for (it = cut_set->modules().begin(); it != cut_set->modules().end();
           ++it) {
        assert(*it > 0);
        if (!processed_modules.count(*it)) {
          std::vector<SimpleGatePtr> module_mcs;  // For new MCS of a module.
          IndexedFaultTree::FindMcsFromModule(*it, &module_mcs);
          processed_modules.insert(std::make_pair(*it, module_mcs));
        }
        std::vector<SimpleGatePtr>* module_mcs =
            &processed_modules.find(*it)->second;
        if (module_mcs->empty()) continue;  // This is a null set.
        /// @todo Treat Unity cases.
        std::vector<SimpleGatePtr> joined_sets;
        std::vector<SimpleGatePtr>::const_iterator it_n;
        for (it_n = new_sets.begin(); it_n != new_sets.end(); ++it_n) {
          std::vector<SimpleGatePtr>::iterator it_m;
          for (it_m = module_mcs->begin(); it_m != module_mcs->end();
               ++it_m) {
            if (((*it_n)->basic_events().size() +
                 (*it_m)->basic_events().size()) <= limit_order_) {
              SimpleGatePtr new_set(new SimpleGate(**it_n));
              new_set->JoinAsMcs(*it_m);
              joined_sets.push_back(new_set);
            }
          }
        }
        new_sets = joined_sets;
      }
      /// @todo Optimize to detect basic-events-only cut sets earlier.
      min_gates.insert(min_gates.end(), new_sets.begin(), new_sets.end());
    }
  }

  // Special case of unity with empty sets.
  std::string state = indexed_gates_.find(top_event_index_)->second->state();
  if (imcs_.empty() && state == "unity") {
    std::set<int> empty_set;
    imcs_.push_back(empty_set);
  }
  double mcs_time = (std::clock() - start_time) /
      static_cast<double>(CLOCKS_PER_SEC);
  LOG() << "The number of MCS found: " << imcs_.size();
  LOG() << "Minimal cut set finding time: " << mcs_time;
}

void IndexedFaultTree::UnrollGates() {
  // Handle spacial case for a top event.
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
  } else if (type == "xor") {
    IndexedFaultTree::UnrollXorGate(top_gate);
  } else if (type == "atleast") {
    IndexedFaultTree::UnrollAtleastGate(top_gate);
  } else {
    assert(false);
  }
  // Gather parent information for negative gate processing.
  std::set<int> processed_gates;
  IndexedFaultTree::GatherParentInformation(top_gate, &processed_gates);
  // Process negative gates except for NOT. Note that top event's negative
  // gate is processed in the above lines.
  boost::unordered_map<int, IndexedGatePtr>::iterator it;
  for (it = indexed_gates_.begin(); it != indexed_gates_.end(); ++it) {
    if (it->first == top_event_index_) continue;
    IndexedFaultTree::NotifyParentsOfNegativeGates(it->second);
  }

  // Assumes that all gates are in indexed_gates_ container.
  boost::unordered_map<int, IndexedGatePtr> original_gates(indexed_gates_);
  for (it = original_gates.begin(); it != original_gates.end(); ++it) {
    if (it->first == top_event_index_) continue;
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
      if (parent->children().count(child_index)) {  // Positive child.
        bool ret = parent->SwapChild(child_index, -child_index);
        assert(ret);
      } else {  // Negative child.
        bool ret = parent->SwapChild(-child_index, child_index);
        assert(ret);
      }
    }
  }
}

void IndexedFaultTree::UnrollGate(IndexedGatePtr& gate) {
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

void IndexedFaultTree::UnrollXorGate(IndexedGatePtr& gate) {
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

void IndexedFaultTree::UnrollAtleastGate(IndexedGatePtr& gate) {
  int vote_number = gate->vote_number();

  assert(vote_number > 1);
  assert(gate->children().size() > vote_number);
  std::set< std::set<int> > all_sets;
  const std::set<int>* children = &gate->children();
  int size = children->size();

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
    IndexedGatePtr& gate,
    std::set<int>* processed_gates) {
  if (processed_gates->count(gate->index())) return;
  processed_gates->insert(gate->index());
  // True house event in AND gate is removed.
  // False house event in AND gate makes the gate NULL.
  // True house event in OR gate makes the gate Unity, and it shouldn't appear
  // in minimal cut sets.
  // False house event in OR gate is removed.
  // Unity must be only due to House event.
  // Null can be due to house events or complement elments.
  // True and false house events are treated as well for XOR and ATLEAST gates.
  std::set<int>::const_iterator it;
  /// @todo This may have bad behavior due to erased children. Needs more
  ///       testing and optimization. The function needs simplification.
  for (it = gate->children().begin(); it != gate->children().end();) {
    bool state = false;  // Null or Unity case.
    if (std::abs(*it) > gate_index_) {  // Processing a gate.
      IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
      PropagateConstants(true_house_events, false_house_events, child_gate,
                         processed_gates);
      std::string string_state = child_gate->state();
      assert(string_state == "normal" || string_state == "null" ||
             string_state == "unity");
      if (string_state == "normal") {
        ++it;
        continue;
      } else if (string_state == "null") {
        state = *it > 0 ? false : true;
      } else if (string_state == "unity") {
        state = *it > 0 ? true : false;
      }
    } else {  // Processing a primary event.
      if (false_house_events.count(std::abs(*it))) {
        state = *it > 0 ? false : true;
      } else if (true_house_events.count(std::abs(*it))) {
        state = *it > 0 ? true : false;
      } else {
        ++it;
        continue;  // Not a house event.
      }
    }

    std::string parent_type = gate->string_type();
    assert(parent_type == "or" || parent_type == "and" ||
           parent_type == "not" || parent_type == "null");

    if (!state) {  // Null state.
      if (parent_type == "or") {
        gate->EraseChild(*it);  // OR gate with null child.
        if (gate->children().empty()) {
          gate->Nullify();
          return;
        }
        it = gate->children().begin();
        continue;
      } else if (parent_type == "and" || parent_type == "null") {
        // AND gate with null child.
        gate->Nullify();
        return;
      } else if (parent_type == "not") {
        gate->MakeUnity();
        return;
      }
    } else {  // Unity state.
      if (parent_type == "or") {
        gate->MakeUnity();
        return;
      } else if (parent_type == "and" || parent_type == "null") {
        gate->EraseChild(*it);
        if (gate->children().empty()) {
          gate->MakeUnity();
          return;
        }
        it = gate->children().begin();
        continue;
      } else if (parent_type == "not") {
        gate->Nullify();
        return;
      }
    }
  }
}

void IndexedFaultTree::PropagateComplements(
    IndexedGatePtr& gate,
    std::map<int, int>* gate_complements,
    std::set<int>* processed_gates) {
  // If the child gate is complement, then create a new gate that propagates
  // its sign to its children and itself becomes non-complement.
  // Keep track of complement gates for optimization of repeted complements.
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

void IndexedFaultTree::ProcessConstGates(IndexedGatePtr& gate,
                                         std::set<int>* processed_gates) {
  // Null state gates' parent: OR->Remove the child and AND->NULL the parent.
  // Unity state gates' parent: OR->Unity the parent and AND->Remove the child.
  // The tree structure is only AND and OR gates.
  if (processed_gates->count(gate->index())) return;
  processed_gates->insert(gate->index());

  if (gate->state() == "null" || gate->state() == "unity") return;

  if (gate->type() == 1) {
    assert(gate->state() != "null");
    std::set<int>::const_iterator it;
    for (it = gate->children().begin(); it != gate->children().end();) {
      if (std::abs(*it) > gate_index_) {
        assert(*it > 0);
        IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
        IndexedFaultTree::ProcessConstGates(child_gate, processed_gates);
        std::string state = child_gate->state();
        if (state == "null") {
          gate->EraseChild(*it);
          if (gate->children().empty()) {
            gate->Nullify();
            return;
          }
          it = gate->children().begin();
          continue;
        } else if (state == "unity") {
          gate->MakeUnity();
          return;
        }
      }
      ++it;
    }
  } else {  // AND gate.
    std::set<int>::const_iterator it;
    for (it = gate->children().begin(); it != gate->children().end();) {
      if (std::abs(*it) > gate_index_) {
        assert(*it > 0);
        IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
        IndexedFaultTree::ProcessConstGates(child_gate, processed_gates);
        std::string state = child_gate->state();
        if (state == "null") {
          gate->Nullify();
          return;
        } else if (state == "unity") {
          gate->EraseChild(*it);
          if (gate->children().empty()) {
            gate->MakeUnity();
            return;
          }
          it = gate->children().begin();
          continue;
        }
      }
      ++it;
    }
  }
}

void IndexedFaultTree::JoinGates(IndexedGatePtr& gate,
                                 std::set<int>* processed_gates) {
  if (processed_gates->count(gate->index())) return;
  processed_gates->insert(gate->index());
  int parent = gate->type();
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end();) {
    if (std::abs(*it) > gate_index_) {
      assert(*it > 0);
      IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
      int child = child_gate->type();
      if (parent == child) {  // Parent is not NULL or NOT.
        changed_tree_ = true;
        if (!gate->MergeGate(&*indexed_gates_.find(*it)->second)) {
          break;
        } else {
          it = gate->children().begin();
          continue;
        }
      } else if (child_gate->children().size() == 1) {
        // This must be from some reduced gate after constant propagation.
        changed_tree_ = true;
        if (!gate->SwapChild(*it, *child_gate->children().begin()))
          break;
        it = gate->children().begin();
        continue;
      } else {
        IndexedFaultTree::JoinGates(child_gate, processed_gates);
      }
    }
    ++it;
  }
}

void IndexedFaultTree::DetectModules(int num_basic_events) {
  // At this stage only AND/OR gates are present.
  // All one element gates and non-coherent gates are converted and processed.
  // All constants are propagated and there is only gates and basic events.
  // First stage, traverse the tree depth-first for gates and indicate
  // visit time for each node.

  LOG() << "Detecting modules in a fault tree.";

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

  LOG() << "Timings are assigned to nodes.";

  int min_time = 0;
  int max_time = 0;
  std::map<int, std::pair<int, int> > visited_gates;
  IndexedFaultTree::FindOriginalModules(top_gate, visit_basics,
                                        &visited_gates,
                                        &min_time, &max_time);
  assert(min_time == 1);
  assert(max_time == top_gate->visits()[2]);

  int orig_mod = modules_.size();
  LOG() << "Detected number of original modules: " << modules_.size();

  std::set<int> visited_gates_new;
  IndexedFaultTree::CreateNewModules(visit_basics, top_gate,
                                     &visited_gates_new);
  LOG() << "The number of new modules created: " << modules_.size() - orig_mod;
}

int IndexedFaultTree::AssignTiming(int time, IndexedGatePtr& gate,
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
    IndexedGatePtr& gate,
    const int visit_basics[][2],
    std::map<int, std::pair<int, int> >* visited_gates,
    int* min_time,
    int* max_time) {
  /// @todo This must get optimized if needed.
  if (visited_gates->count(gate->index())) {
    *min_time = visited_gates->find(gate->index())->second.first;
    *max_time = visited_gates->find(gate->index())->second.second;
    return;
  }
  *min_time = gate->visits()[0];
  *max_time = gate->visits()[1];

  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    int index = std::abs(*it);
    int min = 0;
    int max = 0;
    if (index < top_event_index_) {
      min = visit_basics[index][0];
      max = visit_basics[index][1];

    } else {
      IndexedFaultTree::FindOriginalModules(indexed_gates_.find(index)->second,
                                            visit_basics, visited_gates,
                                            &min, &max);
    }
    assert(min != 0);
    assert(max != 0);
    if (min < *min_time) *min_time = min;
    if (max > *max_time) *max_time = max;
  }

  // Determine if this gate is module itself.
  if (*min_time == gate->visits()[0] && *max_time == gate->visits()[1]) {
    modules_.insert(gate->index());
  }
  if (gate->visits()[2] > *max_time) *max_time = gate->visits()[2];
  visited_gates->insert(std::make_pair(gate->index(),
                                       std::make_pair(*min_time, *max_time)));
}

void IndexedFaultTree::CreateNewModules(const int visit_basics[][2],
                                        IndexedGatePtr& gate,
                                        std::set<int>* visited_gates) {
  if (visited_gates->count(gate->index())) return;
  visited_gates->insert(gate->index());
  // Children that can be grouped in a module.
  std::vector<int> modular_children;
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    if (std::abs(*it) > gate_index_) {
      assert(*it > 0);
      IndexedGatePtr child_gate = indexed_gates_.find(std::abs(*it))->second;
      IndexedFaultTree::CreateNewModules(visit_basics, child_gate,
                                         visited_gates);
      if (!modules_.count(std::abs(*it))) continue;
      if ((gate->visits()[1] > child_gate->visits()[2]) &&
          (gate->visits()[0] < child_gate->visits()[0])) {
        modular_children.push_back(*it);
      }
    } else if ((visit_basics[std::abs(*it)][0] ==
                visit_basics[std::abs(*it)][1]) &&
               (gate->visits()[1] > visit_basics[std::abs(*it)][1]) &&
               (gate->visits()[0] < visit_basics[std::abs(*it)][0])) {
      modular_children.push_back(*it);
    }
  }
  // Check if this gate is pure module itself.
  // That is, its children are all modules themselves and not shared with
  // other gates.
  if (modular_children.size() == gate->children().size()) return;
  if (modular_children.size() > 1) {
    IndexedGatePtr new_module(new IndexedGate(++new_gate_index_));
    indexed_gates_.insert(std::make_pair(new_gate_index_, new_module));
    modules_.insert(new_gate_index_);
    std::vector<int>::iterator it_g;
    for (it_g = modular_children.begin(); it_g != modular_children.end();
         ++it_g) {
      gate->EraseChild(*it_g);
      new_module->InitiateWithChild(*it_g);
    }
    new_module->type(gate->type());
    new_module->string_type(gate->string_type());
    gate->InitiateWithChild(new_module->index());
  }
}

void IndexedFaultTree::FindMcsFromModule(
    int index,
    std::vector<SimpleGatePtr>* min_gates) {
  std::clock_t start_time;
  start_time = std::clock();

  LOG() << "IndexedFaultTree: Finding MCS from module: " << index;

  assert(min_gates->empty());
  assert(index > 0);
  IndexedGatePtr gate = indexed_gates_.find(std::abs(index))->second;
  assert(!gate->children().empty());

  // Upon unrolling the tree, the top event may be detected to be complement.
  // This fact is processed before giving the top event to complement
  // propagation function.
  std::map<int, SimpleGatePtr> processed_simple_gates;
  SimpleGatePtr simple_gate =
      IndexedFaultTree::CreateSimpleTree(std::abs(index),
                                         &processed_simple_gates);

  // Duration of simple tree generation.
  double simple_tree_time = (std::clock() - start_time) /
      static_cast<double>(CLOCKS_PER_SEC);
  LOG() << "Simple tree creation time: " << simple_tree_time;

  IndexedFaultTree::FindMcsFromSimpleGate(simple_gate, min_gates);
}

boost::shared_ptr<SimpleGate> IndexedFaultTree::CreateSimpleTree(
    int gate_index,
    std::map<int, SimpleGatePtr>* processed_gates) {
  assert(gate_index > 0);
  if (processed_gates->count(gate_index))
    return processed_gates->find(gate_index)->second;
  IndexedGatePtr gate = indexed_gates_.find(gate_index)->second;
  SimpleGatePtr simple_gate(new SimpleGate(gate->type()));
  processed_gates->insert(std::make_pair(gate_index, simple_gate));

  std::set<int>::iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    if (*it > gate_index_) {
      if (modules_.count(*it)) {
        simple_gate->InitiateWithModule(*it);
      } else {
        simple_gate->AddChildGate(
            IndexedFaultTree::CreateSimpleTree(*it, processed_gates));
      }
    } else {
      assert(std::abs(*it) < gate_index_);  // No negative gates.
      simple_gate->InitiateWithBasic(*it);
    }
  }
  return simple_gate;
}

void IndexedFaultTree::FindMcsFromSimpleGate(
    SimpleGatePtr& gate,
    std::vector<SimpleGatePtr>* min_gates) {
  std::clock_t start_time;
  start_time = std::clock();

  // Expanding AND gate with basic event children and OR gate children.
  if (gate->basic_events().empty() && gate->gates().empty() &&
      gate->modules().empty()) return;
  LOG() << "IndexedFaultTree: Finding MCS for non-empty gate.";
  if (gate->type() == 2) {
    if (gate->basic_events().size() > limit_order_) {
      return;  // No cut set generation for this level.
    } else if (gate->gates().empty()) {
      // The special case of the gate is only cut set.
      /// @todo This may be removed by early detection optimization or
      ///       reusing the passed gate as a minimal cut set.
      SimpleGatePtr only_set(new SimpleGate(*gate));
      min_gates->push_back(only_set);
      return;
    }
    IndexedFaultTree::ExpandAndLayer(gate);
  }
  std::vector<SimpleGatePtr> cut_sets;
  IndexedFaultTree::ExpandOrLayer(gate, &cut_sets);

  LOG() << "Non-Unique cut sets generated: " << cut_sets.size();
  double cut_sets_time = (std::clock() - start_time) /
      static_cast<double>(CLOCKS_PER_SEC);
  LOG() << "Cut set generation time: " << cut_sets_time;
  LOG() << "Top gate's gate children: " << gate->gates().size();

  // At this point cut sets must be generated.
  SetPtrComp comp;
  std::set<SimpleGatePtr, SetPtrComp> unique_cut_sets(comp);

  std::set<SimpleGatePtr, SetPtrComp>  one_element_sets;
  // Special case when top gate OR has basic events and modules.
  std::set<int>::iterator it_b;
  for (it_b = gate->basic_events().begin();
       it_b != gate->basic_events().end(); ++it_b) {
    SimpleGatePtr new_set(new SimpleGate(2));
    new_set->InitiateWithBasic(*it_b);
    one_element_sets.insert(one_element_sets.end(), new_set);
  }
  for (it_b = gate->modules().begin();
       it_b != gate->modules().end(); ++it_b) {
    SimpleGatePtr new_set(new SimpleGate(2));
    new_set->InitiateWithModule(*it_b);
    one_element_sets.insert(one_element_sets.end(), new_set);
  }
  std::vector<SimpleGatePtr>::const_iterator it;
  for (it = cut_sets.begin(); it != cut_sets.end(); ++it) {
    assert((*it)->type() == 2);
    assert((*it)->gates().empty());
    if ((*it)->basic_events().size() == 1 && (*it)->modules().empty()) {
      one_element_sets.insert(*it);
    } else if ((*it)->modules().size() == 1 && (*it)->basic_events().empty()) {
      one_element_sets.insert(*it);
    } else {
      unique_cut_sets.insert(*it);
    }
  }
  std::vector<SimpleGatePtr> sets_unique;
  std::set<SimpleGatePtr, SetPtrComp>::iterator it_un;
  for (it_un = unique_cut_sets.begin(); it_un != unique_cut_sets.end();
       ++it_un) {
    assert(!(*it_un)->modules().empty() || !(*it_un)->basic_events().empty());
    if ((*it_un)->basic_events().size() == 1 &&
        (*it_un)->modules().empty()) {
      one_element_sets.insert(*it);
    } else if ((*it_un)->modules().size() == 1 &&
               (*it_un)->basic_events().empty()) {
      one_element_sets.insert(*it);
    } else {
      sets_unique.push_back(*it_un);
    }
  }

  min_gates->reserve(sets_unique.size() + one_element_sets.size());
  std::set<SimpleGatePtr, SetPtrComp>::const_iterator it_s;
  for (it_s = one_element_sets.begin(); it_s != one_element_sets.end();
       ++it_s) {
    min_gates->push_back(*it_s);
  }

  LOG() << "Unique cut sets size: " << sets_unique.size();
  LOG() << "One element sets size: " << min_gates->size();

  LOG() << "IndexedFaultTree: Minimizing the cut sets.";
  IndexedFaultTree::MinimizeCutSets(sets_unique, *min_gates, 2, min_gates);
}

void IndexedFaultTree::ExpandOrLayer(SimpleGatePtr& gate,
                                     std::vector<SimpleGatePtr>* cut_sets) {
  if (gate->gates().empty()) return;
  std::vector<SimpleGatePtr> new_gates;
  std::set<SimpleGatePtr>::iterator it;
  for (it = gate->gates().begin(); it != gate->gates().end(); ++it) {
    if ((*it)->basic_events().size() > limit_order_) {
      continue;
    } else if ((*it)->gates().empty()) {
      cut_sets->push_back(*it);
      continue;  // This may leave some larger cut sets for top event.
    }
    SimpleGatePtr new_gate(new SimpleGate(**it));
    IndexedFaultTree::ExpandAndLayer(new_gate);  // The gate becomes OR.
    IndexedFaultTree::ExpandOrLayer(new_gate, cut_sets);
    new_gates.push_back(new_gate);
  }
  gate->gates().clear();
  gate->gates().insert(new_gates.begin(), new_gates.end());
}

void IndexedFaultTree::ExpandAndLayer(SimpleGatePtr& gate) {
  assert(gate->type() == 2);
  assert(gate->basic_events().size() <= limit_order_);
  assert(!gate->gates().empty());
  // Create a new gate with OR logic instead of AND.
  SimpleGatePtr substitute(new SimpleGate(1));

  // The starting basic events for expansion.
  SimpleGatePtr child(new SimpleGate(2));
  child->basic_events(gate->basic_events());
  child->modules(gate->modules());
  substitute->AddChildGate(child);

  // Processes underlying OR layer gates.
  std::set<SimpleGatePtr>::iterator it_v;
  for (it_v = gate->gates().begin(); it_v != gate->gates().end(); ++it_v) {
    assert((*it_v)->type() == 1);
    // Create new sets for multiplication.
    std::set<SimpleGatePtr> children = substitute->gates();
    substitute->gates().clear();  // Prepare for new children gates.
    std::set<SimpleGatePtr>::iterator it;
    for (it = children.begin(); it != children.end(); ++it) {
      // Add basic events.
      std::set<int>::const_iterator it_b;
      for (it_b = (*it_v)->basic_events().begin();
           it_b != (*it_v)->basic_events().end(); ++it_b) {
        SimpleGatePtr new_child(new SimpleGate(**it));
        if (new_child->AddBasic(*it_b) &&
            new_child->basic_events().size() <= limit_order_)
          substitute->AddChildGate(new_child);
      }
      // Add modules just like basic events.
      std::set<int>::const_iterator it_m;
      for (it_m = (*it_v)->modules().begin();
           it_m != (*it_v)->modules().end(); ++it_m) {
        SimpleGatePtr new_child(new SimpleGate(**it));
        new_child->AddModule(*it_m);
        substitute->AddChildGate(new_child);
      }
      // Join underlying AND layer gates.
      std::set<SimpleGatePtr>::iterator it_g;
      for (it_g = (*it_v)->gates().begin();
           it_g != (*it_v)->gates().end(); ++it_g) {
        SimpleGatePtr new_child(new SimpleGate(**it));
        if (new_child->MergeGate(*it_g) &&
            new_child->basic_events().size() <= limit_order_) {
          // This must be underlying AND layer.
          substitute->AddChildGate(new_child);
        }
      }
    }
  }
  gate = substitute;
}

void IndexedFaultTree::MinimizeCutSets(
    const std::vector<SimpleGatePtr>& cut_sets,
    const std::vector<SimpleGatePtr>& mcs_lower_order,
    int min_order,
    std::vector<SimpleGatePtr>* min_gates) {
  if (cut_sets.empty()) return;

  std::vector<SimpleGatePtr> temp_sets;  // For mcs of a level above.
  std::vector<SimpleGatePtr> temp_min_sets;  // For mcs of this level.

  std::vector<SimpleGatePtr>::const_iterator it_uniq;
  for (it_uniq = cut_sets.begin(); it_uniq != cut_sets.end(); ++it_uniq) {
    bool include = true;  // Determine to keep or not.

    std::vector<SimpleGatePtr>::const_iterator it_min;
    for (it_min = mcs_lower_order.begin(); it_min != mcs_lower_order.end();
         ++it_min) {
      if (std::includes((*it_uniq)->basic_events().begin(),
                        (*it_uniq)->basic_events().end(),
                        (*it_min)->basic_events().begin(),
                        (*it_min)->basic_events().end()) &&
          std::includes((*it_uniq)->modules().begin(),
                        (*it_uniq)->modules().end(),
                        (*it_min)->modules().begin(),
                        (*it_min)->modules().end())) {
        // Non-minimal cut set is detected.
        include = false;
        break;
      }
    }
    // After checking for non-minimal cut sets,
    // all minimum sized cut sets are guaranteed to be minimal.
    if (include) {
      if (((*it_uniq)->basic_events().size() + (*it_uniq)->modules().size()) ==
          min_order) {
        temp_min_sets.push_back(*it_uniq);
      } else {
        temp_sets.push_back(*it_uniq);
      }
    }
    // Ignore the cut set because include = false.
  }
  min_gates->insert(min_gates->end(), temp_min_sets.begin(),
                    temp_min_sets.end());
  min_order++;
  IndexedFaultTree::MinimizeCutSets(temp_sets, temp_min_sets, min_order,
                                    min_gates);
}

}  // namespace scram
