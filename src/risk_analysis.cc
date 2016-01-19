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

RiskAnalysis::RiskAnalysis(const ModelPtr& model, const Settings& settings)
    : Analysis::Analysis(settings),
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
      gr.GraphFaultTree(top_event, kSettings_.probability_analysis(), of);
      of.flush();
    }
  }
  LOG(DEBUG1) << "Graphing instructions are produced in " << DUR(graph_time);
}

void RiskAnalysis::Analyze() noexcept {
  // Set the seed for the pseudo-random number generator if given explicitly.
  // Otherwise it defaults to the current time.
  if (kSettings_.seed() >= 0) Random::seed(kSettings_.seed());
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
  if (kSettings_.algorithm() == "bdd") {
    RiskAnalysis::RunAnalysis<Bdd>(name, target);
  } else if (kSettings_.algorithm() == "zbdd") {
    RiskAnalysis::RunAnalysis<Zbdd>(name, target);
  } else {
    assert(kSettings_.algorithm() == "mocus");
    RiskAnalysis::RunAnalysis<Mocus>(name, target);
  }
}

template<typename Algorithm>
void RiskAnalysis::RunAnalysis(const std::string& name,
                               const GatePtr& target) noexcept {
  auto* fta = new FaultTreeAnalyzer<Algorithm>(target, kSettings_);
  fta->Analyze();
  if (kSettings_.probability_analysis()) {
    if (kSettings_.approximation() == "no") {
      RiskAnalysis::RunAnalysis<Algorithm, Bdd>(name, fta);
    } else if (kSettings_.approximation() == "rare-event") {
      RiskAnalysis::RunAnalysis<Algorithm, RareEventCalculator>(name, fta);
    } else {
      assert(kSettings_.approximation() == "mcub");
      RiskAnalysis::RunAnalysis<Algorithm, McubCalculator>(name, fta);
    }
  }
  fault_tree_analyses_.emplace(name, FaultTreeAnalysisPtr(fta));
}

template<typename Algorithm, typename Calculator>
void RiskAnalysis::RunAnalysis(const std::string& name,
                               FaultTreeAnalyzer<Algorithm>* fta) noexcept {
  auto* pa = new ProbabilityAnalyzer<Calculator>(fta);
  pa->Analyze();
  if (kSettings_.importance_analysis()) {
    auto* ia = new ImportanceAnalyzer<Calculator>(pa);
    ia->Analyze();
    importance_analyses_.emplace(name, ImportanceAnalysisPtr(ia));
  }
  if (kSettings_.uncertainty_analysis()) {
    auto* ua = new UncertaintyAnalyzer<Calculator>(pa);
    ua->Analyze();
    uncertainty_analyses_.emplace(name, UncertaintyAnalysisPtr(ua));
  }
  probability_analyses_.emplace(name, ProbabilityAnalysisPtr(pa));
}

void RiskAnalysis::Report(std::ostream& out) {
  Reporter rp = Reporter();

  // Create XML or use already created document.
  std::unique_ptr<xmlpp::Document> doc(new xmlpp::Document());
  rp.SetupReport(model_, kSettings_, doc.get());

  // Container for excess primary events not in the analysis.
  // This container is for warning
  // in case the input is formed not as intended.
  std::vector<std::shared_ptr<const PrimaryEvent>> orphan_primary_events;
  for (const std::pair<const std::string, BasicEventPtr>& event :
       model_->basic_events()) {
    if (event.second->orphan()) orphan_primary_events.push_back(event.second);
  }

  for (const std::pair<const std::string, HouseEventPtr>& event :
       model_->house_events()) {
    if (event.second->orphan()) orphan_primary_events.push_back(event.second);
  }
  rp.ReportOrphanPrimaryEvents(orphan_primary_events, doc.get());

  // Container for unused parameters not in the analysis.
  // This container is for warning in case the input is formed not as intended.
  std::vector<std::shared_ptr<const Parameter>> unused_parameters;
  for (const std::pair<const std::string, ParameterPtr>& param :
       model_->parameters()) {
    if (param.second->unused()) unused_parameters.push_back(param.second);
  }
  rp.ReportUnusedParameters(unused_parameters, doc.get());

  CLOCK(report_time);
  LOG(DEBUG1) << "Reporting analysis results...";
  for (const std::pair<const std::string, FaultTreeAnalysisPtr>& fta :
       fault_tree_analyses_) {
    std::string id = fta.first;
    ProbabilityAnalysis* prob_analysis = nullptr;  // Null if no analysis.
    if (kSettings_.probability_analysis()) {
      prob_analysis = probability_analyses_.at(id).get();
    }
    rp.ReportFta(id, *fta.second, prob_analysis, doc.get());

    if (kSettings_.importance_analysis()) {
      rp.ReportImportance(id, *importance_analyses_.at(id), doc.get());
    }

    if (kSettings_.uncertainty_analysis()) {
      rp.ReportUncertainty(id, *uncertainty_analyses_.at(id), doc.get());
    }
  }
  LOG(DEBUG1) << "Finished reporting in " << DUR(report_time);

  doc->write_to_stream_formatted(out, "UTF-8");
}

void RiskAnalysis::Report(std::string output) {
  std::ofstream of(output.c_str());
  if (!of.good()) {
    throw IOError(output +  " : Cannot write the output file.");
  }
  RiskAnalysis::Report(of);
}

}  // namespace scram
