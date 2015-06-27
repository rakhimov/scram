/// @file fault_tree_analysis.cc
/// Implementation of fault tree analysis.
#include "fault_tree_analysis.h"

#include <sstream>

#include <boost/algorithm/string.hpp>

#include "error.h"
#include "indexed_fault_tree.h"
#include "logger.h"

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
  // Assign an index to each basic event, and populate relevant
  // databases.
  int j = 1;  // Indices must be able to be negated, so 0 is excluded.
  boost::unordered_map<std::string, BasicEventPtr>::const_iterator itp;
  // Dummy basic event at index 0.
  int_to_basic_.push_back(BasicEventPtr(new BasicEvent("dummy")));
  for (itp = basic_events_.begin(); itp != basic_events_.end(); ++itp) {
    int_to_basic_.push_back(itp->second);
    all_to_int_.insert(std::make_pair(itp->first, j));
    ++j;
  }

  // Gather CCF generated basic events.
  boost::unordered_map<std::string, BasicEventPtr> ccf_basic_events;
  if (ccf_analysis_) FaultTreeAnalysis::GatherCcfBasicEvents(&ccf_basic_events);
  for (itp = ccf_basic_events.begin(); itp != ccf_basic_events.end(); ++itp) {
    int_to_basic_.push_back(itp->second);
    all_to_int_.insert(std::make_pair(itp->first, j));
    ++j;
  }

  // Detect true and false house events for constant propagation.
  std::set<int> true_house_events;  // Indices of true house events.
  std::set<int> false_house_events;  // Indices of false house events.

  boost::unordered_map<std::string, HouseEventPtr>::const_iterator ith;
  for (ith = house_events_.begin(); ith != house_events_.end(); ++ith) {
    if (ith->second->state()) {
      true_house_events.insert(true_house_events.end(), j);
    } else {
      false_house_events.insert(false_house_events.end(), j);
    }
    all_to_int_.insert(std::make_pair(ith->first, j));
    ++j;
  }

  // Intermediate events from indices.
  boost::unordered_map<int, GatePtr> int_to_inter;
  // Assign an index to each top and intermediate event and populate
  // relevant databases.
  int top_event_index = j;
  int_to_inter.insert(std::make_pair(j, top_event_));
  all_to_int_.insert(std::make_pair(top_event_->id(), j));
  ++j;
  boost::unordered_map<std::string, GatePtr>::const_iterator iti;
  for (iti = inter_events_.begin(); iti != inter_events_.end(); ++iti) {
    int_to_inter.insert(std::make_pair(j, iti->second));
    all_to_int_.insert(std::make_pair(iti->first, j));
    ++j;
  }

  std::map<std::string, int> ccf_basic_to_gates;
  if (ccf_analysis_) {
    // Include CCF gates instead of basic events.
    boost::unordered_map<std::string, BasicEventPtr>::const_iterator itc;
    for (itc = ccf_events_.begin(); itc != ccf_events_.end(); ++itc) {
      // Does not add ccf gates into all_to_int container because the same
      // ids are used for basic events representing the members of CCF groups.
      assert(itc->second->HasCcf());
      int_to_inter.insert(std::make_pair(j, itc->second->ccf_gate()));
      ccf_basic_to_gates.insert(std::make_pair(itc->first, j));
      ++j;
    }
  }

  IndexedFaultTree* indexed_tree =
      new IndexedFaultTree(top_event_index, limit_order_);
  indexed_tree->InitiateIndexedFaultTree(int_to_inter, ccf_basic_to_gates,
                                         all_to_int_);
  indexed_tree->PropagateConstants(true_house_events, false_house_events);
  indexed_tree->ProcessIndexedFaultTree(int_to_basic_.size());
  indexed_tree->FindMcs();

  const std::vector< std::set<int> >* imcs = &indexed_tree->GetGeneratedMcs();
  // First, defensive check if cut sets exist for the specified limit order.
  if (imcs->empty()) {
    std::stringstream msg;
    msg << " No cut sets for the limit order " <<  limit_order_;
    warnings_ += msg.str();
    return;
  } else if (imcs->size() == 1 && imcs->back().empty()) {
    // Special case of unity of a top event.
    std::stringstream msg;
    msg << " The top event is UNITY. Failure is guaranteed.";
    warnings_ += msg.str();
  }

  analysis_time_ = DUR(analysis_time);  // Duration of MCS generation.
  FaultTreeAnalysis::SetsToString(*imcs);  // MCS with event ids.
  delete indexed_tree;  // No exceptions are expected.
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
    GatePtr child_gate = boost::dynamic_pointer_cast<Gate>(it->second);
    BasicEventPtr basic_event =
        boost::dynamic_pointer_cast<BasicEvent>(it->second);
    HouseEventPtr house_event =
        boost::dynamic_pointer_cast<HouseEvent>(it->second);
    if (child_gate) {
      inter_events_.insert(std::make_pair(child_gate->id(), child_gate));
      FaultTreeAnalysis::GatherEvents(child_gate);

    } else if (basic_event) {
      assert(!house_event);
      basic_events_.insert(std::make_pair(it->first, basic_event));
      if (basic_event->HasCcf())
        ccf_events_.insert(std::make_pair(it->first, basic_event));
    } else {
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

void FaultTreeAnalysis::GatherCcfBasicEvents(
    boost::unordered_map<std::string, BasicEventPtr>* basic_events) {
  boost::unordered_map<std::string, BasicEventPtr>::iterator it_b;
  for (it_b = ccf_events_.begin(); it_b != ccf_events_.end(); ++it_b) {
    assert(it_b->second->HasCcf());
    const std::map<std::string, EventPtr>* children =
        &it_b->second->ccf_gate()->formula()->event_args();
    std::map<std::string, EventPtr>::const_iterator it;
    for (it = children->begin(); it != children->end(); ++it) {
      BasicEventPtr basic_event =
          boost::dynamic_pointer_cast<BasicEvent>(it->second);
      assert(basic_event);
      basic_events->insert(std::make_pair(basic_event->id(), basic_event));
    }
  }
}

void FaultTreeAnalysis::SetsToString(const std::vector< std::set<int> >& imcs) {
  std::vector< std::set<int> >::const_iterator it_min;
  for (it_min = imcs.begin(); it_min != imcs.end(); ++it_min) {
    bool unique = false;
    if (it_min->size() > max_order_) max_order_ = it_min->size();
    std::set<std::string> pr_set;
    std::set<int>::iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      BasicEventPtr basic_event = int_to_basic_[std::abs(*it_set)];
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
