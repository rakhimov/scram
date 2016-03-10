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
  if (top->IsConstant() || top->type() == kNull) {
    constant_graph_ = true;
    zbdd_ = std::unique_ptr<Zbdd>(new Zbdd(fault_tree, settings));
    zbdd_->Analyze();
  }
}

void Mocus::Analyze() {
  BLOG(DEBUG2, constant_graph_) << "Graph is constant. No analysis!";
  if (constant_graph_) return;

  CLOCK(mcs_time);
  LOG(DEBUG2) << "Start minimal cut set generation.";
  zbdd_ = Mocus::AnalyzeModule(graph_->root(), kSettings_);
  LOG(DEBUG2) << "Delegating cut set extraction to ZBDD.";
  zbdd_->Analyze();
  LOG(DEBUG2) << "Minimal cut sets found in " << DUR(mcs_time);
}

const std::vector<std::vector<int>>& Mocus::products() const {
  assert(zbdd_ && "Analysis is not done.");
  return zbdd_->products();
}

std::unique_ptr<zbdd::CutSetContainer>
Mocus::AnalyzeModule(const IGatePtr& gate, const Settings& settings) noexcept {
  assert(gate->IsModule() && "Expected only module gates.");
  CLOCK(gen_time);
  LOG(DEBUG3) << "Finding cut sets from module: G" << gate->index();
  LOG(DEBUG4) << "Limit on product order: " << settings.limit_order();
  std::unordered_map<int, IGatePtr> gates = gate->gate_args();
  std::unique_ptr<zbdd::CutSetContainer> container(
      new zbdd::CutSetContainer(kSettings_, gate->index(),
                                graph_->basic_events().size()));
  container->Merge(container->ConvertGate(gate));
  while (int next_gate_index = container->GetNextGate()) {
    LOG(DEBUG5) << "Expanding gate G" << next_gate_index;
    const IGatePtr& next_gate = gates.find(next_gate_index)->second;
    gates.insert(next_gate->gate_args().begin(), next_gate->gate_args().end());
    container->Merge(container->ExpandGate(
        container->ConvertGate(next_gate),
        container->ExtractIntermediateCutSets(next_gate_index)));
  }
  container->Minimize();
  container->Log();
  LOG(DEBUG3) << "G" << gate->index()
              << " cut set generation time: " << DUR(gen_time);
  if (!gate->coherent()) {
    container->EliminateComplements();
    container->Minimize();
  }
  for (const auto& entry : container->GatherModules()) {
    int index = entry.first;
    int limit = entry.second.second;
    if (limit == 0) {  /// @todo Make cut-offs strict.
      std::unique_ptr<zbdd::CutSetContainer> empty_zbdd(
          new zbdd::CutSetContainer(kSettings_, index,
                                    graph_->basic_events().size()));
      container->JoinModule(index, std::move(empty_zbdd));
      continue;
    }
    Settings adjusted(settings);
    adjusted.limit_order(limit);
    container->JoinModule(index, Mocus::AnalyzeModule(gates.find(index)->second,
                                                      adjusted));
  }
  container->EliminateConstantModules();
  container->Minimize();
  return container;
}

}  // namespace scram
