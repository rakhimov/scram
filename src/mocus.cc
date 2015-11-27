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
/// The algorithm assumes
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

#include <utility>

#include "logger.h"
#include "zbdd.h"

namespace scram {

namespace mocus {

SimpleGate::SimpleGate(Operator type, int limit) noexcept
    : type_(type),
      limit_order_(limit) {}

void SimpleGate::GenerateCutSets(const CutSetPtr& cut_set,
                                 CutSetContainer* new_cut_sets) noexcept {
  assert(cut_set->order() <= limit_order_);
  if (type_ == kOrGate) {
      SimpleGate::OrGateCutSets(cut_set, new_cut_sets);
  } else {
    assert(type_ == kAndGate && "MOCUS works with AND/OR gates only.");
    SimpleGate::AndGateCutSets(cut_set, new_cut_sets);
  }
}

void SimpleGate::AndGateCutSets(const CutSetPtr& cut_set,
                                CutSetContainer* new_cut_sets) noexcept {
  assert(cut_set->order() <= limit_order_);
  // Check for null case.
  if (cut_set->HasNegativeLiteral(pos_literals_)) return;
  if (cut_set->HasPositiveLiteral(neg_literals_)) return;
  // Limit order checks before other expensive operations.
  if (cut_set->CheckJointOrder(pos_literals_, limit_order_)) return;
  CutSetPtr cut_set_copy(new CutSet(*cut_set));
  // Include all basic events and modules into the set.
  cut_set_copy->AddPositiveLiterals(pos_literals_);
  cut_set_copy->AddNegativeLiterals(neg_literals_);
  cut_set_copy->AddModules(modules_);

  // Deal with many OR gate children.
  CutSetContainer arguments = {cut_set_copy};  // Input to OR gates.
  for (const SimpleGatePtr& gate : gates_) {
    CutSetContainer results;
    for (const CutSetPtr& arg_set : arguments) {
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

void SimpleGate::OrGateCutSets(const CutSetPtr& cut_set,
                               CutSetContainer* new_cut_sets) noexcept {
  assert(cut_set->order() <= limit_order_);
  // Check for local minimality.
  if (cut_set->HasPositiveLiteral(pos_literals_) ||
      cut_set->HasNegativeLiteral(neg_literals_) ||
      cut_set->HasModule(modules_)) {
    new_cut_sets->insert(cut_set);
    return;
  }
  // Generate cut sets from child gates of AND type.
  CutSetContainer local_sets;
  for (const SimpleGatePtr& gate : gates_) {
    gate->AndGateCutSets(cut_set, &local_sets);
    if (local_sets.count(cut_set)) {
      new_cut_sets->insert(cut_set);
      return;
    }
  }
  // Create new cut sets from basic events and modules.
  if (cut_set->order() < limit_order_) {
    // There is a guarantee of an order increase of a cut set.
    for (int index : pos_literals_) {
      if (cut_set->HasNegativeLiteral(index)) continue;
      CutSetPtr new_set(new CutSet(*cut_set));
      new_set->AddPositiveLiteral(index);
      new_cut_sets->insert(new_set);
    }
  }
  for (int index : neg_literals_) {
    if (cut_set->HasPositiveLiteral(index)) continue;
    CutSetPtr new_set(new CutSet(*cut_set));
    new_set->AddNegativeLiteral(index);
    new_cut_sets->insert(new_set);
  }
  for (int index : modules_) {
    // No check for complements. The modules are assumed to be positive.
    CutSetPtr new_set(new CutSet(*cut_set));
    new_set->AddModule(index);
    new_cut_sets->insert(new_set);
  }

  new_cut_sets->insert(local_sets.begin(), local_sets.end());
}

}  // namespace mocus

Mocus::Mocus(const BooleanGraph* fault_tree, const Settings& settings)
      : constant_graph_(false),
        kSettings_(settings) {
  IGatePtr top = fault_tree->root();
  root_index_ = top->index();
  // Special case of empty top gate.
  if (top->IsConstant()) {
    if (top->state() == kUnityState) cut_sets_.push_back({});  // Unity set.
    constant_graph_ = true;
    return;  // Other cases are null or empty.
  }
  if (top->type() == kNullGate) {  // Special case of NULL type top.
    assert(top->args().size() == 1);
    assert(top->gate_args().empty());
    int child = *top->args().begin();
    child > 0 ? cut_sets_.push_back({child}) : cut_sets_.push_back({});
    constant_graph_ = true;
    return;
  }
  std::unordered_map<int, SimpleGatePtr> simple_gates;
  Mocus::CreateSimpleTree(top, &simple_gates);
  LOG(DEBUG3) << "Converted Boolean graph with top module: G" << top->index();
}

Mocus::~Mocus() noexcept = default;

const std::vector<std::vector<int>>& Mocus::cut_sets() const {
  if (constant_graph_) return cut_sets_;
  return zbdd_->cut_sets();
}

void Mocus::Analyze() {
  BLOG(DEBUG2, constant_graph_) << "Graph is constant. No analysis!";
  if (constant_graph_) return;

  CLOCK(mcs_time);
  LOG(DEBUG2) << "Start minimal cut set generation.";
  std::vector<std::pair<int, mocus::CutSetContainer>> module_sets;
  for (const std::pair<int, SimpleGatePtr>& module : modules_) {
    CLOCK(gen_time);
    LOG(DEBUG3) << "Finding cut sets from module: G" << module.first;
    mocus::CutSetContainer cut_sets;
    module.second->GenerateCutSets(CutSetPtr(new CutSet), &cut_sets);
    module_sets.emplace_back(module.first, cut_sets);
    LOG(DEBUG4) << "Unique cut sets generated: " << cut_sets.size();
    LOG(DEBUG4) << "Cut set generation time: " << DUR(gen_time);
  }
  LOG(DEBUG2) << "Delegating cut set minimization to ZBDD.";
  zbdd_ = std::unique_ptr<Zbdd>(new Zbdd(root_index_, module_sets, kSettings_));
  zbdd_->Analyze();
  LOG(DEBUG2) << "Minimal cut sets found in " << DUR(mcs_time);
}

void Mocus::CreateSimpleTree(
    const IGatePtr& gate,
    std::unordered_map<int, SimpleGatePtr>* processed_gates) noexcept {
  if (processed_gates->count(gate->index())) return;
  assert(gate->type() == kAndGate || gate->type() == kOrGate);
  SimpleGatePtr simple_gate(
      new mocus::SimpleGate(gate->type(), kSettings_.limit_order()));
  processed_gates->emplace(gate->index(), simple_gate);
  if (gate->IsModule()) modules_.emplace_back(gate->index(), simple_gate);

  assert(gate->constant_args().empty());
  assert(gate->args().size() > 1);
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    assert(arg.first > 0);
    IGatePtr child_gate = arg.second;
    Mocus::CreateSimpleTree(child_gate, processed_gates);
    if (child_gate->IsModule()) {
      simple_gate->AddModule(arg.first);
    } else {
      simple_gate->AddGate(processed_gates->find(arg.first)->second);
    }
  }
  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    simple_gate->AddLiteral(arg.first);
  }
  simple_gate->SetupForAnalysis();
}

}  // namespace scram
