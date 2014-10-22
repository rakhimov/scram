/// @file fault_tree_analysis.cc
/// Implementation of fault tree analysis.
#include "fault_tree_analysis.h"

#include <ctime>
#include <iterator>
#include <functional>
#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>

#include "error.h"

namespace scram {

FaultTreeAnalysis::FaultTreeAnalysis(int limit_order)
  : warnings_(""),
    top_event_index_(-1),
    max_order_(1),
    exp_time_(0),
    mcs_time_(0) {
      // Check for right limit order.
  if (limit_order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than one.";
    throw InvalidArgument(msg);
  }
  limit_order_ = limit_order;

  // Pointer to the top event.
  GatePtr top_event_;

  // Initialize a fault tree with a default name.
  FaultTreePtr fault_tree_;
}

/// @class SetPtrComp
/// Functor for set pointer comparison efficiency.
struct SetPtrComp
    : public std::binary_function<const std::set<int>*,
                                  const std::set<int>*, bool> {
  /// Operator overload.
  /// Compares sets for sorting.
  bool operator()(const std::set<int>* lhs, const std::set<int>* rhs) const {
    return *lhs < *rhs;
  }
};

void FaultTreeAnalysis::Analyze(const FaultTreePtr& fault_tree) {
  // Timing Initialization
  std::clock_t start_time;
  start_time = std::clock();
  // End of Timing Initialization

  // Container for cut sets with primary events only.
  std::vector<SupersetPtr> cut_sets;

  FaultTreeAnalysis::AssignIndices(fault_tree);

  SupersetPtr top_set(new Superset());
  top_set->InsertGate(top_event_index_ * indexed_tree_->top_event_sign());

  ExpandTree(top_set, &cut_sets);  // Cut set generation.

  // Duration of the expansion.
  exp_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);

  // At this point cut sets are generated.
  // Now we need to reduce them to minimal cut sets.

  // First, defensive check if cut sets exist for the specified limit order.
  if (cut_sets.empty()) {
    std::stringstream msg;
    msg << "No cut sets for the limit order " <<  limit_order_;
    warnings_ += msg.str();
    return;
  }

  // An iterator for a vector with sets of ids of primary events.
  std::vector< SupersetPtr >::iterator it_vec;

  // Choose to convert vector to a set to get rid of any duplications.
  SetPtrComp comp;
  std::set< const std::set<int>*, SetPtrComp > unique_cut_sets(comp);
  for (it_vec = cut_sets.begin(); it_vec != cut_sets.end(); ++it_vec) {
    unique_cut_sets.insert(&(*it_vec)->p_events());
  }

  std::vector< std::set<int> > imcs;  // Min cut sets with indexed events.
  imcs.reserve(unique_cut_sets.size());
  std::vector<const std::set<int>* > sets_unique;
  std::set< const std::set<int>*, SetPtrComp >::iterator it_un;
  for (it_un = unique_cut_sets.begin(); it_un != unique_cut_sets.end();
       ++it_un) {
    if ((*it_un)->size() == 1) {
      // Minimal cut set is detected.
      imcs.push_back(**it_un);
      continue;
    }
    sets_unique.push_back(*it_un);
  }

  FaultTreeAnalysis::FindMcs(sets_unique, imcs, 2, &imcs);
  // Duration of MCS generation.
  mcs_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);
  FaultTreeAnalysis::SetsToString(imcs);  // MCS with event ids.
}

void FaultTreeAnalysis::ExpandTree(const SupersetPtr& set_with_gates,
                                   std::vector<SupersetPtr>* cut_sets) {
  // To hold sets of children.
  std::vector<SupersetPtr> children_sets;

  FaultTreeAnalysis::ExpandSets(set_with_gates->PopGate(), &children_sets);

  // An iterator for a vector with Supersets.
  std::vector<SupersetPtr>::iterator it_sup;

  // Attach the original set into this event's sets of children.
  for (it_sup = children_sets.begin(); it_sup != children_sets.end();
       ++it_sup) {
    // Add this set to the original inter_sets.
    if ((*it_sup)->InsertSet(set_with_gates)) {
      // Discard this tmp set if it is larger than the limit.
      if ((*it_sup)->NumOfPrimaryEvents() > limit_order_) continue;

      if ((*it_sup)->gates().empty()) {
        // This is a set with primary events only.
        cut_sets->push_back(*it_sup);
        continue;
      }
      ExpandTree(*it_sup, cut_sets);
    }
  }
}

void FaultTreeAnalysis::ExpandSets(int inter_index,
                                   std::vector< SupersetPtr >* sets) {
  assert(sets->empty());
  assert(inter_index != 0);

  if (indexed_tree_->GateState(std::abs(inter_index)) == "null") return;

  if (FaultTreeAnalysis::GetExpandedSets(inter_index, sets)) return;

  // Populate intermediate and primary events of the top.
  const std::set<int>* events_children = &indexed_tree_
      ->GateChildren(std::abs(inter_index));
  if (inter_index > 0) {
    FaultTreeAnalysis::ExpandPositiveGate(inter_index, *events_children, sets);
  } else {
    FaultTreeAnalysis::ExpandNegativeGate(-inter_index, *events_children, sets);
  }

  SaveExpandedSets(inter_index, *sets);
}

void FaultTreeAnalysis::ExpandPositiveGate(
    int inter_index,
    const std::set<int>& events_children,
    std::vector<SupersetPtr>* sets) {
  assert(inter_index > 0);
  std::string gate = indexed_tree_->GateType(inter_index);
  // Type dependent logic.
  if (gate == "or") {
    FaultTreeAnalysis::SetOr(1, events_children, sets);

  } else if (gate == "and") {
    FaultTreeAnalysis::SetAnd(1, events_children, sets);

  } else {
    boost::to_upper(gate);
    std::string msg = "No algorithm defined for " + gate;
    throw LogicError(msg);
  }
}

void FaultTreeAnalysis::ExpandNegativeGate(
    int inter_index,
    const std::set<int>& events_children,
    std::vector<SupersetPtr>* sets) {
  assert(inter_index > 0);
  std::string gate = indexed_tree_->GateType(inter_index);
  // Type dependent logic.
  if (gate == "or") {
    FaultTreeAnalysis::SetAnd(-1, events_children, sets);

  } else if (gate == "and") {
    FaultTreeAnalysis::SetOr(-1, events_children, sets);

  } else {
    boost::to_upper(gate);
    std::string msg = "No algorithm defined for " + gate;
    throw LogicError(msg);
  }
}

bool FaultTreeAnalysis::GetExpandedSets(int inter_index,
                                        std::vector< SupersetPtr >* sets) {
  assert(sets->empty());
  if (repeat_exp_.count(inter_index)) {
    std::vector<SupersetPtr>* repeat_set =
        &repeat_exp_.find(inter_index)->second;

    std::vector<SupersetPtr>::iterator it;
    for (it = repeat_set->begin(); it != repeat_set->end(); ++it) {
      SupersetPtr temp_set(new Superset(**it));
      sets->push_back(temp_set);
    }
    return true;
  } else {
    return false;
  }
}

void FaultTreeAnalysis::SaveExpandedSets(int inter_index,
                                         const std::vector<SupersetPtr>& sets) {
  std::vector<SupersetPtr> repeat_set;
  std::vector<SupersetPtr>::const_iterator it;
  for (it = sets.begin(); it != sets.end(); ++it) {
    SupersetPtr temp_set(new Superset(**it));
    repeat_set.push_back(temp_set);
  }
  repeat_exp_.insert(std::make_pair(inter_index, repeat_set));
}

void FaultTreeAnalysis::SetOr(int mult,
                              const std::set<int>& events_children,
                              std::vector<SupersetPtr>* sets) {
  assert(mult == 1 || mult == -1);
  std::set<int>::const_iterator it_child;
  for (it_child = events_children.begin();
       it_child != events_children.end(); ++it_child) {
    SupersetPtr tmp_set_c(new Superset());
    if (std::abs(*it_child) > top_event_index_) {
      tmp_set_c->InsertGate(*it_child * mult);
    } else {
      tmp_set_c->InsertPrimary(*it_child * mult);
    }
    sets->push_back(tmp_set_c);
  }
}

void FaultTreeAnalysis::SetAnd(int mult,
                               const std::set<int>& events_children,
                               std::vector<SupersetPtr>* sets) {
  assert(mult == 1 || mult == -1);
  SupersetPtr tmp_set_c(new Superset());
  std::set<int>::const_iterator it_child;
  for (it_child = events_children.begin();
       it_child != events_children.end(); ++it_child) {
    if (std::abs(*it_child) > top_event_index_) {
      tmp_set_c->InsertGate(*it_child * mult);
    } else {
      tmp_set_c->InsertPrimary(*it_child * mult);
    }
  }
  sets->push_back(tmp_set_c);
}

void FaultTreeAnalysis::FindMcs(
    const std::vector< const std::set<int>* >& cut_sets,
    const std::vector< std::set<int> >& mcs_lower_order,
    int min_order,
    std::vector< std::set<int> >* imcs) {
  if (cut_sets.empty()) return;

  // Iterator for cut_sets.
  std::vector< const std::set<int>* >::const_iterator it_uniq;

  // Iterator for minimal cut sets.
  std::vector< std::set<int> >::const_iterator it_min;

  std::vector< const std::set<int>* > temp_sets;  // For mcs of a level above.
  std::vector< std::set<int> > temp_min_sets;  // For mcs of this level.

  for (it_uniq = cut_sets.begin();
       it_uniq != cut_sets.end(); ++it_uniq) {
    bool include = true;  // Determine to keep or not.

    for (it_min = mcs_lower_order.begin(); it_min != mcs_lower_order.end();
         ++it_min) {
      if (std::includes((*it_uniq)->begin(), (*it_uniq)->end(),
                        it_min->begin(), it_min->end())) {
        // Non-minimal cut set is detected.
        include = false;
        break;
      }
    }
    // After checking for non-minimal cut sets,
    // all minimum sized cut sets are guaranteed to be minimal.
    if (include) {
      if ((*it_uniq)->size() == min_order) {
        temp_min_sets.push_back(**it_uniq);
        // Update maximum order of the sets.
        if (min_order > max_order_) max_order_ = min_order;
      } else {
        temp_sets.push_back(*it_uniq);
      }
    }
    // Ignore the cut set because include = false.
  }
  imcs->insert(imcs->end(), temp_min_sets.begin(), temp_min_sets.end());
  min_order++;
  FaultTreeAnalysis::FindMcs(temp_sets, temp_min_sets, min_order, imcs);
}

void FaultTreeAnalysis::AssignIndices(const FaultTreePtr& fault_tree) {
  // Getting events from the fault tree object.
  /// @note Direct assignment of the containers leads to very bad performance.
  /// @todo Very strange performance issue. Conflict between Expansion and
  /// Probability calculations.
  top_event_ = fault_tree->top_event();
  // inter_events_.insert(fault_tree->inter_events().begin(),
  //                      fault_tree->inter_events().end());
  primary_events_.insert(fault_tree->primary_events().begin(),
                         fault_tree->primary_events().end());
  // primary_events_ = fault_tree->primary_events();
  inter_events_ = fault_tree->inter_events();

  basic_events_.insert(fault_tree->basic_events().begin(),
                       fault_tree->basic_events().end());

  // Assign an index to each primary event, and populate relevant
  // databases.
  int j = 1;
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator itp;
  // Dummy primary event at index 0.
  int_to_primary_.push_back(PrimaryEventPtr(new PrimaryEvent("dummy")));
  for (itp = primary_events_.begin(); itp != primary_events_.end(); ++itp) {
    if (boost::dynamic_pointer_cast<HouseEvent>(itp->second)) {
      if (itp->second->p() == 0) {
        false_house_events_.insert(false_house_events_.end(), j);
      } else {
        true_house_events_.insert(true_house_events_.end(), j);
      }
    }
    int_to_primary_.push_back(itp->second);
    primary_to_int_.insert(std::make_pair(itp->first, j));
    all_to_int_.insert(std::make_pair(itp->first, j));
    ++j;
  }

  // Assign an index to each top and intermediate event and populate
  // relevant databases.
  top_event_index_ = j;
  int_to_inter_.insert(std::make_pair(j, top_event_));
  inter_to_int_.insert(std::make_pair(top_event_->id(), j));
  all_to_int_.insert(std::make_pair(top_event_->id(), j));
  ++j;
  boost::unordered_map<std::string, GatePtr>::iterator iti;
  for (iti = inter_events_.begin(); iti != inter_events_.end(); ++iti) {
    int_to_inter_.insert(std::make_pair(j, iti->second));
    inter_to_int_.insert(std::make_pair(iti->first, j));
    all_to_int_.insert(std::make_pair(iti->first, j));
    ++j;
  }

  indexed_tree_ = new IndexedFaultTree(top_event_index_, true_house_events_,
                                       false_house_events_, inter_to_int_,
                                       int_to_inter_, all_to_int_);
  indexed_tree_->InitiateIndexedFaultTree();
  indexed_tree_->ProcessIndexedFaultTree();
}

void FaultTreeAnalysis::SetsToString(const std::vector< std::set<int> >& imcs) {
  std::vector< std::set<int> >::const_iterator it_min;
  for (it_min = imcs.begin(); it_min != imcs.end(); ++it_min) {
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
