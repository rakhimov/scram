/// @file fault_tree_analysis.cc
/// Implementation of fault tree analysis.
#include "fault_tree_analysis.h"

#include <sstream>

#include <boost/algorithm/string.hpp>

#include "error.h"
#include "indexed_fault_tree.h"
#include "logger.h"
#include "mocus.h"
#include "preprocessor.h"

namespace scram {

FaultTreeAnalysis::FaultTreeAnalysis(const GatePtr& root, int limit_order,
                                     bool ccf_analysis)
    : ccf_analysis_(ccf_analysis),
      warnings_(""),
      max_order_(0),
      analysis_time_(0) {
  // Check for the right limit order.
  if (limit_order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than one.";
    throw InvalidArgument(msg);
  }
  limit_order_ = limit_order;
  top_event_ = root;
  FaultTreeAnalysis::GatherEvents(top_event_);
  FaultTreeAnalysis::CleanMarks();
}

void FaultTreeAnalysis::Analyze() {
  CLOCK(analysis_time);

  IndexedFaultTree* indexed_tree = new IndexedFaultTree(top_event_,
                                                        ccf_analysis_);
  Preprocessor* preprocessor = new Preprocessor(indexed_tree);
  preprocessor->ProcessIndexedFaultTree();
  Mocus* mocus = new Mocus(indexed_tree, limit_order_);
  mocus->FindMcs();

  const std::vector< std::set<int> >& imcs = mocus->GetGeneratedMcs();
  // First, defensive check if cut sets exist for the specified limit order.
  if (imcs.empty()) {
    std::stringstream msg;
    msg << " No cut sets for the limit order " <<  limit_order_;
    warnings_ += msg.str();
    return;
  } else if (imcs.size() == 1 && imcs.back().empty()) {
    // Special case of unity of a top event.
    std::stringstream msg;
    msg << " The top event is UNITY. Failure is guaranteed.";
    warnings_ += msg.str();
  }

  analysis_time_ = DUR(analysis_time);  // Duration of MCS generation.
  FaultTreeAnalysis::SetsToString(imcs, indexed_tree);  // MCS with event ids.
  delete indexed_tree;  // No exceptions are expected.
  delete preprocessor;  // No exceptions are expected.
  delete mocus;  // No exceptions are expected.
}

void FaultTreeAnalysis::GatherEvents(const GatePtr& gate) {
  if (gate->mark() == "visited") return;
  gate->mark("visited");
  FaultTreeAnalysis::GatherEvents(gate->formula());
}

void FaultTreeAnalysis::GatherEvents(const FormulaPtr& formula) {
  const std::map<std::string, EventPtr>* children = &formula->event_args();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    if (GatePtr child_gate = boost::dynamic_pointer_cast<Gate>(it->second)) {
      inter_events_.insert(std::make_pair(child_gate->id(), child_gate));
      FaultTreeAnalysis::GatherEvents(child_gate);

    } else if (BasicEventPtr basic_event =
               boost::dynamic_pointer_cast<BasicEvent>(it->second)) {
      basic_events_.insert(std::make_pair(it->first, basic_event));
      if (basic_event->HasCcf())
        ccf_events_.insert(std::make_pair(it->first, basic_event));
    } else {
      HouseEventPtr house_event =
          boost::dynamic_pointer_cast<HouseEvent>(it->second);
      assert(house_event);
      house_events_.insert(std::make_pair(it->first, house_event));
    }
  }
  const std::set<FormulaPtr>* formulas = &formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas->begin(); it_f != formulas->end(); ++it_f) {
    FaultTreeAnalysis::GatherEvents(*it_f);
  }
}

void FaultTreeAnalysis::CleanMarks() {
  top_event_->mark("");
  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    it->second->mark("");
  }
}

void FaultTreeAnalysis::SetsToString(const std::vector< std::set<int> >& imcs,
                                     const IndexedFaultTree* ft) {
  std::vector< std::set<int> >::const_iterator it_min;
  for (it_min = imcs.begin(); it_min != imcs.end(); ++it_min) {
    if (it_min->size() > max_order_) max_order_ = it_min->size();
    std::set<std::string> pr_set;
    std::set<int>::iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      BasicEventPtr basic_event = ft->GetBasicEvent(std::abs(*it_set));
      if (*it_set < 0) {  // NOT logic.
        pr_set.insert("not " + basic_event->id());
      } else {
        pr_set.insert(basic_event->id());
      }
      mcs_basic_events_.insert(std::make_pair(basic_event->id(), basic_event));
    }
    min_cut_sets_.insert(pr_set);
  }
}

}  // namespace scram
