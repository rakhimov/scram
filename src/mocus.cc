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

/// @file mocus.cc
/// Implementation of the MOCUS algorithm.
/// The algorithms assumes
/// that the tree is layered
/// with OR and AND gates on each level.
/// That is, one level contains only AND or OR gates.
/// The algorithm assumes the graph contains only positive gates.
///
/// The description of the algorithm.
///
/// Turn all existing gates in the tree into simple gates
/// with pointers to the child gates but not modules.
/// Leave minimal cut set modules to the last moment
/// till all the gates are operated.
/// Those modules' minimal cut sets can be joined without
/// additional check for minimality.
///
/// Operate on each module starting from the top gate.
/// For now, it is assumed that a module cannot be unity,
/// which means that a module will at least add a new event into a cut set,
/// so the size of a cut set with modules
/// is a minimum number of members in the set.
/// This assumption will fail
/// if there is unity case
/// but will hold
/// if the module is null because the cut set will be deleted anyway.
///
/// Upon walking from top to children gates,
/// there are two types: OR and AND.
/// The generated sets are passed to child gates,
/// which use the passed set to generate new sets.
/// AND gate will simply add its basic events and modules to the set
/// and pass the resultant sets into its OR child,
/// which will generate a lot more sets.
/// These generated sets are passed to the next gate child
/// to generate even more.
///
/// For OR gates, the passed set is checked
/// to have basic events of the gate.
/// If so, this is a local minimum cut set,
/// so generation of the sets stops on this gate.
/// No new sets should be generated in this case.
/// This condition is also applicable
/// if the child AND gate keeps the input set as output
/// and generates only additional supersets.
///
/// The generated sets are kept unique by storing them in a set.

#include "mocus.h"

#include <algorithm>
#include <utility>

#include "logger.h"

namespace scram {

int SimpleGate::limit_order_ = 20;

void SimpleGate::GenerateCutSets(const SetPtr& cut_set,
                                 HashSet* new_cut_sets) noexcept {
  assert(cut_set->size() <= limit_order_);
  assert(type_ == kOrGate || type_ == kAndGate);
  switch (type_) {
    case kOrGate:
      SimpleGate::OrGateCutSets(cut_set, new_cut_sets);
      break;
    case kAndGate:
      SimpleGate::AndGateCutSets(cut_set, new_cut_sets);
      break;
  }
}

void SimpleGate::AndGateCutSets(const SetPtr& cut_set,
                                HashSet* new_cut_sets) noexcept {
  assert(cut_set->size() <= limit_order_);
  // Check for null case.
  for (int index : basic_events_) {
    if (cut_set->count(-index)) return;
  }
  // Limit order checks before other expensive operations.
  int order = cut_set->size();
  for (int index : basic_events_) {
    if (!cut_set->count(index)) ++order;
    if (order > limit_order_) return;
  }
  for (int index : modules_) {
    if (!cut_set->count(index)) ++order;
    if (order > limit_order_) return;
  }
  SetPtr cut_set_copy(new Set(*cut_set));
  // Include all basic events and modules into the set.
  cut_set_copy->insert(basic_events_.begin(), basic_events_.end());
  cut_set_copy->insert(modules_.begin(), modules_.end());

  // Deal with many OR gate children.
  HashSet arguments = {cut_set_copy};  // Input to OR gates.
  for (const SimpleGatePtr& gate : gates_) {
    HashSet results;
    for (const SetPtr& arg_set : arguments) {
      gate->OrGateCutSets(arg_set, &results);
    }
    arguments = results;
  }
  if (arguments.empty()) return;
  if (arguments.count(cut_set_copy)) {  // Other sets are supersets.
    new_cut_sets->insert(cut_set_copy);
  } else {
    new_cut_sets->insert(arguments.begin(), arguments.end());
  }
}

void SimpleGate::OrGateCutSets(const SetPtr& cut_set,
                               HashSet* new_cut_sets) noexcept {
  assert(cut_set->size() <= limit_order_);
  // Check for local minimality.
  for (int index : basic_events_) {
    if (cut_set->count(index)) {
      new_cut_sets->insert(cut_set);
      return;
    }
  }
  for (int index : modules_) {
    if (cut_set->count(index)) {
      new_cut_sets->insert(cut_set);
      return;
    }
  }
  // Generate cut sets from child gates of AND type.
  HashSet local_sets;
  for (const SimpleGatePtr& gate : gates_) {
    gate->AndGateCutSets(cut_set, &local_sets);
    if (local_sets.count(cut_set)) {
      new_cut_sets->insert(cut_set);
      return;
    }
  }
  // There is a guarantee of a size increase of a cut set.
  if (cut_set->size() < limit_order_) {
    // Create new cut sets from basic events and modules.
    for (int index : basic_events_) {
      if (!cut_set->count(-index)) {
        SetPtr new_set(new Set(*cut_set));
        new_set->insert(index);
        new_cut_sets->insert(new_set);
      }
    }
    for (int index : modules_) {
      // No check for complements. The modules are assumed to be positive.
      SetPtr new_set(new Set(*cut_set));
      new_set->insert(index);
      new_cut_sets->insert(new_set);
    }
  }

  new_cut_sets->insert(local_sets.begin(), local_sets.end());
}

Mocus::Mocus(const BooleanGraph* fault_tree, int limit_order)
      : fault_tree_(fault_tree),
        limit_order_(limit_order) {
  SimpleGate::limit_order(limit_order);
}

void Mocus::FindMcs() {
  CLOCK(mcs_time);
  LOG(DEBUG2) << "Start minimal cut set generation.";

  IGatePtr top = fault_tree_->root();

  // Special case of empty top gate.
  if (top->args().empty()) {
    State state = top->state();
    assert(state == kNullState || state == kUnityState);
    if (state == kUnityState) {
      Set empty_set;
      imcs_.push_back(empty_set);  // Special indication of unity set.
    }  // Other cases are null.
    return;
  } else if (top->type() == kNullGate) {  // Special case of NULL type top.
    assert(top->args().size() == 1);
    assert(top->gate_args().empty());
    int child = *top->args().begin();
    Set one_element;
    one_element.insert(child);
    imcs_.push_back(one_element);
    return;
  }

  // Create simple gates from indexed gates.
  std::unordered_map<int, SimpleGatePtr> simple_gates;
  Mocus::CreateSimpleTree(top, &simple_gates);

  LOG(DEBUG3) << "Finding MCS from top module: " << top->index();
  std::vector<Set> mcs;
  Mocus::FindMcsFromSimpleGate(simple_gates.find(top->index())->second, &mcs);

  LOG(DEBUG3) << "Top gate cut sets are generated.";

  // The next is to join all other modules.
  LOG(DEBUG3) << "Joining modules.";
  // Save minimal cut sets of analyzed modules.
  std::unordered_map<int, std::vector<Set>> module_mcs;
  while (!mcs.empty()) {
    Set member = mcs.back();
    mcs.pop_back();
    int largest_element = std::abs(*member.rbegin());  // Positive modules!
    if (largest_element <= fault_tree_->basic_events().size()) {
      imcs_.push_back(member);  // All elements are basic events.
    } else {
      Set::iterator it_s = member.end();
      --it_s;
      int module_index = *it_s;
      member.erase(it_s);
      std::vector<Set> sub_mcs;
      if (module_mcs.count(module_index)) {
        sub_mcs = module_mcs.find(module_index)->second;
      } else {
        LOG(DEBUG3) << "Finding MCS from module index: " << module_index;
        Mocus::FindMcsFromSimpleGate(simple_gates.find(module_index)->second,
                                     &sub_mcs);
        module_mcs.emplace(module_index, sub_mcs);
      }
      std::vector<Set>::iterator it;
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
  assert(top->state() != kUnityState);
  LOG(DEBUG2) << "The number of MCS found: " << imcs_.size();
  LOG(DEBUG2) << "Minimal cut sets found in " << DUR(mcs_time);
}

void Mocus::CreateSimpleTree(
    const IGatePtr& gate,
    std::unordered_map<int, SimpleGatePtr>* processed_gates) noexcept {
  if (processed_gates->count(gate->index())) return;
  assert(gate->type() == kAndGate || gate->type() == kOrGate);
  SimpleGatePtr simple_gate(new SimpleGate(gate->type()));
  processed_gates->emplace(gate->index(), simple_gate);

  assert(gate->constant_args().empty());
  assert(gate->args().size() > 1);
  for (const std::pair<int, IGatePtr>& arg : gate->gate_args()) {
    assert(arg.first > 0);
    IGatePtr child_gate = arg.second;
    Mocus::CreateSimpleTree(child_gate, processed_gates);
    if (child_gate->IsModule()) {
      simple_gate->InitiateWithModule(arg.first);
    } else {
      simple_gate->AddChildGate(processed_gates->find(arg.first)->second);
    }
  }
  typedef std::shared_ptr<Variable> VariablePtr;
  for (const std::pair<int, VariablePtr>& arg : gate->variable_args()) {
    simple_gate->InitiateWithBasic(arg.first);
  }
}

void Mocus::FindMcsFromSimpleGate(const SimpleGatePtr& gate,
                                  std::vector<Set>* mcs) noexcept {
  CLOCK(gen_time);

  SimpleGate::HashSet cut_sets;
  // Generate main minimal cut set gates from top module.
  gate->GenerateCutSets(SetPtr(new Set), &cut_sets);  // Initial empty cut set.

  LOG(DEBUG4) << "Unique cut sets generated: " << cut_sets.size();
  LOG(DEBUG4) << "Cut set generation time: " << DUR(gen_time);

  CLOCK(min_time);
  LOG(DEBUG4) << "Minimizing the cut sets.";

  std::vector<const Set*> cut_sets_vector;
  cut_sets_vector.reserve(cut_sets.size());
  for (const SetPtr& cut_set : cut_sets) {
    assert(!cut_set->empty());
    if (cut_set->size() == 1) {
      mcs->push_back(*cut_set);
    } else {
      cut_sets_vector.push_back(cut_set.get());
    }
  }
  Mocus::MinimizeCutSets(cut_sets_vector, *mcs, 2, mcs);

  LOG(DEBUG4) << "The number of local MCS: " << mcs->size();
  LOG(DEBUG4) << "Cut set minimization time: " << DUR(min_time);
}

void Mocus::MinimizeCutSets(const std::vector<const Set*>& cut_sets,
                            const std::vector<Set>& mcs_lower_order,
                            int min_order,
                            std::vector<Set>* mcs) noexcept {
  if (cut_sets.empty()) return;

  std::vector<const Set*> temp_sets;  // For mcs of a level above.
  std::vector<Set> temp_min_sets;  // For mcs of this level.

  for (const auto& unique_cut_set : cut_sets) {
    bool include = true;  // Determine to keep or not.
    for (const Set& min_cut_set : mcs_lower_order) {
      if (std::includes(unique_cut_set->begin(), unique_cut_set->end(),
                        min_cut_set.begin(), min_cut_set.end())) {
        // Non-minimal cut set is detected.
        include = false;
        break;
      }
    }
    // After checking for non-minimal cut sets,
    // all minimum sized cut sets are guaranteed to be minimal.
    if (include) {
      if (unique_cut_set->size() == min_order) {
        temp_min_sets.push_back(*unique_cut_set);
      } else {
        temp_sets.push_back(unique_cut_set);
      }
    }
    // Ignore the cut set because include = false.
  }
  mcs->insert(mcs->end(), temp_min_sets.begin(), temp_min_sets.end());
  min_order++;
  Mocus::MinimizeCutSets(temp_sets, temp_min_sets, min_order, mcs);
}

}  // namespace scram
