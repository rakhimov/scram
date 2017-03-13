/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file reporter.cc
/// Implements Reporter class.

#include "reporter.h"

#include <fstream>
#include <ostream>
#include <utility>
#include <vector>

#include <boost/date_time.hpp>

#include "ccf_group.h"
#include "error.h"
#include "logger.h"
#include "parameter.h"
#include "version.h"

namespace scram {

void Reporter::Report(const core::RiskAnalysis& risk_an, std::ostream& out) {
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  XmlStreamElement report("report", out);
  ReportInformation(risk_an, &report);

  CLOCK(report_time);
  LOG(DEBUG1) << "Reporting analysis results...";
  XmlStreamElement results = report.AddChild("results");
  for (const auto& fta : risk_an.fault_tree_analyses()) {
    const std::string& id = fta.first;
    core::ProbabilityAnalysis* prob_analysis = nullptr;  // Null if no analysis.
    if (risk_an.settings().probability_analysis()) {
      prob_analysis = risk_an.probability_analyses().at(id).get();
    }
    ReportResults(id, *fta.second, prob_analysis, &results);

    if (prob_analysis) {
      ReportResults(id, *prob_analysis, &results);
    }

    if (risk_an.settings().importance_analysis()) {
      ReportResults(id, *risk_an.importance_analyses().at(id), &results);
    }

    if (risk_an.settings().uncertainty_analysis()) {
      ReportResults(id, *risk_an.uncertainty_analyses().at(id), &results);
    }
  }
  LOG(DEBUG1) << "Finished reporting in " << DUR(report_time);
}

void Reporter::Report(const core::RiskAnalysis& risk_an,
                      const std::string& file) {
  std::ofstream of(file.c_str());
  if (!of.good())
    throw IOError(file + " : Cannot write the output file.");

  Report(risk_an, of);
}

/// Describes the fault tree analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<core::FaultTreeAnalysis>(
    const core::Settings& settings,
    XmlStreamElement* information) {
  {
    XmlStreamElement quant = information->AddChild("calculated-quantity");
    if (settings.prime_implicants()) {
      quant.SetAttribute("name", "Prime Implicants");
    } else {
      quant.SetAttribute("name", "Minimal Cut Sets");
    }
  }
  {
    XmlStreamElement methods = information->AddChild("calculation-method");
    switch (settings.algorithm()) {
      case core::Algorithm::kBdd:
        methods.SetAttribute("name", "Binary Decision Diagram");
        break;
      case core::Algorithm::kZbdd:
        methods.SetAttribute("name", "Zero-Suppressed Binary Decision Diagram");
        break;
      case core::Algorithm::kMocus:
        methods.SetAttribute("name", "MOCUS");
    }
    methods.AddChild("limits")
        .AddChild("product-order")
        .AddText(settings.limit_order());
  }
  if (settings.ccf_analysis()) {
    information->AddChild("calculated-quantity")
        .SetAttribute("name", "Common Cause Failure Analysis")
        .SetAttribute("definition",
                      "Incorporation of common cause failure models");
  }
}

/// Describes the probability analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<core::ProbabilityAnalysis>(
    const core::Settings& settings,
    XmlStreamElement* information) {
  information->AddChild("calculated-quantity")
      .SetAttribute("name", "Probability Analysis")
      .SetAttribute("definition",
                    "Quantitative analysis of failure probability")
      .SetAttribute("approximation",
                    core::kApproximationToString[static_cast<int>(
                        settings.approximation())]);

  XmlStreamElement methods = information->AddChild("calculation-method");
  switch (settings.approximation()) {
    case core::Approximation::kNone:
      methods.SetAttribute("name", "Binary Decision Diagram");
      break;
    case core::Approximation::kRareEvent:
      methods.SetAttribute("name", "Rare-Event Approximation");
      break;
    case core::Approximation::kMcub:
      methods.SetAttribute("name", "MCUB Approximation");
  }
  XmlStreamElement limits = methods.AddChild("limits");
  limits.AddChild("mission-time").AddText(settings.mission_time());
  if (settings.time_step())
    limits.AddChild("time-step").AddText(settings.time_step());
}

/// Describes the importance analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<core::ImportanceAnalysis>(
    const core::Settings& /*settings*/,
    XmlStreamElement* information) {
  information->AddChild("calculated-quantity")
      .SetAttribute("name", "Importance Analysis")
      .SetAttribute("definition",
                    "Quantitative analysis of contributions and "
                    "importance factors of events.");
}

/// Describes the uncertainty analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<core::UncertaintyAnalysis>(
    const core::Settings& settings,
    XmlStreamElement* information) {
  information->AddChild("calculated-quantity")
      .SetAttribute("name", "Uncertainty Analysis")
      .SetAttribute("definition",
                    "Calculation of uncertainties with the Monte Carlo method");

  XmlStreamElement methods = information->AddChild("calculation-method");
  methods.SetAttribute("name", "Monte Carlo");
  XmlStreamElement limits = methods.AddChild("limits");
  limits.AddChild("number-of-trials").AddText(settings.num_trials());
  if (settings.seed() >= 0) {
    limits.AddChild("seed").AddText(settings.seed());
  }
}

/// Describes all performed analyses deduced from settings.
template <>
void Reporter::ReportCalculatedQuantity<core::RiskAnalysis>(
    const core::Settings& settings,
    XmlStreamElement* information) {
  // Report the fault tree analysis by default.
  ReportCalculatedQuantity<core::FaultTreeAnalysis>(settings, information);
  // Report optional analyses.
  if (settings.probability_analysis()) {
    ReportCalculatedQuantity<core::ProbabilityAnalysis>(settings, information);
  }
  if (settings.safety_integrity_levels()) {
    information->AddChild("calculated-quantity")
        .SetAttribute("name", "Safety Integrity Levels");
  }
  if (settings.importance_analysis()) {
    ReportCalculatedQuantity<core::ImportanceAnalysis>(settings, information);
  }
  if (settings.uncertainty_analysis()) {
    ReportCalculatedQuantity<core::UncertaintyAnalysis>(settings, information);
  }
}

void Reporter::ReportInformation(const core::RiskAnalysis& risk_an,
                                 XmlStreamElement* report) {
  XmlStreamElement information = report->AddChild("information");
  ReportSoftwareInformation(&information);
  ReportPerformance(risk_an, &information);
  ReportCalculatedQuantity(risk_an.settings(), &information);
  ReportModelFeatures(risk_an.model(), &information);
  ReportOrphanPrimaryEvents(risk_an.model(), &information);
  ReportUnusedParameters(risk_an.model(), &information);
}

void Reporter::ReportSoftwareInformation(XmlStreamElement* information) {
  information->AddChild("software")
      .SetAttribute("name", "SCRAM")
      .SetAttribute("version", version::core());
  namespace pt = boost::posix_time;
  information->AddChild("time").AddText(
      pt::to_iso_extended_string(pt::second_clock::universal_time()));
}

void Reporter::ReportModelFeatures(const mef::Model& model,
                                   XmlStreamElement* information) {
  XmlStreamElement model_features = information->AddChild("model-features");
  if (!model.name().empty())
    model_features.SetAttribute("name", model.name());
  auto feature = [&model_features](const char* name, const auto& container) {
    if (!container.empty())
      model_features.AddChild(name).AddText(container.size());
  };
  feature("gates", model.gates());
  feature("basic-events", model.basic_events());
  feature("house-events", model.house_events());
  feature("ccf-groups", model.ccf_groups());
  feature("fault-trees", model.fault_trees());
}

void Reporter::ReportPerformance(const core::RiskAnalysis& risk_an,
                                 XmlStreamElement* information) {
  // Setup for performance information.
  XmlStreamElement performance = information->AddChild("performance");
  for (const auto& fta : risk_an.fault_tree_analyses()) {
    XmlStreamElement calc_time = performance.AddChild("calculation-time");
    const std::string& id = fta.first;
    calc_time.SetAttribute("name", id);
    calc_time.AddChild("products").AddText(fta.second->analysis_time());

    if (risk_an.settings().probability_analysis()) {
      calc_time.AddChild("probability")
          .AddText(risk_an.probability_analyses().at(id)->analysis_time());
    }

    if (risk_an.settings().importance_analysis()) {
      calc_time.AddChild("importance")
          .AddText(risk_an.importance_analyses().at(id)->analysis_time());
    }

    if (risk_an.settings().uncertainty_analysis()) {
      calc_time.AddChild("uncertainty")
          .AddText(risk_an.uncertainty_analyses().at(id)->analysis_time());
    }
  }
}

void Reporter::ReportOrphanPrimaryEvents(const mef::Model& model,
                                         XmlStreamElement* information) {
  std::string out;
  for (const mef::BasicEventPtr& param : model.basic_events()) {
    if (param->orphan())
      out += param->id() + " ";
  }
  for (const mef::HouseEventPtr& param : model.house_events()) {
    if (param->orphan())
      out += param->id() + " ";
  }
  if (!out.empty())
    information->AddChild("warning").AddText("Orphan Primary Events: " + out);
}

void Reporter::ReportUnusedParameters(const mef::Model& model,
                                      XmlStreamElement* information) {
  std::string out;
  for (const mef::ParameterPtr& param : model.parameters()) {
    if (param->unused())
      out += param->id() + " ";
  }
  if (!out.empty())
    information->AddChild("warning").AddText("Unused Parameters: " + out);
}

void Reporter::ReportResults(const std::string& ft_name,
                             const core::FaultTreeAnalysis& fta,
                             const core::ProbabilityAnalysis* prob_analysis,
                             XmlStreamElement* results) {
  XmlStreamElement sum_of_products = results->AddChild("sum-of-products");
  sum_of_products.SetAttribute("name", ft_name)
      .SetAttribute("basic-events", fta.products().product_events().size())
      .SetAttribute("products", fta.products().size());

  std::string warning = fta.warnings();
  if (prob_analysis) {
    sum_of_products.SetAttribute("probability", prob_analysis->p_total());
    warning += prob_analysis->warnings();
  }
  if (!warning.empty()) {
    sum_of_products.AddChild("warning").AddText(warning);
  }

  CLOCK(cs_time);
  LOG(DEBUG2) << "Reporting products for " << ft_name << "...";
  double sum = 0;  // Sum of probabilities for contribution calculations.
  if (prob_analysis) {
    for (const core::Product& product_set : fta.products())
      sum += product_set.p();
  }
  for (const core::Product& product_set : fta.products()) {
    XmlStreamElement product = sum_of_products.AddChild("product");
    product.SetAttribute("order", product_set.order());
    if (prob_analysis) {
      double prob = product_set.p();
      product.SetAttribute("probability", prob);
      product.SetAttribute("contribution", prob / sum);
    }
    for (const core::Literal& literal : product_set) {
      ReportLiteral(literal, &product);
    }
  }
  LOG(DEBUG2) << "Finished reporting products in " << DUR(cs_time);
}

void Reporter::ReportResults(const std::string& ft_name,
                             const core::ProbabilityAnalysis& prob_analysis,
                             XmlStreamElement* results) {
  if (!prob_analysis.p_time().empty()) {
    XmlStreamElement curve = results->AddChild("curve");
    curve.SetAttribute("name", ft_name)
        .SetAttribute("description", "Probability values over time")
        .SetAttribute("X-title", "Mission time")
        .SetAttribute("Y-title", "Probability")
        .SetAttribute("X-unit", "hours");
    for (const std::pair<double, double>& p_vs_time : prob_analysis.p_time()) {
      curve.AddChild("point")
          .SetAttribute("X", p_vs_time.second)
          .SetAttribute("Y", p_vs_time.first);
    }
  }

  if (prob_analysis.settings().safety_integrity_levels()) {
    XmlStreamElement sil = results->AddChild("safety-integrity-levels");
    sil.SetAttribute("name", ft_name)
        .SetAttribute("PFD-avg", prob_analysis.sil().pfd_avg)
        .SetAttribute("PFH-avg", prob_analysis.sil().pfh_avg);
    auto report_sil_fractions = [&sil](const auto& sil_fractions) {
      XmlStreamElement hist = sil.AddChild("histogram");
      hist.SetAttribute("number", sil_fractions.size());
      double b_0 = 0;
      int bin_number = 1;
      for (const std::pair<const double, double>& sil_bucket : sil_fractions) {
        double b_1 = sil_bucket.first;
        hist.AddChild("bin")
            .SetAttribute("number", bin_number++)
            .SetAttribute("value", sil_bucket.second)
            .SetAttribute("lower-bound", b_0)
            .SetAttribute("upper-bound", b_1);
        b_0 = b_1;
      }
    };
    report_sil_fractions(prob_analysis.sil().pfd_fractions);
    report_sil_fractions(prob_analysis.sil().pfh_fractions);
  }
}

void Reporter::ReportResults(
    const std::string& ft_name,
    const core::ImportanceAnalysis& importance_analysis,
    XmlStreamElement* results) {
  XmlStreamElement importance = results->AddChild("importance");
  importance.SetAttribute("name", ft_name);
  importance.SetAttribute("basic-events",
                          importance_analysis.importance().size());
  if (!importance_analysis.warnings().empty()) {
    importance.AddChild("warning").AddText(importance_analysis.warnings());
  }

  for (const core::ImportanceRecord& entry : importance_analysis.importance()) {
    const core::ImportanceFactors& factors = entry.factors;
    const mef::BasicEvent& event = entry.event;
    auto add_data = [&event, &factors](XmlStreamElement* element) {
      element->SetAttribute("occurrence", factors.occurrence)
          .SetAttribute("probability", event.p())
          .SetAttribute("MIF", factors.mif)
          .SetAttribute("CIF", factors.cif)
          .SetAttribute("DIF", factors.dif)
          .SetAttribute("RAW", factors.raw)
          .SetAttribute("RRW", factors.rrw);
    };
    ReportBasicEvent(event, &importance, add_data);
  }
}

void Reporter::ReportResults(const std::string& ft_name,
                             const core::UncertaintyAnalysis& uncert_analysis,
                             XmlStreamElement* results) {
  XmlStreamElement measure = results->AddChild("measure");
  measure.SetAttribute("name", ft_name);
  if (!uncert_analysis.warnings().empty()) {
    measure.AddChild("warning").AddText(uncert_analysis.warnings());
  }
  measure.AddChild("mean").SetAttribute("value", uncert_analysis.mean());
  measure.AddChild("standard-deviation")
      .SetAttribute("value", uncert_analysis.sigma());

  measure.AddChild("confidence-range")
      .SetAttribute("percentage", "95")
      .SetAttribute("lower-bound", uncert_analysis.confidence_interval().first)
      .SetAttribute("upper-bound",
                    uncert_analysis.confidence_interval().second);

  measure.AddChild("error-factor")
      .SetAttribute("percentage", "95")
      .SetAttribute("value", uncert_analysis.error_factor());
  {
    XmlStreamElement quantiles = measure.AddChild("quantiles");
    int num_quantiles = uncert_analysis.quantiles().size();
    quantiles.SetAttribute("number", num_quantiles);
    double prev_bound = 0;
    double delta = 1.0 / num_quantiles;
    for (int i = 0; i < num_quantiles; ++i) {
      double upper = uncert_analysis.quantiles()[i];
      double value = delta * (i + 1);
      quantiles.AddChild("quantile")
          .SetAttribute("number", i + 1)
          .SetAttribute("value", value)
          .SetAttribute("lower-bound", prev_bound)
          .SetAttribute("upper-bound", upper);
      prev_bound = upper;
    }
  }
  {
    XmlStreamElement hist = measure.AddChild("histogram");
    int num_bins = uncert_analysis.distribution().size() - 1;
    hist.SetAttribute("number", num_bins);
    for (int i = 0; i < num_bins; ++i) {
      double lower = uncert_analysis.distribution()[i].first;
      double upper = uncert_analysis.distribution()[i + 1].first;
      double value = uncert_analysis.distribution()[i].second;
      hist.AddChild("bin")
          .SetAttribute("number", i + 1)
          .SetAttribute("value", value)
          .SetAttribute("lower-bound", lower)
          .SetAttribute("upper-bound", upper);
    }
  }
}

void Reporter::ReportLiteral(const core::Literal& literal,
                             XmlStreamElement* parent) {
  auto add_data = [](XmlStreamElement* /*element*/) {};
  if (literal.complement) {
    XmlStreamElement not_parent = parent->AddChild("not");
    ReportBasicEvent(literal.event, &not_parent, add_data);
  } else {
    ReportBasicEvent(literal.event, parent, add_data);
  }
}

template <class T>
void Reporter::ReportBasicEvent(const mef::BasicEvent& basic_event,
                                XmlStreamElement* parent, const T& add_data) {
  const auto* ccf_event = dynamic_cast<const mef::CcfEvent*>(&basic_event);
  if (!ccf_event) {
    XmlStreamElement element = parent->AddChild("basic-event");
    element.SetAttribute("name", basic_event.id());
    add_data(&element);
  } else {
    const mef::CcfGroup& ccf_group = ccf_event->ccf_group();
    XmlStreamElement element = parent->AddChild("ccf-event");
    element.SetAttribute("ccf-group", ccf_group.id())
        .SetAttribute("order", ccf_event->members().size())
        .SetAttribute("group-size", ccf_group.members().size());
    add_data(&element);
    for (const mef::Gate* member : ccf_event->members()) {
      element.AddChild("basic-event").SetAttribute("name", member->name());
    }
  }
}

}  // namespace scram
