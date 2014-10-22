#include "indexed_fault_tree.h"

typedef boost::shared_ptr<scram::Event> EventPtr;

namespace scram {

IndexedFaultTree::IndexedFaultTree(
    int top_event_id,
    const std::set<int>& true_house_events,
    const std::set<int>& false_house_events,
    const boost::unordered_map<std::string, int>& inter_to_int,
    const boost::unordered_map<int, GatePtr>& int_to_inter,
    const boost::unordered_map<std::string, int>& all_to_int) {

  top_event_index_ = top_event_id;
  true_house_events_ = &true_house_events;
  false_house_events_ = &false_house_events;
  inter_to_int_ = &inter_to_int;
  int_to_inter_ = &int_to_inter;
  all_to_int_ = &all_to_int;
  new_gate_index_ = 0;
  top_event_sign_ = 1;
}

IndexedFaultTree::~IndexedFaultTree() {
  boost::unordered_map<int, IndexedGate*>::iterator it;
  for (it = indexed_gates_.begin(); it != indexed_gates_.end(); ++it) {
    delete it->second;
  }
}

void IndexedFaultTree::InitiateIndexedFaultTree() {
  boost::unordered_map<int, GatePtr>::const_iterator it;
  for (it = int_to_inter_->begin(); it != int_to_inter_->end(); ++it) {
    IndexedGate* gate = new IndexedGate(it->first);
    gate->string_type(it->second->type());  // Get the original gate type.
    const std::map<std::string, EventPtr>* children =
        &it->second->children();
    std::map<std::string, EventPtr>::const_iterator it_children;
    for (it_children = children->begin();
         it_children != children->end(); ++it_children) {
      gate->InitiateWithChild(all_to_int_->find(it_children->first)->second);
    }
    indexed_gates_.insert(std::make_pair(it->first, gate));
    if (gate->index() > new_gate_index_) new_gate_index_ = gate->index() + 1;
  }
}

/// Performs processing of a fault tree.
void IndexedFaultTree::ProcessIndexedFaultTree() {
  IndexedFaultTree::StartUnrollingGates();

  IndexedGate* top = indexed_gates_.find(top_event_index_)->second;

  IndexedFaultTree::PropagateConstants(top);

  IndexedFaultTree::PreprocessTree(top);

  IndexedFaultTree::ProcessNullGates(top);
}

/// Start unrolling gates.
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

/// Unrolls the top gate.
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
  std::set<int>::const_iterator it;
  for (it = top_gate->children().begin(); it != top_gate->children().end();
       ++it) {
    if (std::abs(*it) > top_event_index_) {
      IndexedFaultTree::UnrollGate(*it, top_gate);
    }
  }
}

/// Unrolls a gate.
void IndexedFaultTree::UnrollGate(int gate_index, IndexedGate* parent_gate) {
  if (unrolled_gates_.count(std::abs(gate_index))) return;
  IndexedGate* gate = indexed_gates_.find(std::abs(gate_index))->second;
  unrolled_gates_.insert(gate->index());
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
    bool ret = parent_gate->SwapChild(gate_index, -gate_index);
    assert(ret);
  } else if (type == "nor") {
    gate->string_type("or");
    gate->type(1);
    bool ret = parent_gate->SwapChild(gate_index, -gate_index);
    assert(ret);
  } else if (type == "xor") {
    IndexedFaultTree::UnrollXorGate(gate);
  } else if (type == "atleast") {
    IndexedFaultTree::UnrollAtleastGate(gate);
  } else {
    assert(false);
  }
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end();
       ++it) {
    if (std::abs(*it) > top_event_index_) {
      IndexedFaultTree::UnrollGate(*it, gate);
    }
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
  int vote_number =
      int_to_inter_->find(gate->index())->second->vote_number();

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

/// Remove all house events.
void IndexedFaultTree::PropagateConstants(IndexedGate* gate) {
  // True house event in AND gate is removed.
  // False house event in AND gate makes the gate NULL.
  // True house event in OR gate makes the gate Unity, and it shouldn't appear
  // in minimal cut sets.
  // False house event in OR gate is removed.
  // Unity must be only due to House event.
  // Null can be due to house events or complement elments.
  std::set<int>::const_iterator it;
  int parent_type = gate->type();
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    if (std::abs(*it) > top_event_index_) {
      IndexedGate* child_gate = indexed_gates_.find(std::abs(*it))->second;
      PropagateConstants(child_gate);
      if (*it > 0) {
        if (child_gate->state() == "null") {
          if (parent_type == 1) {
            gate->EraseChild(*it);  // OR gate with null child.
          } else {
            // AND gate with null child.
            gate->Nullify();
          }
        } else if (child_gate->state() == "unity") {
          if (parent_type == 1) {
            gate->MakeUnity();
          } else {
            gate->EraseChild(*it);
          }
        }
      } else {
        if (child_gate->state() == "null") {
          if (parent_type == 1) {
            // Makes unity.
            gate->MakeUnity();
          } else {
            gate->EraseChild(*it);  // AND gate with unity child.
          }
        } else if (child_gate->state() == "unity") {
          if (parent_type == 1) {
            gate->EraseChild(*it);  // OR gate with null child.
          } else {
            // AND gate with null child.
            gate->Nullify();
          }
        }
      }
    } else {
      if (*it > 0) {
        if (false_house_events_->count(*it)) {
          if (parent_type == 1) {
            gate->EraseChild(*it);  // OR gate with false house child.
          } else {
            // AND gate with false child.
            gate->Nullify();
          }
        } else if (true_house_events_->count(*it)) {
          if (parent_type == 1) {
            gate->MakeUnity();
          } else {
            gate->EraseChild(*it);
          }
        }
      } else {
        if (false_house_events_->count(-*it)) {
          if (parent_type == 1) {
            gate->MakeUnity();
          } else {
            gate->EraseChild(*it);
          }
        } else if (true_house_events_->count(-*it)) {
          if (parent_type == 1) {
            gate->EraseChild(*it);  // OR gate with false house child.
          } else {
            // AND gate with false child.
            gate->Nullify();
          }
        }
      }
    }
  }
}

/// This method pre-processes the tree by doing Boolean algebra.
/// At this point all gates are expected to be either OR type or AND type.
void IndexedFaultTree::PreprocessTree(IndexedGate* gate) {
  if (preprocessed_gates_.count(gate->index())) return;
  preprocessed_gates_.insert(gate->index());
  int parent = gate->type();
  std::set<int>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    if (std::abs(*it) < top_event_index_) continue;
    IndexedGate* child_gate = indexed_gates_.find(std::abs(*it))->second;
    int child = child_gate->type();
    if (*it > 0 && (parent == child)) {
      if (!gate->MergeGate(indexed_gates_.find(*it)->second)) {
        null_gates_.insert(gate->index());
        break;
      } else {
        it = gate->children().begin();
        continue;
      }
    } else if (child_gate->children().size() == 1) {
      // This must be from null, not, or some reduced gate.
      bool ret = true;
      if (*it > 0) {
        ret = gate->SwapChild(*it, *child_gate->children().begin());
      } else {
        ret = gate->SwapChild(*it, -*child_gate->children().begin());
      }
      if (!ret) {
        null_gates_.insert(gate->index());
        break;
      } else {
        it = gate->children().begin();
        continue;
      }
    } else {
      IndexedFaultTree::PreprocessTree(child_gate);
    }
  }
}

/// Process null gates.
void IndexedFaultTree::ProcessNullGates(IndexedGate* parent) {
  // NULL gates' parent: OR->Remove the child and AND->NULL the parent.
  if (parent->type() == 1) {

  } else {

  }
}

}  // namespace scram
