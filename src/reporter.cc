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
#include <numeric>
#include <sstream>
#include <type_traits>
#include <utility>

#include <boost/date_time.hpp>

#include "ccf_group.h"
#include "error.h"
#include "event.h"
#include "expression.h"
#include "fault_tree_analysis.h"
#include "importance_analysis.h"
#include "logger.h"
#include "model.h"
#include "probability_analysis.h"
#include "risk_analysis.h"
#include "settings.h"
#include "uncertainty_analysis.h"
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

void Reporter::SetupReport(const std::shared_ptr<const Model>& model,
                           const Settings& settings,
                           xmlpp::Document* doc) {
  if (doc->get_root_node()) throw LogicError("The document is not empty.");
  xmlpp::Node* root = doc->create_root_node("report");
  // Add an information node.
  xmlpp::Element* information = root->add_child("information");
  xmlpp::Element* software = information->add_child("software");
  software->set_attribute("name", "SCRAM");
  software->set_attribute("version", version::core());
  std::stringstream time;
  time << boost::posix_time::second_clock::local_time();
  information->add_child("time")->add_child_text(time.str());
  // Setup for performance information.
  information->add_child("performance");

  // Report the setup for main analysis.
  xmlpp::Element* quant = information->add_child("calculated-quantity");
  if (settings.prime_implicants()) {
    quant->set_attribute("name", "Prime Implicants");
  } else {
    quant->set_attribute("name", "Minimal Cut Sets");
  }

  xmlpp::Element* methods = information->add_child("calculation-method");
  if (settings.algorithm() == "bdd") {
    methods->set_attribute("name", "Binary Decision Diagram");
  } else if (settings.algorithm() == "zbdd") {
    methods->set_attribute("name", "Zero-Suppressed Binary Decision Diagram");
  } else {
    assert(settings.algorithm() == "mocus");
    methods->set_attribute("name", "MOCUS");
  }
  methods->add_child("limits")->add_child("number-of-basic-events")
      ->add_child_text(ToString(settings.limit_order()));

  // Report the setup for CCF analysis.
  if (settings.ccf_analysis()) {
    quant = information->add_child("calculated-quantity");
    quant->set_attribute("name", "Common Cause Failure Analysis");
    quant->set_attribute("definition",
                         "Incorporation of common cause failure models");
  }

  // Report the setup for probability analysis.
  if (settings.probability_analysis()) {
    quant = information->add_child("calculated-quantity");
    quant->set_attribute("name", "Probability Analysis");
    quant->set_attribute("definition",
                         "Quantitative analysis of failure probability");
    quant->set_attribute("approximation", settings.approximation());

    methods = information->add_child("calculation-method");

    if (settings.approximation() == "rare-event") {
      information->add_child("warning")->add_child_text(
          " The rare-event approximation may be inaccurate for analysis"
          " if cut sets' probabilities exceed 0.1.");
      methods->set_attribute("name", "Rare-Event Approximation");
    } else if (settings.approximation() == "mcub") {
      information->add_child("warning")->add_child_text(
          " The MCUB approximation may not hold"
          " if the fault tree is non-coherent"
          " or there are many common events.");
      methods->set_attribute("name", "MCUB Approximation");
    } else {
      assert(settings.approximation() == "no");
      methods->set_attribute("name", "Binary Decision Diagram");
    }
    xmlpp::Element* limits = methods->add_child("limits");
    limits->add_child("mission-time")
        ->add_child_text(ToString(settings.mission_time()));
  }

  // Report the setup for optional importance analysis.
  if (settings.importance_analysis()) {
    quant = information->add_child("calculated-quantity");
    quant->set_attribute("name", "Importance Analysis");
    quant->set_attribute("definition",
                         "Quantitative analysis of contributions and "
                         "importance factors of events.");
  }

  // Report the setup for optional uncertainty analysis.
  if (settings.uncertainty_analysis()) {
    quant = information->add_child("calculated-quantity");
    quant->set_attribute("name", "Uncertainty Analysis");
    quant->set_attribute(
        "definition",
        "Calculation of uncertainties with the Monte Carlo method");

    methods = information->add_child("calculation-method");
    methods->set_attribute("name", "Monte Carlo");
    xmlpp::Element* limits = methods->add_child("limits");
    limits->add_child("number-of-trials")
        ->add_child_text(ToString(settings.num_trials()));
    if (settings.seed() >= 0) {
      limits->add_child("seed")->add_child_text(ToString(settings.seed()));
    }
  }

  xmlpp::Element* model_features = information->add_child("model-features");
  if (!model->name().empty())
    model_features->set_attribute("name", model->name());
  model_features->add_child("gates")
      ->add_child_text(ToString(model->gates().size()));
  model_features->add_child("basic-events")
      ->add_child_text(ToString(model->basic_events().size()));
  model_features->add_child("house-events")
      ->add_child_text(ToString(model->house_events().size()));
  model_features->add_child("ccf-groups")
      ->add_child_text(ToString(model->ccf_groups().size()));
  model_features->add_child("fault-trees")
      ->add_child_text(ToString(model->fault_trees().size()));

  // Setup for results.
  root->add_child("results");
}

void Reporter::ReportOrphanPrimaryEvents(
    const std::vector<PrimaryEventPtr>& orphan_primary_events,
    xmlpp::Document* doc) {
  if (orphan_primary_events.empty()) return;
  std::string out = "WARNING! Orphan Primary Events: ";
  for (const auto& event : orphan_primary_events) {
    out += event->is_public() ? "" : event->base_path() + ".";
    out += event->name();
    out += " ";
  }
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet inf = root->find("./information");
  assert(inf.size() == 1);
  xmlpp::Element* information = static_cast<xmlpp::Element*>(inf[0]);
  information->add_child("warning")->add_child_text(out);
}

void Reporter::ReportUnusedParameters(
    const std::vector<std::shared_ptr<const Parameter>>& unused_parameters,
    xmlpp::Document* doc) {
  if (unused_parameters.empty()) return;
  std::string out = "WARNING! Unused Parameters: ";
  for (const auto param : unused_parameters) {
    out += param->is_public() ? "" : param->base_path() + ".";
    out += param->name();
    out += " ";
  }
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet inf = root->find("./information");
  assert(inf.size() == 1);
  xmlpp::Element* information = static_cast<xmlpp::Element*>(inf[0]);
  information->add_child("warning")->add_child_text(out);
}

void Reporter::ReportFta(std::string ft_name, const FaultTreeAnalysis& fta,
                         const ProbabilityAnalysis* prob_analysis,
                         xmlpp::Document* doc) {
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet res = root->find("./results");
  assert(res.size() == 1);
  xmlpp::Element* results = static_cast<xmlpp::Element*>(res[0]);
  xmlpp::Element* sum_of_products = results->add_child("sum-of-products");
  sum_of_products->set_attribute("name", ft_name);
  sum_of_products->set_attribute("basic-events",
                                 ToString(fta.product_events().size()));
  sum_of_products->set_attribute("products",
                                 ToString(fta.products().size()));

  std::string warning = fta.warnings();
  if (prob_analysis) warning += prob_analysis->warnings();
  if (warning != "") {
    sum_of_products->add_child("warning")->add_child_text(warning);
  }

  CLOCK(cs_time);
  LOG(DEBUG2) << "Reporting products for " << ft_name << "...";
  std::vector<xmlpp::Element*> products;  // To add more info later.
  for (const Product& product_set : fta.products()) {
    xmlpp::Element* product = sum_of_products->add_child("product");
    products.push_back(product);
    product->set_attribute("order", ToString(GetOrder(product_set)));
    for (const Literal& literal : product_set) {
      xmlpp::Element* parent = product;
      if (literal.complement) parent = product->add_child("not");
      Reporter::ReportBasicEvent(literal.event, parent);
    }
  }
  if (prob_analysis) {
    sum_of_products->set_attribute("probability",
                                   ToString(prob_analysis->p_total(), 7));

    std::vector<double> probs;  // Product probabilities.
    for (const Product& product_set : fta.products())
      probs.push_back(CalculateProbability(product_set));

    double sum = std::accumulate(probs.begin(), probs.end(), 0.0);
    assert(products.size() == probs.size() && "Elements are missing!");
    for (int i = 0; i < products.size(); ++i) {
      auto* product = products[i];
      double prob = probs[i];
      product->set_attribute("probability", ToString(prob, 7));
      product->set_attribute("contribution", ToString(prob / sum, 7));
    }
  }
  LOG(DEBUG2) << "Finished reporting products in " << DUR(cs_time);

  // Report calculation time in the information section.
  // It is assumed that MCS reporting is the default
  // and the first thing to be reported.
  xmlpp::NodeSet perf = root->find("./information/performance");
  assert(perf.size() == 1);
  xmlpp::Element* performance = static_cast<xmlpp::Element*>(perf[0]);
  xmlpp::Element* calc_time = performance->add_child("calculation-time");
  calc_time->set_attribute("name", ft_name);
  calc_time->add_child("products")
      ->add_child_text(ToString(fta.analysis_time(), 5));
  if (prob_analysis) {
    calc_time->add_child("probability")
        ->add_child_text(ToString(prob_analysis->analysis_time(), 5));
  }
}

void Reporter::ReportImportance(std::string ft_name,
                                const ImportanceAnalysis& importance_analysis,
                                xmlpp::Document* doc) {
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet res = root->find("./results");
  assert(res.size() == 1);
  xmlpp::Element* results = static_cast<xmlpp::Element*>(res[0]);
  xmlpp::Element* importance = results->add_child("importance");
  importance->set_attribute("name", ft_name);
  importance->set_attribute("basic-events",
                            ToString(importance_analysis.importance().size()));

  std::string warning = importance_analysis.warnings();
  if (warning != "") {
    importance->add_child("warning")->add_child_text(warning);
  }

  for (const std::pair<BasicEventPtr, ImportanceFactors>& entry :
       importance_analysis.important_events()) {
    xmlpp::Element* element =
        Reporter::ReportBasicEvent(entry.first, importance);
    const ImportanceFactors& factors = entry.second;
    element->set_attribute("MIF", ToString(factors.mif, 4));
    element->set_attribute("CIF", ToString(factors.cif, 4));
    element->set_attribute("DIF", ToString(factors.dif, 4));
    element->set_attribute("RAW", ToString(factors.raw, 4));
    element->set_attribute("RRW", ToString(factors.rrw, 4));
  }
  xmlpp::NodeSet calc_times =
      root->find("./information/performance/calculation-time");
  assert(!calc_times.empty());
  xmlpp::Element* calc_time = static_cast<xmlpp::Element*>(calc_times.back());
  calc_time->add_child("importance")
      ->add_child_text(ToString(importance_analysis.analysis_time(), 5));
}

void Reporter::ReportUncertainty(std::string ft_name,
                                 const UncertaintyAnalysis& uncert_analysis,
                                 xmlpp::Document* doc) {
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet res = root->find("./results");
  assert(res.size() == 1);
  xmlpp::Element* results = static_cast<xmlpp::Element*>(res[0]);
  xmlpp::Element* measure = results->add_child("measure");
  measure->set_attribute("name", ft_name);

  std::string warning = uncert_analysis.warnings();
  if (warning != "") {
    measure->add_child("warning")->add_child_text(warning);
  }

  measure->add_child("mean")
      ->set_attribute("value", ToString(uncert_analysis.mean(), 7));
  measure->add_child("standard-deviation")
      ->set_attribute("value", ToString(uncert_analysis.sigma(), 7));
  xmlpp::Element* confidence = measure->add_child("confidence-range");
  confidence->set_attribute("percentage", "95");
  confidence->set_attribute(
      "lower-bound",
      ToString(uncert_analysis.confidence_interval().first, 7));
  confidence->set_attribute(
      "upper-bound",
      ToString(uncert_analysis.confidence_interval().second, 7));
  xmlpp::Element* error_factor = measure->add_child("error-factor");
  error_factor->set_attribute("percentage", "95");
  error_factor->set_attribute("value",
                              ToString(uncert_analysis.error_factor(), 7));

  xmlpp::Element* quantiles = measure->add_child("quantiles");
  int num_quantiles = uncert_analysis.quantiles().size();
  quantiles->set_attribute("number", ToString(num_quantiles));
  double lower_bound = 0;
  double delta = 1.0 / num_quantiles;
  for (int i = 0; i < num_quantiles; ++i) {
    xmlpp::Element* quant = quantiles->add_child("quantile");
    quant->set_attribute("number", ToString(i + 1));
    double upper = uncert_analysis.quantiles()[i];
    double value = delta * (i + 1);
    quant->set_attribute("value", ToString(value, 7));
    quant->set_attribute("lower-bound", ToString(lower_bound, 7));
    quant->set_attribute("upper-bound", ToString(upper, 7));
    lower_bound = upper;
  }

  xmlpp::Element* hist = measure->add_child("histogram");
  int num_bins = uncert_analysis.distribution().size() - 1;
  hist->set_attribute("number", ToString(num_bins));
  for (int i = 0; i < num_bins; ++i) {
    xmlpp::Element* bin = hist->add_child("bin");
    bin->set_attribute("number", ToString(i + 1));
    double lower = uncert_analysis.distribution()[i].first;
    double upper = uncert_analysis.distribution()[i + 1].first;
    double value = uncert_analysis.distribution()[i].second;
    bin->set_attribute("value", ToString(value, 7));
    bin->set_attribute("lower-bound", ToString(lower, 7));
    bin->set_attribute("upper-bound", ToString(upper, 7));
  }
  xmlpp::NodeSet calc_times =
      root->find("./information/performance/calculation-time");
  assert(!calc_times.empty());
  xmlpp::Element* calc_time = static_cast<xmlpp::Element*>(calc_times.back());
  calc_time->add_child("uncertainty")
      ->add_child_text(ToString(uncert_analysis.analysis_time(), 5));
}

xmlpp::Element* Reporter::ReportBasicEvent(const BasicEventPtr& basic_event,
                                           xmlpp::Element* parent) {
  xmlpp::Element* element;
  std::shared_ptr<const CcfEvent> ccf_event =
      std::dynamic_pointer_cast<const CcfEvent>(basic_event);
  std::string prefix =
        basic_event->is_public() ? "" : basic_event->base_path() + ".";
  if (!ccf_event) {
    std::string name = prefix + basic_event->name();
    element = parent->add_child("basic-event");
    element->set_attribute("name", name);
  } else {
    element = parent->add_child("ccf-event");
    const CcfGroup* ccf_group = ccf_event->ccf_group();
    std::string group_name = prefix + ccf_group->name();
    element->set_attribute("ccf-group", group_name);
    int order = ccf_event->member_names().size();
    element->set_attribute("order", ToString(order));
    int group_size = ccf_group->members().size();
    element->set_attribute("group-size", ToString(group_size));
    for (const std::string& name : ccf_event->member_names()) {
      element->add_child("basic-event")->set_attribute("name", name);
    }
  }
  return element;
}

}  // namespace scram
