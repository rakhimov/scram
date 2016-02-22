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

/// @file reporter.cc
/// Implements Reporter class.

#include "reporter.h"

#include <iomanip>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/date_time.hpp>

#include "ccf_group.h"
#include "expression.h"
#include "logger.h"
#include "version.h"

namespace scram {

namespace {

/// A generic function to convert numbers to string.
///
/// @tparam T  Numerical type.
///
/// @param[in] num  The number to be converted.
///
/// @returns Formatted string that represents the number.
template <typename T>
inline typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
ToString(T num) {
  std::stringstream ss;  // Gets better formatting than std::to_string.
  ss << num;
  return ss.str();
}

/// A helper function to convert a floating point number to string.
///
/// @param[in] num  The number to be converted.
/// @param[in] precision  Decimal precision for reporting.
///
/// @returns Formatted string that represents the floating point number.
inline std::string ToString(double num, int precision) {
  std::stringstream ss;
  ss << std::setprecision(precision) << num;
  return ss.str();
}

}  // namespace

void Reporter::Report(const RiskAnalysis& risk_an, std::ostream& out) {
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  XmlStreamElement report("report", out);
  Reporter::ReportInformation(risk_an, &report);

  CLOCK(report_time);
  LOG(DEBUG1) << "Reporting analysis results...";
  XmlStreamElement results = report.AddChild("results");
  for (const auto& fta : risk_an.fault_tree_analyses()) {
    std::string id = fta.first;
    ProbabilityAnalysis* prob_analysis = nullptr;  // Null if no analysis.
    if (risk_an.settings().probability_analysis()) {
      prob_analysis = risk_an.probability_analyses().at(id).get();
    }
    Reporter::ReportResults(id, *fta.second, prob_analysis, &results);

    if (risk_an.settings().importance_analysis()) {
      Reporter::ReportResults(id, *risk_an.importance_analyses().at(id),
                              &results);
    }

    if (risk_an.settings().uncertainty_analysis()) {
      Reporter::ReportResults(id, *risk_an.uncertainty_analyses().at(id),
                              &results);
    }
  }
  LOG(DEBUG1) << "Finished reporting in " << DUR(report_time);
}

/// Describes the fault tree analysis and techniques.
template<>
void Reporter::ReportCalculatedQuantity<FaultTreeAnalysis>(
    const Settings& settings,
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
    if (settings.algorithm() == "bdd") {
      methods.SetAttribute("name", "Binary Decision Diagram");
    } else if (settings.algorithm() == "zbdd") {
      methods.SetAttribute("name", "Zero-Suppressed Binary Decision Diagram");
    } else {
      assert(settings.algorithm() == "mocus");
      methods.SetAttribute("name", "MOCUS");
    }
    methods.AddChild("limits").AddChild("number-of-basic-events").AddChildText(
        ToString(settings.limit_order()));
  }
  if (settings.ccf_analysis()) {
    XmlStreamElement ccf_quant = information->AddChild("calculated-quantity");
    ccf_quant.SetAttribute("name", "Common Cause Failure Analysis");
    ccf_quant.SetAttribute("definition",
                           "Incorporation of common cause failure models");
  }
}

/// Describes the probability analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<ProbabilityAnalysis>(
    const Settings& settings,
    XmlStreamElement* information) {
  {
    XmlStreamElement quant = information->AddChild("calculated-quantity");
    quant.SetAttribute("name", "Probability Analysis");
    quant.SetAttribute("definition",
                       "Quantitative analysis of failure probability");
    quant.SetAttribute("approximation", settings.approximation());
  }

  XmlStreamElement methods = information->AddChild("calculation-method");
  if (settings.approximation() == "rare-event") {
    methods.SetAttribute("name", "Rare-Event Approximation");
  } else if (settings.approximation() == "mcub") {
    methods.SetAttribute("name", "MCUB Approximation");
  } else {
    assert(settings.approximation() == "no");
    methods.SetAttribute("name", "Binary Decision Diagram");
  }
  XmlStreamElement limits = methods.AddChild("limits");
  limits.AddChild("mission-time")
      .AddChildText(ToString(settings.mission_time()));
}

/// Describes the importance analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<ImportanceAnalysis>(
    const Settings& /*settings*/,
    XmlStreamElement* information) {
  XmlStreamElement quant = information->AddChild("calculated-quantity");
  quant.SetAttribute("name", "Importance Analysis");
  quant.SetAttribute("definition",
                     "Quantitative analysis of contributions and "
                     "importance factors of events.");
}

/// Describes the uncertainty analysis and techniques.
template <>
void Reporter::ReportCalculatedQuantity<UncertaintyAnalysis>(
    const Settings& settings,
    XmlStreamElement* information) {
  {
    XmlStreamElement quant = information->AddChild("calculated-quantity");
    quant.SetAttribute("name", "Uncertainty Analysis");
    quant.SetAttribute(
        "definition",
        "Calculation of uncertainties with the Monte Carlo method");
  }

  XmlStreamElement methods = information->AddChild("calculation-method");
  methods.SetAttribute("name", "Monte Carlo");
  XmlStreamElement limits = methods.AddChild("limits");
  limits.AddChild("number-of-trials")
      .AddChildText(ToString(settings.num_trials()));
  if (settings.seed() >= 0) {
    limits.AddChild("seed").AddChildText(ToString(settings.seed()));
  }
}

/// Describes all performed analyses deduced from settings.
template <>
void Reporter::ReportCalculatedQuantity<RiskAnalysis>(
    const Settings& settings,
    XmlStreamElement* information) {
  // Report the fault tree analysis by default.
  Reporter::ReportCalculatedQuantity<FaultTreeAnalysis>(settings, information);
  // Report optional analyses.
  if (settings.probability_analysis()) {
    Reporter::ReportCalculatedQuantity<ProbabilityAnalysis>(settings,
                                                            information);
  }
  if (settings.importance_analysis()) {
    Reporter::ReportCalculatedQuantity<ImportanceAnalysis>(settings,
                                                           information);
  }
  if (settings.uncertainty_analysis()) {
    Reporter::ReportCalculatedQuantity<UncertaintyAnalysis>(settings,
                                                            information);
  }
}

void Reporter::ReportInformation(const RiskAnalysis& risk_an,
                                 XmlStreamElement* report) {
  XmlStreamElement information = report->AddChild("information");
  Reporter::ReportSoftwareInformation(&information);
  Reporter::ReportPerformance(risk_an, &information);
  Reporter::ReportCalculatedQuantity(risk_an.settings(), &information);
  Reporter::ReportModelFeatures(risk_an.model(), &information);
  Reporter::ReportOrphanPrimaryEvents(risk_an.model(), &information);
  Reporter::ReportUnusedParameters(risk_an.model(), &information);
}

void Reporter::ReportSoftwareInformation(XmlStreamElement* information) {
  {
    XmlStreamElement software = information->AddChild("software");
    software.SetAttribute("name", "SCRAM");
    software.SetAttribute("version", version::core());
  }
  std::stringstream time;
  time << boost::posix_time::second_clock::local_time();
  information->AddChild("time").AddChildText(time.str());
}

void Reporter::ReportModelFeatures(const Model& model,
                                   XmlStreamElement* information) {
  XmlStreamElement model_features = information->AddChild("model-features");
  if (!model.name().empty())
    model_features.SetAttribute("name", model.name());
  model_features.AddChild("gates").AddChildText(ToString(model.gates().size()));
  model_features.AddChild("basic-events")
      .AddChildText(ToString(model.basic_events().size()));
  model_features.AddChild("house-events")
      .AddChildText(ToString(model.house_events().size()));
  model_features.AddChild("ccf-groups")
      .AddChildText(ToString(model.ccf_groups().size()));
  model_features.AddChild("fault-trees")
      .AddChildText(ToString(model.fault_trees().size()));
}

void Reporter::ReportPerformance(const RiskAnalysis& risk_an,
                                 XmlStreamElement* information) {
  // Setup for performance information.
  XmlStreamElement performance = information->AddChild("performance");
  for (const auto& fta : risk_an.fault_tree_analyses()) {
    XmlStreamElement calc_time = performance.AddChild("calculation-time");
    std::string id = fta.first;
    calc_time.SetAttribute("name", id);
    calc_time.AddChild("products")
        .AddChildText(ToString(fta.second->analysis_time(), 5));

    if (risk_an.settings().probability_analysis()) {
      calc_time.AddChild("probability").AddChildText(
          ToString(risk_an.probability_analyses().at(id)->analysis_time(), 5));
    }

    if (risk_an.settings().importance_analysis()) {
      calc_time.AddChild("importance").AddChildText(
          ToString(risk_an.importance_analyses().at(id)->analysis_time(), 5));
    }

    if (risk_an.settings().uncertainty_analysis()) {
      calc_time.AddChild("uncertainty").AddChildText(
          ToString(risk_an.uncertainty_analyses().at(id)->analysis_time(), 5));
    }
  }
}

void Reporter::ReportOrphanPrimaryEvents(const Model& model,
                                         XmlStreamElement* information) {
  std::string out = "";
  for (const std::pair<const std::string, BasicEventPtr>& entry :
       model.basic_events()) {
    const auto& param = entry.second;
    if (param->orphan()) {
      out += param->is_public() ? "" : param->base_path() + ".";
      out += param->name() + " ";
    }
  }
  for (const std::pair<const std::string, HouseEventPtr>& entry :
       model.house_events()) {
    const auto& param = entry.second;
    if (param->orphan()) {
      out += param->is_public() ? "" : param->base_path() + ".";
      out += param->name() + " ";
    }
  }
  if (!out.empty())
    information->AddChild("warning")
        .AddChildText("Orphan Primary Events: " + out);
}

void Reporter::ReportUnusedParameters(const Model& model,
                                      XmlStreamElement* information) {
  std::string out = "";
  for (const std::pair<const std::string, ParameterPtr>& entry :
       model.parameters()) {
    const auto& param = entry.second;
    if (param->unused()) {
      out += param->is_public() ? "" : param->base_path() + ".";
      out += param->name() + " ";
    }
  }
  if (!out.empty())
    information->AddChild("warning").AddChildText("Unused Parameters: " + out);
}

void Reporter::ReportResults(std::string ft_name, const FaultTreeAnalysis& fta,
                             const ProbabilityAnalysis* prob_analysis,
                             XmlStreamElement* results) {
  XmlStreamElement sum_of_products = results->AddChild("sum-of-products");
  sum_of_products.SetAttribute("name", ft_name);
  sum_of_products.SetAttribute("basic-events",
                               ToString(fta.product_events().size()));
  sum_of_products.SetAttribute("products", ToString(fta.products().size()));
  std::string warning = fta.warnings();
  if (prob_analysis) {
    sum_of_products.SetAttribute("probability",
                                 ToString(prob_analysis->p_total(), 7));
    warning += prob_analysis->warnings();
  }
  if (!warning.empty()) {
    sum_of_products.AddChild("warning").AddChildText(warning);
  }

  CLOCK(cs_time);
  LOG(DEBUG2) << "Reporting products for " << ft_name << "...";
  std::vector<double> probs;  // Product probabilities.
  double sum = 0;  // Sum of probabilities for contribution calculations.
  if (prob_analysis) {
    for (const Product& product_set : fta.products()) {
      double prob = CalculateProbability(product_set);
      sum += prob;
      probs.push_back(prob);
    }
  }  // Ugliness because FTA and Probability analysis are not integrated.
  for (int i = 0; i < fta.products().size(); ++i) {
    const Product& product_set = fta.products()[i];
    XmlStreamElement product = sum_of_products.AddChild("product");
    product.SetAttribute("order", ToString(GetOrder(product_set)));
    if (prob_analysis) {
      double prob = probs[i];
      product.SetAttribute("probability", ToString(prob, 7));
      product.SetAttribute("contribution", ToString(prob / sum, 7));
    }
    for (const Literal& literal : product_set) {
      Reporter::ReportLiteral(literal, &product);
    }
  }
  LOG(DEBUG2) << "Finished reporting products in " << DUR(cs_time);
}

void Reporter::ReportResults(std::string ft_name,
                             const ImportanceAnalysis& importance_analysis,
                             XmlStreamElement* results) {
  XmlStreamElement importance = results->AddChild("importance");
  importance.SetAttribute("name", ft_name);
  importance.SetAttribute("basic-events",
                          ToString(importance_analysis.importance().size()));
  if (!importance_analysis.warnings().empty()) {
    importance.AddChild("warning").AddChildText(importance_analysis.warnings());
  }

  for (const std::pair<BasicEventPtr, ImportanceFactors>& entry :
       importance_analysis.important_events()) {
    Reporter::ReportImportantEvent(entry.first, entry.second, &importance);
  }
}

void Reporter::ReportResults(std::string ft_name,
                             const UncertaintyAnalysis& uncert_analysis,
                             XmlStreamElement* results) {
  XmlStreamElement measure = results->AddChild("measure");
  measure.SetAttribute("name", ft_name);
  if (!uncert_analysis.warnings().empty()) {
    measure.AddChild("warning").AddChildText(uncert_analysis.warnings());
  }
  measure.AddChild("mean")
      .SetAttribute("value", ToString(uncert_analysis.mean(), 7));
  measure.AddChild("standard-deviation")
      .SetAttribute("value", ToString(uncert_analysis.sigma(), 7));
  {
    XmlStreamElement confidence = measure.AddChild("confidence-range");
    confidence.SetAttribute("percentage", "95");
    confidence.SetAttribute(
        "lower-bound",
        ToString(uncert_analysis.confidence_interval().first, 7));
    confidence.SetAttribute(
        "upper-bound",
        ToString(uncert_analysis.confidence_interval().second, 7));
  }
  {
    XmlStreamElement error_factor = measure.AddChild("error-factor");
    error_factor.SetAttribute("percentage", "95");
    error_factor.SetAttribute("value",
                              ToString(uncert_analysis.error_factor(), 7));
  }
  {
    XmlStreamElement quantiles = measure.AddChild("quantiles");
    int num_quantiles = uncert_analysis.quantiles().size();
    quantiles.SetAttribute("number", ToString(num_quantiles));
    double lower_bound = 0;
    double delta = 1.0 / num_quantiles;
    for (int i = 0; i < num_quantiles; ++i) {
      XmlStreamElement quant = quantiles.AddChild("quantile");
      quant.SetAttribute("number", ToString(i + 1));
      double upper = uncert_analysis.quantiles()[i];
      double value = delta * (i + 1);
      quant.SetAttribute("value", ToString(value, 7));
      quant.SetAttribute("lower-bound", ToString(lower_bound, 7));
      quant.SetAttribute("upper-bound", ToString(upper, 7));
      lower_bound = upper;
    }
  }
  {
    XmlStreamElement hist = measure.AddChild("histogram");
    int num_bins = uncert_analysis.distribution().size() - 1;
    hist.SetAttribute("number", ToString(num_bins));
    for (int i = 0; i < num_bins; ++i) {
      XmlStreamElement bin = hist.AddChild("bin");
      bin.SetAttribute("number", ToString(i + 1));
      double lower = uncert_analysis.distribution()[i].first;
      double upper = uncert_analysis.distribution()[i + 1].first;
      double value = uncert_analysis.distribution()[i].second;
      bin.SetAttribute("value", ToString(value, 7));
      bin.SetAttribute("lower-bound", ToString(lower, 7));
      bin.SetAttribute("upper-bound", ToString(upper, 7));
    }
  }
}

void Reporter::ReportLiteral(const Literal& literal, XmlStreamElement* parent) {
  if (literal.complement) {
    XmlStreamElement not_parent = parent->AddChild("not");
    Reporter::ReportBasicEvent(literal.event, &not_parent);
  } else {
    Reporter::ReportBasicEvent(literal.event, parent);
  }
}

void Reporter::ReportBasicEvent(const BasicEventPtr& basic_event,
                                XmlStreamElement* parent) {
  std::shared_ptr<const CcfEvent> ccf_event =
      std::dynamic_pointer_cast<const CcfEvent>(basic_event);
  std::string prefix =
        basic_event->is_public() ? "" : basic_event->base_path() + ".";
  if (!ccf_event) {
    std::string name = prefix + basic_event->name();
    XmlStreamElement element = parent->AddChild("basic-event");
    element.SetAttribute("name", name);
  } else {
    XmlStreamElement element = parent->AddChild("ccf-event");
    const CcfGroup* ccf_group = ccf_event->ccf_group();
    element.SetAttribute("ccf-group", prefix + ccf_group->name());
    element.SetAttribute("order", ToString(ccf_event->member_names().size()));
    element.SetAttribute("group-size", ToString(ccf_group->members().size()));
    for (const std::string& name : ccf_event->member_names()) {
      element.AddChild("basic-event").SetAttribute("name", name);
    }
  }
}

void Reporter::ReportImportantEvent(const BasicEventPtr& basic_event,
                                    const ImportanceFactors& factors,
                                    XmlStreamElement* parent) {
  /// @todo Refactor the code duplication.
  std::shared_ptr<const CcfEvent> ccf_event =
      std::dynamic_pointer_cast<const CcfEvent>(basic_event);
  std::string prefix =
        basic_event->is_public() ? "" : basic_event->base_path() + ".";
  if (!ccf_event) {
    std::string name = prefix + basic_event->name();
    XmlStreamElement element = parent->AddChild("basic-event");
    element.SetAttribute("name", name);
    element.SetAttribute("MIF", ToString(factors.mif, 4));
    element.SetAttribute("CIF", ToString(factors.cif, 4));
    element.SetAttribute("DIF", ToString(factors.dif, 4));
    element.SetAttribute("RAW", ToString(factors.raw, 4));
    element.SetAttribute("RRW", ToString(factors.rrw, 4));
  } else {
    XmlStreamElement element = parent->AddChild("ccf-event");
    const CcfGroup* ccf_group = ccf_event->ccf_group();
    element.SetAttribute("ccf-group", prefix + ccf_group->name());
    element.SetAttribute("order", ToString(ccf_event->member_names().size()));
    element.SetAttribute("group-size", ToString(ccf_group->members().size()));
    element.SetAttribute("MIF", ToString(factors.mif, 4));
    element.SetAttribute("CIF", ToString(factors.cif, 4));
    element.SetAttribute("DIF", ToString(factors.dif, 4));
    element.SetAttribute("RAW", ToString(factors.raw, 4));
    element.SetAttribute("RRW", ToString(factors.rrw, 4));
    for (const std::string& name : ccf_event->member_names()) {
      element.AddChild("basic-event").SetAttribute("name", name);
    }
  }
}

}  // namespace scram
