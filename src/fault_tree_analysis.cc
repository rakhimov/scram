/// @file fault_tree_analysis.cc
/// Implementation of fault tree analysis.
#include "fault_tree_analysis.h"

#include <sstream>

#include <boost/algorithm/string.hpp>

#include "error.h"
#include "fault_tree.h"
#include "indexed_fault_tree.h"
#include "logger.h"

namespace scram {

FaultTreeAnalysis::FaultTreeAnalysis(int limit_order, bool ccf_analysis)
    : ccf_analysis_(ccf_analysis),
      warnings_(""),
      top_event_index_(-1),
      max_order_(0),
      num_gates_(0),
      num_basic_events_(0),
      num_mcs_events_(0),
      analysis_time_(0) {
  // Check for the right limit order.
  if (limit_order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than one.";
    throw InvalidArgument(msg);
  }
  limit_order_ = limit_order;
}

FaultTreeAnalysis::FaultTreeAnalysis(const GatePtr& root, int limit_order,
                                     bool ccf_analysis)
    : ccf_analysis_(ccf_analysis),
      warnings_(""),
      top_event_index_(-1),
      max_order_(0),
      num_gates_(0),
      num_basic_events_(0),
      num_mcs_events_(0),
      analysis_time_(0) {
  // Check for the right limit order.
  if (limit_order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than one.";
    throw InvalidArgument(msg);
  }
  limit_order_ = limit_order;
  top_event_ = root;
  FaultTreeAnalysis::SetupForAnalysis();
}

void FaultTreeAnalysis::Analyze(const FaultTreePtr& fault_tree) {
  CLOCK(analysis_time);
  // Getting events from the fault tree object.
  top_event_name_ = fault_tree->top_event()->name();
  num_gates_ = fault_tree->inter_events().size() + 1;  // Include top event.
  basic_events_ = fault_tree->basic_events();
  num_basic_events_ = fault_tree->num_basic_events();

  // Assign an index to each basic event, and populate relevant
  // databases.
  int j = 1;  // Indices must be able to be negated, so 0 is excluded.
  boost::unordered_map<std::string, BasicEventPtr>::const_iterator itp;
  // Dummy basic event at index 0.
  int_to_basic_.push_back(BasicEventPtr(new BasicEvent("dummy")));
  for (itp = fault_tree->basic_events().begin();
       itp != fault_tree->basic_events().end(); ++itp) {
    int_to_basic_.push_back(itp->second);
    all_to_int_.insert(std::make_pair(itp->first, j));
    ++j;
  }

  // Detect true and false house events for constant propagation.
  std::set<int> true_house_events;  // Indices of true house events.
  std::set<int> false_house_events;  // Indices of false house events.

  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  boost::unordered_map<std::string, HouseEventPtr>::const_iterator ith;
  for (ith = fault_tree->house_events().begin();
       ith != fault_tree->house_events().end(); ++ith) {
    if (ith->second->state()) {
      true_house_events.insert(true_house_events.end(), j);
    } else {
      false_house_events.insert(false_house_events.end(), j);
    }
    all_to_int_.insert(std::make_pair(ith->first, j));
    ++j;
  }

  typedef boost::shared_ptr<Gate> GatePtr;
  // Intermediate events from indices.
  boost::unordered_map<int, GatePtr> int_to_inter;
  // Assign an index to each top and intermediate event and populate
  // relevant databases.
  top_event_index_ = j;
  int_to_inter.insert(std::make_pair(j, fault_tree->top_event()));
  all_to_int_.insert(std::make_pair(fault_tree->top_event()->id(), j));
  ++j;
  boost::unordered_map<std::string, GatePtr>::const_iterator iti;
  for (iti = fault_tree->inter_events().begin();
       iti != fault_tree->inter_events().end(); ++iti) {
    int_to_inter.insert(std::make_pair(j, iti->second));
    all_to_int_.insert(std::make_pair(iti->first, j));
    ++j;
  }

  std::map<std::string, int> ccf_basic_to_gates;
  if (ccf_analysis_) {
    // Include CCF gates instead of basic events.
    boost::unordered_map<std::string, BasicEventPtr>::const_iterator itc;
    for (itc = fault_tree->ccf_events().begin();
         itc != fault_tree->ccf_events().end(); ++itc) {
      assert(itc->second->HasCcf());
      int_to_inter.insert(std::make_pair(j, itc->second->ccf_gate()));
      ccf_basic_to_gates.insert(std::make_pair(itc->first, j));
      ++j;
    }
  }

  IndexedFaultTree* indexed_tree =
      new IndexedFaultTree(top_event_index_, limit_order_);
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

void FaultTreeAnalysis::SetupForAnalysis() {
  FaultTreeAnalysis::GatherInterEvents(top_event_);
  FaultTreeAnalysis::GatherPrimaryEvents();
  // Recording number of original basic events before putting new CCF events.
  num_basic_events_ = basic_events_.size();
  // Gather CCF generated basic events.
  FaultTreeAnalysis::GatherCcfBasicEvents();
}

void FaultTreeAnalysis::GatherInterEvents(const GatePtr& gate) {
  if (gate->mark() == "visited") return;
  gate->mark("visited");
  const std::map<std::string, EventPtr>* children =
      &gate->formula()->event_args();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    GatePtr child_gate = boost::dynamic_pointer_cast<Gate>(it->second);
    if (child_gate) {
      inter_events_.insert(std::make_pair(child_gate->id(), child_gate));
      FaultTreeAnalysis::GatherInterEvents(child_gate);
    }
  }
}

void FaultTreeAnalysis::GatherPrimaryEvents() {
  FaultTreeAnalysis::GetPrimaryEvents(top_event_);

  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = inter_events_.begin(); it != inter_events_.end(); ++it) {
    it->second->mark("");
    FaultTreeAnalysis::GetPrimaryEvents(it->second);
  }
}

void FaultTreeAnalysis::GetPrimaryEvents(const GatePtr& gate) {
  const std::map<std::string, EventPtr>* children =
      &gate->formula()->event_args();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = children->begin(); it != children->end(); ++it) {
    if (!inter_events_.count(it->first)) {
      PrimaryEventPtr primary_event =
          boost::dynamic_pointer_cast<PrimaryEvent>(it->second);

      if (primary_event == 0) {  // The tree must be fully defined.
        throw LogicError("Node " + it->second->name() +
                         " is not fully defined.");
      }

      primary_events_.insert(std::make_pair(it->first, primary_event));
      BasicEventPtr basic_event =
          boost::dynamic_pointer_cast<BasicEvent>(primary_event);
      if (basic_event) {
        basic_events_.insert(std::make_pair(it->first, basic_event));
        if (basic_event->HasCcf())
          ccf_events_.insert(std::make_pair(it->first, basic_event));
      } else {
        HouseEventPtr house_event =
            boost::dynamic_pointer_cast<HouseEvent>(primary_event);
        assert(house_event);
        house_events_.insert(std::make_pair(it->first, house_event));
      }
    }
  }
}

void FaultTreeAnalysis::GatherCcfBasicEvents() {
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
      basic_events_.insert(std::make_pair(basic_event->id(), basic_event));
      primary_events_.insert(std::make_pair(basic_event->id(), basic_event));
    }
  }
}


void FaultTreeAnalysis::SetsToString(const std::vector< std::set<int> >& imcs) {
  std::set<int> unique_events;
  std::vector< std::set<int> >::const_iterator it_min;
  for (it_min = imcs.begin(); it_min != imcs.end(); ++it_min) {
    if (it_min->size() > max_order_) max_order_ = it_min->size();
    std::set<std::string> pr_set;
    std::set<int>::iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      if (*it_set < 0) {  // NOT logic.
        pr_set.insert("not " + int_to_basic_[std::abs(*it_set)]->id());
        unique_events.insert(-*it_set);
      } else {
        pr_set.insert(int_to_basic_[*it_set]->id());
        unique_events.insert(*it_set);
      }
    }
    min_cut_sets_.insert(pr_set);
  }
  num_mcs_events_ = unique_events.size();
}

}  // namespace scram
