/// @file risk_analysis.cc
/// Implementation of risk analysis handler.
#include "risk_analysis.h"

#include <fstream>
#include <set>

#include <boost/unordered_map.hpp>

#include "error.h"
#include "event.h"
#include "fault_tree.h"
#include "grapher.h"
#include "logger.h"
#include "model.h"
#include "random.h"
#include "reporter.h"

namespace scram {

RiskAnalysis::RiskAnalysis(const ModelPtr& model, const Settings& settings) {
  model_ = model;
  settings_ = settings;
}

void RiskAnalysis::GraphingInstructions() {
  CLOCK(graph_time);
  LOG(DEBUG1) << "Producing graphing instructions";
  boost::unordered_map<std::string, FaultTreePtr>::const_iterator it;
  for (it = model_->fault_trees().begin(); it != model_->fault_trees().end();
       ++it) {
    const std::vector<GatePtr>* top_events = &it->second->top_events();
    std::vector<GatePtr>::const_iterator it_top;
    for (it_top = top_events->begin(); it_top != top_events->end(); ++it_top) {
      std::string output =
          it->second->name() + "_" + (*it_top)->name() + ".dot";
      std::ofstream of(output.c_str());
      if (!of.good()) {
        throw IOError(output +  " : Cannot write the graphing file.");
      }
      Grapher gr = Grapher();
      gr.GraphFaultTree(*it_top, settings_.probability_analysis_, of);
      of.flush();
    }
  }
  LOG(DEBUG1) << "Graphing instructions are produced in " << DUR(graph_time);
}

void RiskAnalysis::Analyze() {
  // Set the seed for the pseudo-random number generator if given explicitly.
  // Otherwise it defaults to the current time.
  if (settings_.seed_ >= 0) Random::seed(settings_.seed_);

  boost::unordered_map<std::string, FaultTreePtr>::const_iterator it;
  for (it = model_->fault_trees().begin(); it != model_->fault_trees().end();
       ++it) {
    const std::vector<GatePtr>* top_events = &it->second->top_events();
    std::vector<GatePtr>::const_iterator it_top;
    for (it_top = top_events->begin(); it_top != top_events->end(); ++it_top) {
      GatePtr target = *it_top;
      std::string base_path =
          target->is_public() ? "" : target->base_path() + ".";
      std::string name = base_path + target->name();  // Analysis ID.

      FaultTreeAnalysisPtr fta(new FaultTreeAnalysis(*it_top,
                                                     settings_.limit_order_,
                                                     settings_.ccf_analysis_));
      fta->Analyze();
      fault_tree_analyses_.insert(std::make_pair(name, fta));

      if (settings_.probability_analysis_) {
        ProbabilityAnalysisPtr pa(
            new ProbabilityAnalysis(settings_.approx_, settings_.num_sums_,
                                    settings_.cut_off_,
                                    settings_.importance_analysis_));
        pa->UpdateDatabase(fta->mcs_basic_events());
        pa->Analyze(fta->min_cut_sets());
        probability_analyses_.insert(std::make_pair(name, pa));
      }

      if (settings_.uncertainty_analysis_) {
        UncertaintyAnalysisPtr ua(
            new UncertaintyAnalysis(settings_.num_sums_, settings_.cut_off_,
                                    settings_.num_trials_));
        ua->UpdateDatabase(fta->mcs_basic_events());
        ua->Analyze(fta->min_cut_sets());
        uncertainty_analyses_.insert(std::make_pair(name, ua));
      }
    }
  }
}

void RiskAnalysis::Report(std::ostream& out) {
  Reporter rp = Reporter();

  // Create XML or use already created document.
  xmlpp::Document* doc = new xmlpp::Document();
  rp.SetupReport(model_, settings_, doc);

  /// Container for excess primary events not in the analysis.
  /// This container is for warning in case the input is formed not as intended.
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  std::set<PrimaryEventPtr> orphan_primary_events;
  boost::unordered_map<std::string, BasicEventPtr>::const_iterator it_b;
  for (it_b = model_->basic_events().begin();
       it_b != model_->basic_events().end(); ++it_b) {
    if (it_b->second->orphan()) orphan_primary_events.insert(it_b->second);
  }
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  boost::unordered_map<std::string, HouseEventPtr>::const_iterator it_h;
  for (it_h = model_->house_events().begin();
       it_h != model_->house_events().end(); ++it_h) {
    if (it_h->second->orphan()) orphan_primary_events.insert(it_h->second);
  }
  if (!orphan_primary_events.empty())
    rp.ReportOrphanPrimaryEvents(orphan_primary_events, doc);

  /// Container for unused parameters not in the analysis.
  /// This container is for warning in case the input is formed not as intended.
  typedef boost::shared_ptr<Parameter> ParameterPtr;
  std::set<ParameterPtr> unused_parameters;
  boost::unordered_map<std::string, ParameterPtr>::const_iterator it_v;
  for (it_v = model_->parameters().begin(); it_v != model_->parameters().end();
       ++it_v) {
    if (it_v->second->unused()) unused_parameters.insert(it_v->second);
  }

  if (!unused_parameters.empty())
    rp.ReportUnusedParameters(unused_parameters, doc);

  std::map<std::string, FaultTreeAnalysisPtr>::iterator it;
  for (it = fault_tree_analyses_.begin(); it != fault_tree_analyses_.end();
       ++it) {
    ProbabilityAnalysisPtr prob_analysis;  // Null pointer if no analysis.
    if (settings_.probability_analysis_) {
      prob_analysis = probability_analyses_.find(it->first)->second;
    }
    rp.ReportFta(it->first, fault_tree_analyses_.find(it->first)->second,
                 prob_analysis, doc);

    if (settings_.importance_analysis_) {
      rp.ReportImportance(it->first,
                          probability_analyses_.find(it->first)->second, doc);
    }

    if (settings_.uncertainty_analysis_) {
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
