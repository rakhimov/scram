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

/// @file risk_analysis.cc
/// Implementation of risk analysis handler.

#include "risk_analysis.h"

#include <fstream>
#include <utility>
#include <vector>

#include "bdd.h"
#include "error.h"
#include "expression.h"
#include "grapher.h"
#include "logger.h"
#include "mocus.h"
#include "model.h"
#include "random.h"
#include "reporter.h"
#include "zbdd.h"

namespace scram {

RiskAnalysis::RiskAnalysis(const std::shared_ptr<const Model>& model,
                           const Settings& settings)
    : Analysis(settings),
      model_(model) {}

void RiskAnalysis::GraphingInstructions() {
  CLOCK(graph_time);
  LOG(DEBUG1) << "Producing graphing instructions";
  for (const std::pair<const std::string, FaultTreePtr>& fault_tree :
       model_->fault_trees()) {
    for (const GatePtr& top_event : fault_tree.second->top_events()) {
      std::string output =
          fault_tree.second->name() + "_" + top_event->name() + ".dot";
      std::ofstream of(output.c_str());
      if (!of.good()) {
        throw IOError(output +  " : Cannot write the graphing file.");
      }
      Grapher gr = Grapher();
      gr.GraphFaultTree(top_event, Analysis::settings().probability_analysis(),
                        of);
      of.flush();
    }
  }
  LOG(DEBUG1) << "Graphing instructions are produced in " << DUR(graph_time);
}

void RiskAnalysis::Analyze() noexcept {
  // Set the seed for the pseudo-random number generator if given explicitly.
  // Otherwise it defaults to the current time.
  if (Analysis::settings().seed() >= 0)
    Random::seed(Analysis::settings().seed());
  for (const std::pair<const std::string, FaultTreePtr>& ft :
       model_->fault_trees()) {
    for (const GatePtr& target : ft.second->top_events()) {
      std::string base_path =
          target->is_public() ? "" : target->base_path() + ".";
      std::string name = base_path + target->name();  // Analysis ID.

      LOG(INFO) << "Running analysis: " << name;
      RiskAnalysis::RunAnalysis(name, target);
      LOG(INFO) << "Finished analysis: " << name;
    }
  }
}

void RiskAnalysis::RunAnalysis(const std::string& name,
                               const GatePtr& target) noexcept {
  if (Analysis::settings().algorithm() == "bdd") {
    RiskAnalysis::RunAnalysis<Bdd>(name, target);
  } else if (Analysis::settings().algorithm() == "zbdd") {
    RiskAnalysis::RunAnalysis<Zbdd>(name, target);
  } else {
    assert(Analysis::settings().algorithm() == "mocus");
    RiskAnalysis::RunAnalysis<Mocus>(name, target);
  }
}

template <class Algorithm>
void RiskAnalysis::RunAnalysis(const std::string& name,
                               const GatePtr& target) noexcept {
  auto* fta = new FaultTreeAnalyzer<Algorithm>(target, Analysis::settings());
  fta->Analyze();
  if (Analysis::settings().probability_analysis()) {
    if (Analysis::settings().approximation() == "no") {
      RiskAnalysis::RunAnalysis<Algorithm, Bdd>(name, fta);
    } else if (Analysis::settings().approximation() == "rare-event") {
      RiskAnalysis::RunAnalysis<Algorithm, RareEventCalculator>(name, fta);
    } else {
      assert(Analysis::settings().approximation() == "mcub");
      RiskAnalysis::RunAnalysis<Algorithm, McubCalculator>(name, fta);
    }
  }
  fault_tree_analyses_.emplace(name, FaultTreeAnalysisPtr(fta));
}

template <class Algorithm, class Calculator>
void RiskAnalysis::RunAnalysis(const std::string& name,
                               FaultTreeAnalyzer<Algorithm>* fta) noexcept {
  auto* pa = new ProbabilityAnalyzer<Calculator>(fta);
  pa->Analyze();
  if (Analysis::settings().importance_analysis()) {
    auto* ia = new ImportanceAnalyzer<Calculator>(pa);
    ia->Analyze();
    importance_analyses_.emplace(name, ImportanceAnalysisPtr(ia));
  }
  if (Analysis::settings().uncertainty_analysis()) {
    auto* ua = new UncertaintyAnalyzer<Calculator>(pa);
    ua->Analyze();
    uncertainty_analyses_.emplace(name, UncertaintyAnalysisPtr(ua));
  }
  probability_analyses_.emplace(name, ProbabilityAnalysisPtr(pa));
}

void RiskAnalysis::Report(std::ostream& out) {
  Reporter rp = Reporter();
  rp.Report(*this, out);
}

void RiskAnalysis::Report(std::string output) {
  std::ofstream of(output.c_str());
  if (!of.good()) {
    throw IOError(output +  " : Cannot write the output file.");
  }
  RiskAnalysis::Report(of);
}

}  // namespace scram
