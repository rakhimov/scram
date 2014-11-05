/// @file fault_tree_analysis.cc
/// Implementation of fault tree analysis.
#include "fault_tree_analysis.h"

#include <ctime>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>

#include "error.h"
#include "event.h"
#include "fault_tree.h"
#include "indexed_fault_tree.h"

namespace scram {

FaultTreeAnalysis::FaultTreeAnalysis(int limit_order)
  : warnings_(""),
    top_event_index_(-1),
    max_order_(1),
    analysis_time_(0) {
      // Check for right limit order.
  if (limit_order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than one.";
    throw InvalidArgument(msg);
  }
  limit_order_ = limit_order;
}

void FaultTreeAnalysis::Analyze(const FaultTreePtr& fault_tree) {
  // Timing Initialization
  std::clock_t start_time;
  start_time = std::clock();
  // End of Timing Initialization

  // Getting events from the fault tree object.
  top_event_name_ = fault_tree->top_event()->orig_id();
  num_gates_ = fault_tree->inter_events().size() + 1;  // Include top event.
  basic_events_ = fault_tree->basic_events();

  std::set<int> true_house_events;  // Indices of true house events.
  std::set<int> false_house_events;  // Indices of false house events.

  // Assign an index to each primary event, and populate relevant
  // databases.
  int j = 1;
  boost::unordered_map<std::string, PrimaryEventPtr>::const_iterator itp;
  // Dummy primary event at index 0.
  int_to_primary_.push_back(PrimaryEventPtr(new PrimaryEvent("dummy")));
  for (itp = fault_tree->primary_events().begin();
       itp != fault_tree->primary_events().end(); ++itp) {
    if (boost::dynamic_pointer_cast<HouseEvent>(itp->second)) {
      if (itp->second->p() == 0) {
        false_house_events.insert(false_house_events.end(), j);
      } else {
        true_house_events.insert(true_house_events.end(), j);
      }
    }
    int_to_primary_.push_back(itp->second);
    all_to_int_.insert(std::make_pair(itp->first, j));
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

  IndexedFaultTree* indexed_tree =
      new IndexedFaultTree(top_event_index_, limit_order_);
  indexed_tree->InitiateIndexedFaultTree(int_to_inter, all_to_int_);
  indexed_tree->PropagateConstants(true_house_events, false_house_events);
  indexed_tree->ProcessIndexedFaultTree();
  indexed_tree->FindMcs();

  const std::vector< std::set<int> >* imcs = &indexed_tree->GetGeneratedMcs();
  // First, defensive check if cut sets exist for the specified limit order.
  if (imcs->empty()) {
    std::stringstream msg;
    msg << "No cut sets for the limit order " <<  limit_order_;
    warnings_ += msg.str();
    return;
  }

  // Duration of MCS generation.
  analysis_time_ = (std::clock() - start_time) /
                   static_cast<double>(CLOCKS_PER_SEC);
  FaultTreeAnalysis::SetsToString(*imcs);  // MCS with event ids.
  delete indexed_tree;  // No exeptions are expected.
}

void FaultTreeAnalysis::SetsToString(const std::vector< std::set<int> >& imcs) {
  std::vector< std::set<int> >::const_iterator it_min;
  for (it_min = imcs.begin(); it_min != imcs.end(); ++it_min) {
    if (it_min->size() > max_order_) max_order_ = it_min->size();
    std::set<std::string> pr_set;
    std::set<int>::iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      if (*it_set < 0) {  // NOT logic.
        pr_set.insert("not " + int_to_primary_[std::abs(*it_set)]->id());
      } else {
        pr_set.insert(int_to_primary_[*it_set]->id());
      }
    }
    min_cut_sets_.insert(pr_set);
  }
}

}  // namespace scram
