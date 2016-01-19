/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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
/// that the graph is in negation normal form.
///
/// A ZBDD data structure is employed to store and extract
/// intermediate (containing gates)
/// and final (basic events only) cut sets
/// upon cut set generation.

#include "mocus.h"

#include "logger.h"

namespace scram {

Mocus::Mocus(const BooleanGraph* fault_tree, const Settings& settings)
      : constant_graph_(false),
        graph_(fault_tree),
        kSettings_(settings) {
  IGatePtr top = fault_tree->root();
  // Special case of empty top gate.
  if (top->IsConstant()) {
    if (top->state() == kUnityState) products_.push_back({});  // Unity set.
    constant_graph_ = true;
    return;  // Other cases are null or empty.
  }
  if (top->type() == kNullGate) {  // Special case of NULL type top.
    assert(top->args().size() == 1);
    assert(top->gate_args().empty());
    int child = *top->args().begin();
    child > 0 ? products_.push_back({child}) : products_.push_back({});
    constant_graph_ = true;
    return;
  }
}

void Mocus::Analyze() {
  BLOG(DEBUG2, constant_graph_) << "Graph is constant. No analysis!";
  if (constant_graph_) return;

  CLOCK(mcs_time);
  LOG(DEBUG2) << "Start minimal cut set generation.";
  zbdd::CutSetContainer container = Mocus::AnalyzeModule(graph_->root());
  LOG(DEBUG2) << "Delegating cut set minimization to ZBDD.";
  container.Analyze();
  products_ = container.products();
  LOG(DEBUG2) << "Minimal cut sets found in " << DUR(mcs_time);
}

zbdd::CutSetContainer Mocus::AnalyzeModule(const IGatePtr& gate) noexcept {
  assert(gate->IsModule() && "Expected only module gates.");
  CLOCK(gen_time);
  LOG(DEBUG3) << "Finding cut sets from module: G" << gate->index();
  std::unordered_map<int, IGatePtr> gates;
  gates.insert(gate->gate_args().begin(), gate->gate_args().end());

  zbdd::CutSetContainer cut_sets(kSettings_, graph_->basic_events().size());
  cut_sets.Merge(cut_sets.ConvertGate(gate));
  int next_gate = cut_sets.GetNextGate();
  while (next_gate) {
    LOG(DEBUG5) << "Expanding gate G" << next_gate;
    IGatePtr inter_gate = gates.find(next_gate)->second;
    gates.insert(inter_gate->gate_args().begin(),
                 inter_gate->gate_args().end());
    cut_sets.Merge(
        cut_sets.ExpandGate(cut_sets.ConvertGate(inter_gate),
                            cut_sets.ExtractIntermediateCutSets(next_gate)));
    next_gate = cut_sets.GetNextGate();
  }
  cut_sets.Minimize();
  if (!graph_->coherent()) {
    cut_sets.EliminateComplements();
    cut_sets.Minimize();
  }
  for (int module : cut_sets.GatherModules()) {
    cut_sets.JoinModule(module,
                        Mocus::AnalyzeModule(gates.find(module)->second));
  }
  cut_sets.EliminateConstantModules();
  cut_sets.Minimize();
  LOG(DEBUG4) << "G" << gate->index()
              << " cut set generation time: " << DUR(gen_time);
  return cut_sets;
}

}  // namespace scram
