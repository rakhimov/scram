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

/// @file risk_analysis.cc
/// Implementation of risk analysis handler.

#include "risk_analysis.h"

#include <fstream>
#include <set>
#include <unordered_map>
#include <utility>

#include "error.h"
#include "event.h"
#include "fault_tree.h"
#include "grapher.h"
#include "logger.h"
#include "model.h"
#include "random.h"
#include "reporter.h"

namespace scram {

RiskAnalysis::RiskAnalysis(const ModelPtr& model, const Settings& settings)
    : model_(model),
      kSettings_(settings) {}

void RiskAnalysis::GraphingInstructions() {
  CLOCK(graph_time);
  LOG(DEBUG1) << "Producing graphing instructions";
  std::unordered_map<std::string, FaultTreePtr>::const_iterator it;
  for (it = model_->fault_trees().begin(); it != model_->fault_trees().end();
       ++it) {
    const std::vector<GatePtr>& top_events = it->second->top_events();
    std::vector<GatePtr>::const_iterator it_top;
    for (it_top = top_events.begin(); it_top != top_events.end(); ++it_top) {
      std::string output =
          it->second->name() + "_" + (*it_top)->name() + ".dot";
      std::ofstream of(output.c_str());
      if (!of.good()) {
        throw IOError(output +  " : Cannot write the graphing file.");
      }
      Grapher gr = Grapher();
      gr.GraphFaultTree(*it_top, kSettings_.probability_analysis(), of);
      of.flush();
    }
  }
  LOG(DEBUG1) << "Graphing instructions are produced in " << DUR(graph_time);
}

void RiskAnalysis::Analyze() {
  // Set the seed for the pseudo-random number generator if given explicitly.
  // Otherwise it defaults to the current time.
  if (kSettings_.seed() >= 0) Random::seed(kSettings_.seed());

  const std::unordered_map<std::string, FaultTreePtr>& fault_trees =
      model_->fault_trees();
  for (const auto& ft : fault_trees) {
    for (const GatePtr& target : ft.second->top_events()) {
      std::string base_path =
          target->is_public() ? "" : target->base_path() + ".";
      std::string name = base_path + target->name();  // Analysis ID.

      FaultTreeAnalysisPtr fta(new FaultTreeAnalysis(target, kSettings_));
      fta->Analyze();
      fault_tree_analyses_.emplace(name, fta);

      if (kSettings_.probability_analysis()) {
        ProbabilityAnalysisPtr pa(new ProbabilityAnalysis(kSettings_));
        pa->UpdateDatabase(fta->mcs_basic_events());
        pa->Analyze(fta->min_cut_sets());
        probability_analyses_.emplace(name, pa);
      }

      if (kSettings_.uncertainty_analysis()) {
        UncertaintyAnalysisPtr ua(new UncertaintyAnalysis(kSettings_));
        ua->UpdateDatabase(fta->mcs_basic_events());
        ua->Analyze(fta->min_cut_sets());
        uncertainty_analyses_.emplace(name, ua);
      }
    }
  }
}

void RiskAnalysis::Report(std::ostream& out) {
  Reporter rp = Reporter();

  // Create XML or use already created document.
  xmlpp::Document* doc = new xmlpp::Document();
  rp.SetupReport(model_, kSettings_, doc);

  // Container for excess primary events not in the analysis.
  // This container is for warning
  // in case the input is formed not as intended.
  typedef std::shared_ptr<const PrimaryEvent> PrimaryEventPtr;
  typedef std::shared_ptr<BasicEvent> BasicEventPtr;
  std::vector<PrimaryEventPtr> orphan_primary_events;
  std::unordered_map<std::string, BasicEventPtr>::const_iterator it_b;
  for (it_b = model_->basic_events().begin();
       it_b != model_->basic_events().end(); ++it_b) {
    if (it_b->second->orphan()) orphan_primary_events.push_back(it_b->second);
  }
  typedef std::shared_ptr<HouseEvent> HouseEventPtr;
  std::unordered_map<std::string, HouseEventPtr>::const_iterator it_h;
  for (it_h = model_->house_events().begin();
       it_h != model_->house_events().end(); ++it_h) {
    if (it_h->second->orphan()) orphan_primary_events.push_back(it_h->second);
  }
  rp.ReportOrphanPrimaryEvents(orphan_primary_events, doc);

  // Container for unused parameters not in the analysis.
  // This container is for warning in case the input is formed not as intended.
  typedef std::shared_ptr<Parameter> ParameterPtr;
  std::vector<std::shared_ptr<const Parameter>> unused_parameters;
  std::unordered_map<std::string, ParameterPtr>::const_iterator it_v;
  for (it_v = model_->parameters().begin(); it_v != model_->parameters().end();
       ++it_v) {
    if (it_v->second->unused()) unused_parameters.push_back(it_v->second);
  }
  rp.ReportUnusedParameters(unused_parameters, doc);

  std::map<std::string, FaultTreeAnalysisPtr>::iterator it;
  for (it = fault_tree_analyses_.begin(); it != fault_tree_analyses_.end();
       ++it) {
    ProbabilityAnalysisPtr prob_analysis;  // Null pointer if no analysis.
    if (kSettings_.probability_analysis()) {
      prob_analysis = probability_analyses_.find(it->first)->second;
    }
    rp.ReportFta(it->first, fault_tree_analyses_.find(it->first)->second,
                 prob_analysis, doc);

    if (kSettings_.importance_analysis()) {
      rp.ReportImportance(it->first,
                          probability_analyses_.find(it->first)->second, doc);
    }

    if (kSettings_.uncertainty_analysis()) {
        rp.ReportUncertainty(it->first,
                             uncertainty_analyses_.find(it->first)->second,
                             doc);
    }
  }

  doc->write_to_stream_formatted(out, "UTF-8");
  delete doc;
}

void RiskAnalysis::Report(std::string output) {
  std::ofstream of(output.c_str());
  if (!of.good()) {
    throw IOError(output +  " : Cannot write the output file.");
  }
  RiskAnalysis::Report(of);
}

}  // namespace scram
