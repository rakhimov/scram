/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// Implements Reporter class.

#include "reporter.h"

#include <ctime>

#include <memory>
#include <utility>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_file_open_mode.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "ccf_group.h"
#include "element.h"
#include "error.h"
#include "logger.h"
#include "parameter.h"
#include "version.h"

namespace scram {

namespace {

/// Puts analysis id into report XML element.
void PutId(const core::RiskAnalysis::Result::Id& id,
           xml::StreamElement* report) {
  struct {
    void operator()(const mef::Gate* gate) {
      report_->SetAttribute("name", gate->id());
    }
    void operator()(const std::pair<const mef::InitiatingEvent&,
                                    const mef::Sequence&>& sequence) {
      report_->SetAttribute("initiating-event", sequence.first.name());
      report_->SetAttribute("name", sequence.second.name());
    }
    xml::StreamElement* report_;
  } extractor{report};
  std::visit(extractor, id.target);

  if (id.context) {
    report->SetAttribute("alignment", id.context->alignment.name());
    report->SetAttribute("phase", id.context->phase.name());
  }
}

}  // namespace

void Reporter::Report(const core::RiskAnalysis& risk_an, std::FILE* out,
                      bool indent) {
  xml::Stream xml_stream(out, indent);
  xml::StreamElement report = xml_stream.root("report");
  ReportInformation(risk_an, &report);

  if (risk_an.results().empty() && risk_an.event_tree_results().empty())
    return;
  TIMER(DEBUG1, "Reporting analysis results");
  xml::StreamElement results = report.AddChild("results");
  if (risk_an.settings().probability_analysis()) {
    for (const core::RiskAnalysis::EtaResult& result :
         risk_an.event_tree_results()) {
      ReportResults(result, &results);
    }
  }

  for (const core::RiskAnalysis::Result& result : risk_an.results()) {
    if (result.fault_tree_analysis)
      ReportResults(result.id, *result.fault_tree_analysis,
                    result.probability_analysis.get(), &results);

    if (result.probability_analysis)
      ReportResults(result.id, *result.probability_analysis, &results);

    if (result.importance_analysis)
      ReportResults(result.id, *result.importance_analysis, &results);

    if (result.uncertainty_analysis)
      ReportResults(result.id, *result.uncertainty_analysis, &results);
  }
}

void Reporter::Report(const core::RiskAnalysis& risk_an,
                      const std::string& file, bool indent) {
  std::unique_ptr<std::FILE, decltype(&std::fclose)> fp(
      std::fopen(file.c_str(), "w"), &std::fclose);
  try {
    if (!fp) {
      SCRAM_THROW(IOError("Cannot open the output file for report."))
          << boost::errinfo_errno(errno) << boost::errinfo_file_open_mode("w");
    }
    Report(risk_an, fp.get(), indent);
  } catch (IOError& err) {
    err << boost::errinfo_file_name(file);
    throw;
  }
}

/// Describes the fault tree analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<core::FaultTreeAnalysis>(
    const core::Settings& settings, xml::StreamElement* information) {
  {
    xml::StreamElement quant = information->AddChild("calculated-quantity");
    if (settings.prime_implicants()) {
      quant.SetAttribute("name", "Prime Implicants");
    } else {
      quant.SetAttribute("name", "Minimal Cut Sets");
    }
    xml::StreamElement methods = quant.AddChild("calculation-method");
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
    const core::Settings& settings, xml::StreamElement* information) {
  xml::StreamElement quant = information->AddChild("calculated-quantity");
  quant.SetAttribute("name", "Probability Analysis")
      .SetAttribute("definition",
                    "Quantitative analysis of"
                    " failure probability or unavailability")
      .SetAttribute("approximation",
                    core::kApproximationToString[static_cast<int>(
                        settings.approximation())]);

  xml::StreamElement methods = quant.AddChild("calculation-method");
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
  xml::StreamElement limits = methods.AddChild("limits");
  limits.AddChild("mission-time").AddText(settings.mission_time());
  if (settings.time_step())
    limits.AddChild("time-step").AddText(settings.time_step());
}

/// Describes the importance analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<core::ImportanceAnalysis>(
    const core::Settings& /*settings*/, xml::StreamElement* information) {
  information->AddChild("calculated-quantity")
      .SetAttribute("name", "Importance Analysis")
      .SetAttribute("definition",
                    "Quantitative analysis of contributions and "
                    "importance factors of events.");
}

/// Describes the uncertainty analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<core::UncertaintyAnalysis>(
    const core::Settings& settings, xml::StreamElement* information) {
  xml::StreamElement quant = information->AddChild("calculated-quantity");
  quant.SetAttribute("name", "Uncertainty Analysis")
      .SetAttribute("definition",
                    "Calculation of uncertainties with the Monte Carlo method");

  xml::StreamElement methods = quant.AddChild("calculation-method");
  methods.SetAttribute("name", "Monte Carlo");
  xml::StreamElement limits = methods.AddChild("limits");
  limits.AddChild("number-of-trials").AddText(settings.num_trials());
  if (settings.seed() >= 0) {
    limits.AddChild("seed").AddText(settings.seed());
  }
}

/// Describes all performed analyses deduced from settings.
template <>
void Reporter::ReportCalculatedQuantity<core::RiskAnalysis>(
    const core::Settings& settings, xml::StreamElement* information) {
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
                                 xml::StreamElement* report) {
  xml::StreamElement information = report->AddChild("information");
  ReportSoftwareInformation(&information);
  ReportPerformance(risk_an, &information);
  ReportCalculatedQuantity(risk_an.settings(), &information);
  ReportModelFeatures(risk_an.model(), &information);
  ReportUnusedElements(risk_an.model().basic_events(),
                       "Unused basic events: ", &information);
  ReportUnusedElements(risk_an.model().house_events(),
                       "Unused house events: ", &information);
  ReportUnusedElements(risk_an.model().parameters(),
                       "Unused parameters: ", &information);
  ReportUnusedElements(risk_an.model().libraries(),
                       "Unused libraries: ", &information);
  ReportUnusedElements(risk_an.model().extern_functions(),
                       "Unused extern functions: ", &information);
  ReportUnusedElements(risk_an.model().initiating_events(),
                       "Unused initiating events: ", &information);
  ReportUnusedElements(risk_an.model().event_trees(),
                       "Unused event trees: ", &information);
  ReportUnusedElements(risk_an.model().sequences(),
                       "Unused sequences: ", &information);
  ReportUnusedElements(risk_an.model().rules(), "Unused rules: ", &information);
  for (const mef::EventTree& event_tree : risk_an.model().event_trees()) {
    std::string header = "In event tree " + event_tree.name() + ", ";
    ReportUnusedElements(event_tree.branches(),
                         header + "unused branches: ", &information);
    ReportUnusedElements(event_tree.functional_events(),
                         header + "unused functional events: ", &information);
  }
}

void Reporter::ReportSoftwareInformation(xml::StreamElement* information) {
  information->AddChild("software")
      .SetAttribute("name", "SCRAM")
      .SetAttribute("version", *SCRAM_GIT_REVISION != '\0' ? SCRAM_GIT_REVISION
                                                           : SCRAM_VERSION)
      .SetAttribute("contacts", "https://scram-pra.org");

  std::time_t current_time = std::time(nullptr);
  char iso_extended[20] = {};
  auto ret = std::strftime(iso_extended, sizeof(iso_extended),
                           "%Y-%m-%dT%H:%M:%S", std::gmtime(&current_time));
  assert(ret && "Time formatting failure. Who is running this in year 10000?");
  if (ret)  // The user can set the wall-clock year beyond 10k.
    information->AddChild("time").AddText(iso_extended);
}

void Reporter::ReportModelFeatures(const mef::Model& model,
                                   xml::StreamElement* information) {
  xml::StreamElement model_features = information->AddChild("model-features");
  if (!model.HasDefaultName())
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
  feature("event-trees", model.event_trees());

  int num_functional_events = 0;
  for (const mef::EventTree& event_tree : model.event_trees())
    num_functional_events += event_tree.functional_events().size();
  if (num_functional_events)
    model_features.AddChild("functional-events").AddText(num_functional_events);

  feature("sequences", model.sequences());
  feature("rules", model.rules());
  feature("initiating-events", model.initiating_events());
  feature("substitutions", model.substitutions());
}

void Reporter::ReportPerformance(const core::RiskAnalysis& risk_an,
                                 xml::StreamElement* information) {
  if (risk_an.results().empty())
    return;
  // Setup for performance information.
  xml::StreamElement performance = information->AddChild("performance");
  for (const core::RiskAnalysis::Result& result : risk_an.results()) {
    xml::StreamElement calc_time = performance.AddChild("calculation-time");
    scram::PutId(result.id, &calc_time);
    if (result.fault_tree_analysis)
      calc_time.AddChild("products")
          .AddText(result.fault_tree_analysis->analysis_time());

    if (result.probability_analysis)
      calc_time.AddChild("probability")
          .AddText(result.probability_analysis->analysis_time());

    if (result.importance_analysis)
      calc_time.AddChild("importance")
          .AddText(result.importance_analysis->analysis_time());

    if (result.uncertainty_analysis)
      calc_time.AddChild("uncertainty")
          .AddText(result.uncertainty_analysis->analysis_time());
  }
}

template <class T>
void Reporter::ReportUnusedElements(const T& container,
                                    const std::string& header,
                                    xml::StreamElement* information) {
  std::string out = boost::join(
      container | boost::adaptors::filtered([](const auto& arg) {
        return !arg.usage();
      }) | boost::adaptors::transformed([](const auto& arg) -> decltype(auto) {
        return mef::Id::unique_name(arg);
      }),
      " ");
  if (!out.empty())
    information->AddChild("warning").AddText(header + out);
}

void Reporter::ReportResults(const core::RiskAnalysis::EtaResult& eta_result,
                             xml::StreamElement* results) {
  const core::EventTreeAnalysis& eta = *eta_result.event_tree_analysis;
  xml::StreamElement initiating_event = results->AddChild("initiating-event");
  initiating_event.SetAttribute("name", eta.initiating_event().name());

  if (eta_result.context) {
    initiating_event
        .SetAttribute("alignment", eta_result.context->alignment.name())
        .SetAttribute("phase", eta_result.context->phase.name());
  }

  initiating_event.SetAttribute("sequences", eta.sequences().size());
  for (const core::EventTreeAnalysis::Result& result_sequence :
       eta.sequences()) {
    initiating_event.AddChild("sequence")
        .SetAttribute("name", result_sequence.sequence.name())
        .SetAttribute("value", result_sequence.p_sequence);
  }
}

void Reporter::ReportResults(const core::RiskAnalysis::Result::Id& id,
                             const core::FaultTreeAnalysis& fta,
                             const core::ProbabilityAnalysis* prob_analysis,
                             xml::StreamElement* results) {
  TIMER(DEBUG2, "Reporting products");
  xml::StreamElement sum_of_products = results->AddChild("sum-of-products");
  scram::PutId(id, &sum_of_products);

  std::string warning = fta.warnings();
  if (prob_analysis && prob_analysis->warnings().empty() == false)
    warning += (warning.empty() ? "" : "; ") + prob_analysis->warnings();
  if (!warning.empty())
    sum_of_products.SetAttribute("warning", warning);

  sum_of_products
      .SetAttribute("basic-events", fta.products().product_events().size())
      .SetAttribute("products", fta.products().size());

  if (prob_analysis)
    sum_of_products.SetAttribute("probability", prob_analysis->p_total());

  if (fta.products().empty() == false) {
    sum_of_products.SetAttribute(
        "distribution",
        boost::join(fta.products().distribution() |
                        boost::adaptors::transformed(
                            [](int number) { return std::to_string(number); }),
                    " "));
  }

  double sum = 0;  // Sum of probabilities for contribution calculations.
  if (prob_analysis) {
    for (const core::Product& product_set : fta.products())
      sum += product_set.p();
  }
  for (const core::Product& product_set : fta.products()) {
    xml::StreamElement product = sum_of_products.AddChild("product");
    product.SetAttribute("order", product_set.order());
    if (prob_analysis) {
      double prob = product_set.p();
      product.SetAttribute("probability", prob);
      if (sum != 0)
        product.SetAttribute("contribution", prob / sum);
    }
    for (const core::Literal& literal : product_set) {
      ReportLiteral(literal, &product);
    }
  }
}

void Reporter::ReportResults(const core::RiskAnalysis::Result::Id& id,
                             const core::ProbabilityAnalysis& prob_analysis,
                             xml::StreamElement* results) {
  if (!prob_analysis.p_time().empty()) {
    xml::StreamElement curve = results->AddChild("curve");
    scram::PutId(id, &curve);
    curve.SetAttribute("description", "Probability values over time")
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
    xml::StreamElement sil = results->AddChild("safety-integrity-levels");
    scram::PutId(id, &sil);
    sil.SetAttribute("PFD-avg", prob_analysis.sil().pfd_avg)
        .SetAttribute("PFH-avg", prob_analysis.sil().pfh_avg);
    auto report_sil_fractions = [&sil](const auto& sil_fractions) {
      xml::StreamElement hist = sil.AddChild("histogram");
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
    const core::RiskAnalysis::Result::Id& id,
    const core::ImportanceAnalysis& importance_analysis,
    xml::StreamElement* results) {
  xml::StreamElement importance = results->AddChild("importance");
  scram::PutId(id, &importance);
  if (!importance_analysis.warnings().empty()) {
    importance.SetAttribute("warning", importance_analysis.warnings());
  }
  importance.SetAttribute("basic-events",
                          importance_analysis.importance().size());

  for (const core::ImportanceRecord& entry : importance_analysis.importance()) {
    const core::ImportanceFactors& factors = entry.factors;
    const mef::BasicEvent& event = entry.event;
    auto add_data = [&event, &factors](xml::StreamElement* element) {
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

void Reporter::ReportResults(const core::RiskAnalysis::Result::Id& id,
                             const core::UncertaintyAnalysis& uncert_analysis,
                             xml::StreamElement* results) {
  xml::StreamElement measure = results->AddChild("measure");
  scram::PutId(id, &measure);
  if (!uncert_analysis.warnings().empty()) {
    measure.SetAttribute("warning", uncert_analysis.warnings());
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
    xml::StreamElement quantiles = measure.AddChild("quantiles");
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
    xml::StreamElement hist = measure.AddChild("histogram");
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
                             xml::StreamElement* parent) {
  auto add_data = [](xml::StreamElement* /*element*/) {};
  if (literal.complement) {
    xml::StreamElement not_parent = parent->AddChild("not");
    ReportBasicEvent(literal.event, &not_parent, add_data);
  } else {
    ReportBasicEvent(literal.event, parent, add_data);
  }
}

template <class T>
void Reporter::ReportBasicEvent(const mef::BasicEvent& basic_event,
                                xml::StreamElement* parent, const T& add_data) {
  const auto* ccf_event = dynamic_cast<const mef::CcfEvent*>(&basic_event);
  if (!ccf_event) {
    xml::StreamElement element = parent->AddChild("basic-event");
    element.SetAttribute("name", basic_event.id());
    add_data(&element);
  } else {
    const mef::CcfGroup& ccf_group = ccf_event->ccf_group();
    xml::StreamElement element = parent->AddChild("ccf-event");
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
