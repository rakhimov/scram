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
    new_gate_index_(0),
    limit_order_(limit_order),
    top_event_sign_(1) {}

IndexedFaultTree::~IndexedFaultTree() {
  boost::unordered_map<int, IndexedGate*>::iterator it;
  for (it = indexed_gates_.begin(); it != indexed_gates_.end(); ++it) {
    delete it->second;
  }
}

void IndexedFaultTree::InitiateIndexedFaultTree(
    const boost::unordered_map<int, GatePtr>& int_to_inter,
    const boost::unordered_map<std::string, int>& all_to_int) {
  boost::unordered_map<int, GatePtr>::const_iterator it;
  for (it = int_to_inter.begin(); it != int_to_inter.end(); ++it) {
    IndexedGate* gate = new IndexedGate(it->first);
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

  IndexedFaultTree::StartUnrollingGates();
}

void IndexedFaultTree::PropagateConstants(
    const std::set<int>& true_house_events,
    const std::set<int>& false_house_events) {
  IndexedGate* top = indexed_gates_.find(top_event_index_)->second;
  std::set<int> processed_gates;
  IndexedFaultTree::PropagateConstants(true_house_events, false_house_events,
                                       top, &processed_gates);
}

void IndexedFaultTree::ProcessIndexedFaultTree() {
  IndexedGate* top = indexed_gates_.find(top_event_index_)->second;
  // Upon unrolling the tree, the top event may be detected to be complement.
  // This fact is processed before giving the top event to complement
  // propagation function.
  if (top_event_sign_ < 0) {
    top->type(top->type() == 1 ? 2 : 1);
    top->InvertChildren();
  }
  std::map<int, int> complements;
  std::set<int> processed_gates;
  IndexedFaultTree::PropagateComplements(top, &complements, &processed_gates);
  // After this point there should not be any negative gates.

  processed_gates.clear();
  IndexedFaultTree::PreprocessTree(top, &processed_gates);
  // After this point there might be null AND gates, and the tree structure
  // should be repeating OR and AND.

  processed_gates.clear();
  IndexedFaultTree::ProcessNullGates(top, &processed_gates);
}

boost::shared_ptr<SimpleGate> IndexedFaultTree::CreateSimpleTree(
    int gate_index,
    std::map<int, SimpleGatePtr>* processed_gates) {
  if (processed_gates->count(gate_index))
    return processed_gates->find(gate_index)->second;
  IndexedGate* gate = indexed_gates_.find(gate_index)->second;
  SimpleGatePtr simple_gate(new SimpleGate(gate->type()));
  processed_gates->insert(std::make_pair(gate_index, simple_gate));

  std::set<int>::iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    if (*it < top_event_index_) {
      assert(std::abs(*it) < top_event_index_);  // No negative gates.
      simple_gate->InitiateWithBasic(*it);
    } else {
      simple_gate->AddChildGate(
          IndexedFaultTree::CreateSimpleTree(*it, processed_gates));
    }
  }
  return simple_gate;
}

void IndexedFaultTree::FindMcs() {
  // It is assumed that the tree is layered with OR and AND gates on each
  // level. That is, one level contains only AND or OR gates.
  // AND gates are operated; whereas, OR gates are left for later minimal
  // cut set finding. This operations make a big tree consisting of
  // only OR gates. The function assumes the tree contains only positive gates.
  std::clock_t start_time;
  start_time = std::clock();

  LOG() << "IndexedFaultTree: Start creation of simple gates";
  std::map<int, SimpleGatePtr> processed_gates;
  SimpleGatePtr top_gate =
      IndexedFaultTree::CreateSimpleTree(top_event_index_, &processed_gates);

  // Duration of simple tree generation.
  double simple_tree_time = (std::clock() - start_time) /
                            static_cast<double>(CLOCKS_PER_SEC);
  LOG() << "Simple tree creation time: " << simple_tree_time;

  // Expanding AND gate with basic event children and OR gate children.
  if (top_gate->basic_events().empty() && top_gate->gates().empty()) return;
  LOG() << "IndexedFaultTree: Finding MCS for non-empty gate.";
  if (top_gate->type() == 2) {
    if (top_gate->basic_events().size() > limit_order_) {
      return;  // No cut set generation for this level.
    } else if (top_gate->gates().empty()) {
      // The special case of top gate is only cut set.
      imcs_.push_back(top_gate->basic_events());
      return;
    }
    IndexedFaultTree::ExpandAndLayer(top_gate);
  }
  IndexedFaultTree::ExpandOrLayer(top_gate);

  LOG() << "IndexedFaultTree: Cut sets are generated.";
  double cut_sets_time = (std::clock() - start_time) /
                         static_cast<double>(CLOCKS_PER_SEC);
  LOG() << "Cut set generation time: " << cut_sets_time - simple_tree_time;
  LOG() << "Top gate's gate children: " << top_gate->gates().size();

  // At this point cut sets must be generated.
  SetPtrComp comp;
  std::set< const std::set<int>*, SetPtrComp > unique_cut_sets(comp);
  IndexedFaultTree::GatherCutSets(top_gate, &unique_cut_sets);

  imcs_.reserve(unique_cut_sets.size() + one_element_sets_.size());
  std::set< std::set<int> >::const_iterator it_s;
  std::vector< const std::set<int>* > sets_unique;
  std::set< const std::set<int>*, SetPtrComp >::iterator it_un;
  for (it_un = unique_cut_sets.begin(); it_un != unique_cut_sets.end();
       ++it_un) {
    assert(!(*it_un)->empty());
    if ((*it_un)->size() == 1) {
      one_element_sets_.insert(**it_un);
      continue;
    }
    sets_unique.push_back(*it_un);
  }

  for (it_s = one_element_sets_.begin(); it_s != one_element_sets_.end();
       ++it_s) {
    imcs_.push_back(*it_s);
  }

  LOG() << "Unique cut sets size: " << sets_unique.size();
  LOG() << "One element sets size: " << one_element_sets_.size();

  LOG() << "IndexedFaultTree: Minimizing the cut sets.";
  IndexedFaultTree::FindMcs(sets_unique, imcs_, 2, &imcs_);
  double mcs_time = (std::clock() - start_time) /
                    static_cast<double>(CLOCKS_PER_SEC);
  LOG() << "Minimal cut set finding time: " << mcs_time - cut_sets_time;
}

void IndexedFaultTree::ExpandOrLayer(SimpleGatePtr& gate) {
  if (gate->gates().empty()) return;
  std::vector<SimpleGatePtr> new_gates;
  std::set<SimpleGatePtr>::iterator it;
  for (it = gate->gates().begin(); it != gate->gates().end(); ++it) {
    if ((*it)->basic_events().size() > limit_order_) {
      continue;
    } else if ((*it)->gates().empty()) {
      new_gates.push_back(*it);
      continue;  // This may leave some larger cut sets for top event.
    }
    SimpleGatePtr new_gate(new SimpleGate(**it));
    IndexedFaultTree::ExpandAndLayer(new_gate);  // The gate becomes OR.
    IndexedFaultTree::ExpandOrLayer(new_gate);
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
  substitute->AddChildGate(child);

  std::set<SimpleGatePtr>::iterator it_v;
  for (it_v = gate->gates().begin(); it_v != gate->gates().end(); ++it_v) {
    assert((*it_v)->type() == 1);
    // Create new sets for multiplication.
    // Note that we already have one set of gates ready to be
    // cloned.
    std::set<SimpleGatePtr> children = substitute->gates();
    substitute->gates().clear();  // Prepare for new children gates.
    std::set<SimpleGatePtr>::iterator it;
    for (it = children.begin(); it != children.end(); ++it) {
      std::set<int>::const_iterator it_b;
      for (it_b = (*it_v)->basic_events().begin();
           it_b != (*it_v)->basic_events().end(); ++it_b) {
        SimpleGatePtr new_child(new SimpleGate(**it));
        if (new_child->AddBasic(*it_b) &&
            new_child->basic_events().size() <= limit_order_)
          substitute->AddChildGate(new_child);
      }
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

void IndexedFaultTree::GatherCutSets(
    const SimpleGatePtr& gate,
    std::set< const std::set<int>*, SetPtrComp >* unique_sets) {
  assert(gate->type() == 1);
  std::set<int>::iterator it_b;
  for (it_b = gate->basic_events().begin(); it_b != gate->basic_events().end();
       ++it_b) {
    std::set<int> one;
    one.insert(*it_b);
    one_element_sets_.insert(one);
  }
  std::set<SimpleGatePtr>::iterator it_g;
  for (it_g = gate->gates().begin(); it_g != gate->gates().end(); ++it_g) {
    if ((*it_g)->type() == 1) {
      IndexedFaultTree::GatherCutSets(*it_g, unique_sets);
    } else {
      assert((*it_g)->gates().empty());
      if ((*it_g)->basic_events().size() == 1)
        one_element_sets_.insert((*it_g)->basic_events());
      unique_sets->insert(&(*it_g)->basic_events());
    }
  }
}

void IndexedFaultTree::FindMcs(
    const std::vector< const std::set<int>* >& cut_sets,
    const std::vector< std::set<int> >& mcs_lower_order,
    int min_order,
    std::vector< std::set<int> >* imcs) {
  if (cut_sets.empty()) return;

  // Iterator for cut_sets.
  std::vector< const std::set<int>* >::const_iterator it_uniq;

  // Iterator for minimal cut sets.
  std::vector< std::set<int> >::const_iterator it_min;

  std::vector< const std::set<int>* > temp_sets;  // For mcs of a level above.
  std::vector< std::set<int> > temp_min_sets;  // For mcs of this level.

  for (it_uniq = cut_sets.begin();
       it_uniq != cut_sets.end(); ++it_uniq) {
    bool include = true;  // Determine to keep or not.

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
  imcs->insert(imcs->end(), temp_min_sets.begin(), temp_min_sets.end());
  min_order++;
  IndexedFaultTree::FindMcs(temp_sets, temp_min_sets, min_order, imcs);
}

void IndexedFaultTree::StartUnrollingGates() {
  // Handle spacial case for a negative or one-child top event.
  IndexedGate* top_gate = indexed_gates_.find(top_event_index_)->second;
  std::string type = top_gate->string_type();
  top_event_sign_ = 1;  // For positive gates.
  if (type == "nor") {
    top_gate->string_type("or");
    top_event_sign_ = -1;  // For negative gates.
  } else if (type == "nand") {
    top_gate->string_type("and");
    top_event_sign_ = -1;
  } else if (type == "not") {
    top_gate->string_type("and");
    top_event_sign_ = -1;
  } else if (type == "null") {
    top_gate->string_type("and");
    top_event_sign_ = 1;
  }
  IndexedFaultTree::UnrollTopGate(top_gate);
}

void IndexedFaultTree::UnrollTopGate(IndexedGate* top_gate) {
  std::string type = top_gate->string_type();
  assert(type != "finished");
  if (type == "or") {
    top_gate->type(1);
  } else if (type == "and") {
    top_gate->type(2);
  } else if (type == "xor") {
    IndexedFaultTree::UnrollXorGate(top_gate);
  } else if (type == "atleast") {
    IndexedFaultTree::UnrollAtleastGate(top_gate);
  } else {
    assert(false);
  }
  std::set<int> unrolled_gates;
  IndexedFaultTree::UnrollGates(top_gate, &unrolled_gates);
}

void IndexedFaultTree::UnrollGates(IndexedGate* parent_gate,
                                   std::set<int>* unrolled_gates) {
  std::set<int>::const_iterator it;
  for (it = parent_gate->children().begin();
       it != parent_gate->children().end();) {
    if (std::abs(*it) > top_event_index_ &&
        !unrolled_gates->count(std::abs(*it))) {
      IndexedGate* gate = indexed_gates_.find(std::abs(*it))->second;
      unrolled_gates->insert(gate->index());
      std::string type = gate->string_type();
      assert(type != "finished");
      if (type == "or") {
        gate->type(1);
      } else if (type == "and" || type == "null") {
        gate->type(2);
        gate->string_type("and");
      } else if (type == "not" || type == "nand") {
        gate->string_type("and");
        gate->type(2);
        int gate_index = *it;  // Might be negative.
        bool ret = parent_gate->SwapChild(gate_index, -gate_index);
        assert(ret);
        it = parent_gate->children().begin();
        continue;
      } else if (type == "nor") {
        gate->string_type("or");
        gate->type(1);
        int gate_index = *it;  // Might be negative.
        bool ret = parent_gate->SwapChild(gate_index, -gate_index);
        assert(ret);
        it = parent_gate->children().begin();
        continue;
      } else if (type == "xor") {
        IndexedFaultTree::UnrollXorGate(gate);
      } else if (type == "atleast") {
        IndexedFaultTree::UnrollAtleastGate(gate);
      } else {
        assert(false);
      }
      IndexedFaultTree::UnrollGates(gate, unrolled_gates);
    }
    ++it;
  }
}

void IndexedFaultTree::UnrollXorGate(IndexedGate* gate) {
  assert(gate->children().size() == 2);
  std::set<int>::const_iterator it = gate->children().begin();
  IndexedGate* gate_one = new IndexedGate(++new_gate_index_);
  IndexedGate* gate_two = new IndexedGate(++new_gate_index_);

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

void IndexedFaultTree::UnrollAtleastGate(IndexedGate* gate) {
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
    IndexedGate* gate_one = new IndexedGate(++new_gate_index_);
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
    IndexedGate* gate,
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
  std::set<int>::const_iterator it;
  int parent_type = gate->type();
  /// @todo This may have bad behavior due to erased children. Needs more
  ///       testing. The function needs simplification.
  for (it = gate->children().begin(); it != gate->children().end();) {
    if (std::abs(*it) > top_event_index_) {  // Processing a gate.
      IndexedGate* child_gate = indexed_gates_.find(std::abs(*it))->second;
      PropagateConstants(true_house_events, false_house_events, child_gate,
                         processed_gates);
      if (*it > 0) {
        if (child_gate->state() == "null") {
          if (parent_type == 1) {
            gate->EraseChild(*it);  // OR gate with null child.
            if (gate->children().empty()) {
              gate->Nullify();
              return;
            }
            it = gate->children().begin();
            continue;
          } else {
            // AND gate with null child.
            gate->Nullify();
            return;
          }
        } else if (child_gate->state() == "unity") {
          if (parent_type == 1) {
            gate->MakeUnity();
            return;
          } else {
            gate->EraseChild(*it);
            if (gate->children().empty()) {
              gate->MakeUnity();
              return;
            }
            it = gate->children().begin();
            continue;
          }
        }
      } else {
        if (child_gate->state() == "null") {
          if (parent_type == 1) {
            // Makes unity.
            gate->MakeUnity();
            return;
          } else {
            gate->EraseChild(*it);  // AND gate with unity child.
            if (gate->children().empty()) {
              gate->MakeUnity();
              return;
            }
            it = gate->children().begin();
            continue;
          }
        } else if (child_gate->state() == "unity") {
          if (parent_type == 1) {
            gate->EraseChild(*it);  // OR gate with null child.
            if (gate->children().empty()) {
              gate->Nullify();
              return;
            }
            it = gate->children().begin();
            continue;
          } else {
            // AND gate with null child.
            gate->Nullify();
            return;
          }
        }
      }
    } else {  // Process an event.
      if (*it > 0) {
        if (false_house_events.count(*it)) {
          if (parent_type == 1) {
            gate->EraseChild(*it);  // OR gate with false house child.
            if (gate->children().empty()) {
              gate->Nullify();
              return;
            }
            it = gate->children().begin();
            continue;
          } else {
            // AND gate with false child.
            gate->Nullify();
            return;
          }
        } else if (true_house_events.count(*it)) {
          if (parent_type == 1) {
            gate->MakeUnity();
            return;
          } else {
            gate->EraseChild(*it);
            if (gate->children().empty()) {
              gate->MakeUnity();
              return;
            }
            it = gate->children().begin();
            continue;
          }
        }
      } else {
        if (false_house_events.count(-*it)) {
          if (parent_type == 1) {
            gate->MakeUnity();
            return;
          } else {
            gate->EraseChild(*it);
            if (gate->children().empty()) {
              gate->MakeUnity();
              return;
            }
            it = gate->children().begin();
            continue;
          }
        } else if (true_house_events.count(-*it)) {
          if (parent_type == 1) {
            gate->EraseChild(*it);  // OR gate with false house child.
            if (gate->children().empty()) {
              gate->Nullify();
              return;
            }
            it = gate->children().begin();
            continue;
          } else {
            // AND gate with false child.
            gate->Nullify();
            return;
          }
        }
      }
    }
    ++it;
  }
}

void IndexedFaultTree::PropagateComplements(
    IndexedGate* gate,
    std::map<int, int>* gate_complements,
    std::set<int>* processed_gates) {
  // If the child gate is complement, then create a new gate that propagates
  // its sign to its children and itself becomes non-complement.
  // Keep track of complement gates for optimization of repeted complements.
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end();) {
    if (std::abs(*it) > top_event_index_) {
      if (*it < 0) {
        if (gate_complements->count(-*it)) {
          gate->SwapChild(*it, gate_complements->find(-*it)->second);
        } else {
          IndexedGate* complement_gate = new IndexedGate(++new_gate_index_);
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

void IndexedFaultTree::PreprocessTree(IndexedGate* gate,
                                      std::set<int>* processed_gates) {
  if (processed_gates->count(gate->index())) return;
  processed_gates->insert(gate->index());
  int parent = gate->type();
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end();) {
    if (std::abs(*it) > top_event_index_) {
      assert(*it > 0);  // All the gates must be positive.
      IndexedGate* child_gate = indexed_gates_.find(std::abs(*it))->second;
      int child = child_gate->type();
      if (parent == child) {
        if (!gate->MergeGate(indexed_gates_.find(*it)->second)) {
          break;
        } else {
          it = gate->children().begin();
          continue;
        }
      } else if (child_gate->children().size() == 1) {
        // This must be from null, not, or some reduced gate.
        bool ret = gate->SwapChild(*it, *child_gate->children().begin());
        if (!ret) {
          break;
        } else {
          it = gate->children().begin();
          continue;
        }
      } else {
        IndexedFaultTree::PreprocessTree(child_gate, processed_gates);
      }
    }
    ++it;
  }
}

void IndexedFaultTree::ProcessNullGates(IndexedGate* gate,
                                        std::set<int>* processed_gates) {
  // NULL gates' parent: OR->Remove the child and AND->NULL the parent.
  // At this stage, only positive gates are left.
  // The tree structure is repeating ...->OR->AND->OR->...

  if (processed_gates->count(gate->index())) return;
  processed_gates->insert(gate->index());

  if (gate->state() == "null") {
    assert(gate->type() == 2);
    gate->EraseAllChildren();
    return;
  }

  if (gate->type() == 1) {
    assert(gate->state() != "null");
    std::set<int>::const_iterator it;
    for (it = gate->children().begin(); it != gate->children().end();) {
      if (std::abs(*it) > top_event_index_) {
        assert(*it > 0);
        IndexedGate* child_gate = indexed_gates_.find(*it)->second;
        assert(child_gate->type() == 2);
        IndexedFaultTree::ProcessNullGates(child_gate, processed_gates);
        if (child_gate->state() == "null") {
          gate->EraseChild(*it);
          it = gate->children().begin();
          continue;
        }
      }
      ++it;
    }
    if (gate->children().empty()) gate->Nullify();
  } else {
    std::set<int>::const_iterator it;
    for (it = gate->children().begin(); it != gate->children().end(); ++it) {
      if (std::abs(*it) > top_event_index_) {
        assert(*it > 0);
        IndexedGate* child_gate = indexed_gates_.find(*it)->second;
        assert(child_gate->type() == 1);
        IndexedFaultTree::ProcessNullGates(child_gate, processed_gates);
        if (child_gate->state() == "null") {
          gate->Nullify();
          return;
        }
      }
    }
  }
}

}  // namespace scram
