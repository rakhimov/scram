/// @file reporter.cc
/// Implements Reporter class.
#include "reporter.h"

#include <map>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <boost/lexical_cast.hpp>

#include "event.h"
#include "error.h"
#include "expression.h"
#include "fault_tree_analysis.h"
#include "probability_analysis.h"
#include "risk_analysis.h"
#include "settings.h"
#include "uncertainty_analysis.h"
#include "version.h"

namespace scram {

void Reporter::SetupReport(const RiskAnalysis* risk_an,
                           const Settings& settings, xmlpp::Document* doc) {
  if (doc->get_root_node() != 0) {
    throw LogicError("The passed document is not empty for reporting");
  }
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

  // Report the setup for main minimal cut set analysis.
  xmlpp::Element* quant = information->add_child("calculated-quantity");
  quant->set_attribute("name", "Minimal Cut Set Analysis");
  quant->set_attribute(
      "definition",
      "Groups of events sufficient for a top event failure");

  xmlpp::Element* methods = information->add_child("calculation-method");
  methods->set_attribute("name", "MOCUS");
  methods->add_child("limits")->add_child("number-of-basic-events")
      ->add_child_text(boost::lexical_cast<std::string>(settings.limit_order_));

  // Report the setup for CCF analysis.
  if (settings.ccf_analysis_) {
    xmlpp::Element* ccf_an = information->add_child("calculated-quantity");
    ccf_an->set_attribute("name", "CCF Analysis");
    ccf_an->set_attribute("definition",
                          "Failure of multiple elements due to a common cause");
  }

  // Report the setup for probability analysis.
  if (settings.probability_analysis_) {
    quant = information->add_child("calculated-quantity");
    quant->set_attribute("name", "Probability Analysis");
    quant->set_attribute("definition",
                         "Quantitative analysis of failure probability");
    quant->set_attribute("approximation", settings.approx_);

    methods = information->add_child("calculation-method");
    methods->set_attribute("name", "Numerical Probability");
    xmlpp::Element* limits = methods->add_child("limits");
    limits->add_child("mission-time")
        ->add_child_text(
            boost::lexical_cast<std::string>(settings.mission_time_));
    limits->add_child("cut-off")
        ->add_child_text(boost::lexical_cast<std::string>(settings.cut_off_));
    limits->add_child("number-of-sums")
        ->add_child_text(boost::lexical_cast<std::string>(settings.num_sums_));
  }

  // Report the setup for optional importance analysis.
  if (settings.importance_analysis_) {
    quant = information->add_child("calculated-quantity");
    quant->set_attribute("name", "Importance Analysis");
    quant->set_attribute("definition",
                         "Quantitative analysis of contributions and "\
                         "importance of events.");
  }

  // Report the setup for optional uncertainty analysis.
  if (settings.uncertainty_analysis_) {
    xmlpp::Element* quant = information->add_child("calculated-quantity");
    quant->set_attribute("name", "Uncertainty Analysis");
    quant->set_attribute(
        "definition",
        "Calculation of uncertainties with the Monte Carlo method");

    xmlpp::Element* methods = information->add_child("calculation-method");
    methods->set_attribute("name", "Monte Carlo");
    xmlpp::Element* limits = methods->add_child("limits");
    limits->add_child("number-of-trials")->add_child_text(
        boost::lexical_cast<std::string>(settings.num_trials_));
    if (settings.seed_ >= 0) {
      limits->add_child("seed")->add_child_text(
          boost::lexical_cast<std::string>(settings.seed_));
    }
  }

  xmlpp::Element* model = information->add_child("model-features");
  model->add_child("gates")->add_child_text(
      boost::lexical_cast<std::string>(risk_an->gates_.size()));
  model->add_child("basic-events")->add_child_text(
      boost::lexical_cast<std::string>(risk_an->basic_events_.size()));
  model->add_child("house-events")
      ->add_child_text(boost::lexical_cast<std::string>(
              risk_an->primary_events_.size() - risk_an->basic_events_.size()));
  model->add_child("ccf-groups")->add_child_text(
      boost::lexical_cast<std::string>(risk_an->ccf_groups_.size()));
  model->add_child("fault-trees")->add_child_text(
      boost::lexical_cast<std::string>(risk_an->fault_trees_.size()));

  // Setup for results.
  root->add_child("results");
}

void Reporter::ReportOrphanPrimaryEvents(
    const std::set<boost::shared_ptr<PrimaryEvent> >& orphan_primary_events,
    xmlpp::Document* doc) {
  assert(!orphan_primary_events.empty());
  std::string out = "";
  out += "WARNING! Orphan Primary Events: ";
  std::set<boost::shared_ptr<PrimaryEvent> >::const_iterator it;
  for (it = orphan_primary_events.begin(); it != orphan_primary_events.end();
       ++it) {
    out += (*it)->name() + " ";
  }
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet inf = root->find("./information");
  assert(inf.size() == 1);
  xmlpp::Element* information = dynamic_cast<xmlpp::Element*>(inf[0]);
  information->add_child("warning")->add_child_text(out);
}

void Reporter::ReportUnusedParameters(
    const std::set<boost::shared_ptr<Parameter> >& unused_parameters,
    xmlpp::Document* doc) {
  assert(!unused_parameters.empty());
  std::string out = "";
  out += "WARNING! Unused Parameters: ";
  std::set<boost::shared_ptr<Parameter> >::const_iterator it;
  for (it = unused_parameters.begin(); it != unused_parameters.end();
       ++it) {
    out += (*it)->name() + " ";
  }
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet inf = root->find("./information");
  assert(inf.size() == 1);
  xmlpp::Element* information = dynamic_cast<xmlpp::Element*>(inf[0]);
  information->add_child("warning")->add_child_text(out);
}

void Reporter::ReportFta(
    std::string ft_name,
    const boost::shared_ptr<const FaultTreeAnalysis>& fta,
    const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
    xmlpp::Document* doc) {
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet res = root->find("./results");
  assert(res.size() == 1);
  xmlpp::Element* results = dynamic_cast<xmlpp::Element*>(res[0]);
  xmlpp::Element* sum_of_products = results->add_child("sum-of-products");
  sum_of_products->set_attribute("name", ft_name);
  sum_of_products->set_attribute(
      "basic-events",
      boost::lexical_cast<std::string>(fta->num_mcs_events_));
  sum_of_products->set_attribute(
      "products",
      boost::lexical_cast<std::string>(fta->min_cut_sets_.size()));

  if (prob_analysis) {
    sum_of_products->set_attribute(
        "probability",
        Reporter::ToString(prob_analysis->p_total(), 7));
  }

  std::string warning = fta->warnings();
  if (prob_analysis) warning += prob_analysis->warnings();
  if (warning != "") {
    sum_of_products->add_child("warning")->add_child_text(warning);
  }

  std::set< std::set<std::string> >::const_iterator it_min;
  for (it_min = fta->min_cut_sets_.begin(); it_min != fta->min_cut_sets_.end();
       ++it_min) {
    xmlpp::Element* product = sum_of_products->add_child("product");
    product->set_attribute("order",
                           boost::lexical_cast<std::string>(it_min->size()));

    if (prob_analysis) {
      product->set_attribute(
          "probability",
          Reporter::ToString(
              prob_analysis->prob_of_min_sets().find(*it_min)->second,
              7));
    }

    // List elements of minimal cut sets.
    std::set<std::string>::const_iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      std::vector<std::string> names;
      std::string full_name = *it_set;
      boost::split(names, full_name, boost::is_any_of(" "),
                   boost::token_compress_on);
      assert(names.size() >= 1);
      xmlpp::Element* parent = product;
      std::string name = full_name;  // Id of a basic event.
      if (names[0] == "not") {  // Detect negation.
        std::string comp_name = full_name;
        boost::replace_first(comp_name, "not ", "");
        name = comp_name;
        parent = product->add_child("not");
      }
      Reporter::ReportBasicEvent(fta->basic_events_.find(name)->second,
                                 parent);
    }
  }

  // Report calculation time in the information section.
  // It is assumed that MCS reporting is the default and the first thing
  // to be reported.
  xmlpp::NodeSet perf = root->find("./information/performance");
  assert(perf.size() == 1);
  xmlpp::Element* performance = dynamic_cast<xmlpp::Element*>(perf[0]);
  xmlpp::Element* calc_time = performance->add_child("calculation-time");
  calc_time->set_attribute("name", ft_name);
  calc_time->add_child("minimal-cut-set")->add_child_text(
      Reporter::ToString(fta->analysis_time_, 5));
  if (prob_analysis) {
    calc_time->add_child("probability")->add_child_text(
      Reporter::ToString(prob_analysis->p_time_, 5));
  }
}

void Reporter::ReportImportance(
    std::string ft_name,
    const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
    xmlpp::Document* doc) {
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet res = root->find("./results");
  assert(res.size() == 1);
  xmlpp::Element* results = dynamic_cast<xmlpp::Element*>(res[0]);
  xmlpp::Element* importance = results->add_child("importance");
  importance->set_attribute("name", ft_name);
  importance->set_attribute(
      "basic-events",
      boost::lexical_cast<std::string>(prob_analysis->importance().size()));

  std::string warning = prob_analysis->warnings();
  if (warning != "") {
    importance->add_child("warning")->add_child_text(warning);
  }

  std::map< std::string, std::vector<double> >::const_iterator it;
  for (it = prob_analysis->importance().begin();
       it != prob_analysis->importance().end(); ++it) {
    typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
    typedef boost::shared_ptr<CcfEvent> CcfEventPtr;
    xmlpp::Element* element = Reporter::ReportBasicEvent(
        prob_analysis->basic_events_.find(it->first)->second,
        importance);
    element->set_attribute("DIF", Reporter::ToString(it->second[0], 4));
    element->set_attribute("MIF", Reporter::ToString(it->second[1], 4));
    element->set_attribute("CIF", Reporter::ToString(it->second[2], 4));
    element->set_attribute("RRW", Reporter::ToString(it->second[3], 4));
    element->set_attribute("RAW", Reporter::ToString(it->second[4], 4));
  }
  xmlpp::NodeSet calc_times =
      root->find("./information/performance/calculation-time");
  assert(!calc_times.empty());
  xmlpp::Element* calc_time = dynamic_cast<xmlpp::Element*>(calc_times.back());
  calc_time->add_child("importance")->add_child_text(
      Reporter::ToString(prob_analysis->imp_time_, 5));
}

void Reporter::ReportUncertainty(
    std::string ft_name,
    const boost::shared_ptr<const UncertaintyAnalysis>& uncert_analysis,
    xmlpp::Document* doc) {
  xmlpp::Node* root = doc->get_root_node();
  xmlpp::NodeSet res = root->find("./results");
  assert(res.size() == 1);
  xmlpp::Element* results = dynamic_cast<xmlpp::Element*>(res[0]);
  xmlpp::Element* measure = results->add_child("measure");
  measure->set_attribute("name", ft_name);

  std::string warning = uncert_analysis->warnings();
  if (warning != "") {
    measure->add_child("warning")->add_child_text(warning);
  }

  measure->add_child("mean")
      ->set_attribute("value", Reporter::ToString(uncert_analysis->mean(), 7));
  measure->add_child("standard-deviation")
      ->set_attribute("value", Reporter::ToString(uncert_analysis->sigma(), 7));
  xmlpp::Element* confidence = measure->add_child("confidence-range");
  confidence->set_attribute("percentage", "95");
  confidence->set_attribute(
      "lower-bound",
      Reporter::ToString(uncert_analysis->confidence_interval().first, 7));
  confidence->set_attribute(
      "upper-bound",
      Reporter::ToString(uncert_analysis->confidence_interval().second, 7));
  /// @todo Error factor reporting.
  xmlpp::Element* quantiles = measure->add_child("quantiles");
  int num_bins = uncert_analysis->distribution().size() - 1;
  quantiles->set_attribute("number",
                           boost::lexical_cast<std::string>(num_bins));
  for (int i = 0; i < num_bins; ++i) {
    xmlpp::Element* quant = quantiles->add_child("quantile");
    quant->set_attribute("number", boost::lexical_cast<std::string>(i + 1));
    double lower = uncert_analysis->distribution()[i].first;
    double upper = uncert_analysis->distribution()[i + 1].first;
    double value = uncert_analysis->distribution()[i + 1].second;
    quant->set_attribute("mean", Reporter::ToString(value, 7));
    quant->set_attribute("lower-bound", Reporter::ToString(lower, 7));
    quant->set_attribute("upper-bound", Reporter::ToString(upper, 7));
  }
  xmlpp::NodeSet calc_times =
      root->find("./information/performance/calculation-time");
  assert(!calc_times.empty());
  xmlpp::Element* calc_time = dynamic_cast<xmlpp::Element*>(calc_times.back());
  calc_time->add_child("uncertainty")->add_child_text(
      Reporter::ToString(uncert_analysis->p_time_, 5));
}

xmlpp::Element* Reporter::ReportBasicEvent(
    const boost::shared_ptr<BasicEvent>& basic_event,
    xmlpp::Element* parent) {
  boost::shared_ptr<CcfEvent> ccf_event =
      boost::dynamic_pointer_cast<CcfEvent>(basic_event);
  xmlpp::Element* element;
  if (!ccf_event) {
    element = parent->add_child("basic-event");
    element->set_attribute("name", basic_event->name());
  } else {
    element = parent->add_child("ccf-event");
    element->set_attribute("ccf-group", ccf_event->ccf_group_name());
    element->set_attribute(
        "order",
        boost::lexical_cast<std::string>(ccf_event->member_names().size()));
    element->set_attribute(
        "group-size",
        boost::lexical_cast<std::string>(ccf_event->ccf_group_size()));
    std::vector<std::string>::const_iterator it;
    for (it = ccf_event->member_names().begin();
         it != ccf_event->member_names().end(); ++it) {
      element->add_child("basic-event")->set_attribute("name", *it);
    }
  }
  return element;
}

}  // namespace scram
