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

namespace scram {

FaultTreeAnalysis::FaultTreeAnalysis(std::string analysis, std::string approx,
                                     int limit_order, int nsums,
                                     double cut_off)
    : warnings_(""),
      top_event_index_(-1),
      prob_requested_(false),
      max_order_(1),
      num_prob_mcs_(0),
      p_total_(0),
      exp_time_(0),
      mcs_time_(0),
      p_time_(0) {
  // Check for valid analysis type.
  if (analysis != "default" && analysis != "mc") {
    std::string msg = "The analysis type is not recognized.";
    throw scram::ValueError(msg);
  }
  analysis_ = analysis;

  // Check for right limit order.
  if (limit_order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than one.";
    throw scram::ValueError(msg);
  }
  limit_order_ = limit_order;

  // Check for right number of sums.
  if (nsums < 1) {
    std::string msg = "The number of sums in the probability calculation "
                      "cannot be less than one";
    throw scram::ValueError(msg);
  }
  nsums_ = nsums;

  // Check for valid cut-off probability.
  if (cut_off < 0 || cut_off > 1) {
    std::string msg = "The cut-off probability cannot be negative or"
                      " more than 1.";
    throw scram::ValueError(msg);
  }
  cut_off_ = cut_off;

  // Check the right approximation for probability calculations.
  if (approx != "no" && approx != "rare" && approx != "mcub") {
    std::string msg = "The probability approximation is not recognized.";
    throw scram::ValueError(msg);
  }
  approx_ = approx;

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

void FaultTreeAnalysis::Analyze(const FaultTreePtr& fault_tree,
                                bool prob_requested) {
  // Timing Initialization
  std::clock_t start_time;
  start_time = std::clock();
  // End of Timing Initialization

  // Pre-process the tree.
  FaultTreeAnalysis::PreprocessTree(fault_tree->top_event());

  // Container for cut sets with primary events only.
  std::vector<SupersetPtr> cut_sets;

  prob_requested_ = prob_requested;

  FaultTreeAnalysis::AssignIndices(fault_tree);

  SupersetPtr top_set(new Superset());
  top_set->InsertGate(top_event_index_);
  ExpandTree(top_set, &cut_sets);

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

  imcs_.reserve(unique_cut_sets.size());
  std::vector<const std::set<int>* > sets_unique;
  std::set< const std::set<int>*, SetPtrComp >::iterator it_un;
  for (it_un = unique_cut_sets.begin(); it_un != unique_cut_sets.end();
       ++it_un) {
    if ((*it_un)->size() == 1) {
      // Minimal cut set is detected.
      imcs_.push_back(**it_un);
      continue;
    }
    sets_unique.push_back(*it_un);
  }

  FaultTreeAnalysis::FindMcs(sets_unique, imcs_, 2);
  // Duration of MCS generation.
  mcs_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);
  FaultTreeAnalysis::SetsToString();  // MCS with event ids.

  // Compute probabilities only if requested.
  if (!prob_requested_) return;

  // Maximum number of sums in the series.
  if (nsums_ > imcs_.size()) nsums_ = imcs_.size();

  // Perform Monte Carlo Uncertainty analysis.
  if (analysis_ == "mc") {
    std::set<std::set<int> > iset(imcs_.begin(), imcs_.end());
    // Generate the equation.
    FaultTreeAnalysis::MProbOr(1, nsums_, &iset);
    // Sample probabilities and generate data.
    FaultTreeAnalysis::MSample();
    return;
  }

  // Iterator for minimal cut sets.
  std::vector< std::set<int> >::iterator it_min;

  /// Minimal cut sets with higher than cut-off probability.
  std::set< std::set<int> > mcs_for_prob;
  // Iterate minimal cut sets and find probabilities for each set.
  for (it_min = imcs_.begin(); it_min != imcs_.end(); ++it_min) {
    // Calculate a probability of a set with AND relationship.
    double p_sub_set = FaultTreeAnalysis::ProbAnd(*it_min);
    if (p_sub_set > cut_off_) mcs_for_prob.insert(*it_min);

    // Update a container with minimal cut sets and probabilities.
    prob_of_min_sets_.insert(
        std::make_pair(imcs_to_smcs_.find(*it_min)->second, p_sub_set));
    ordered_min_sets_.insert(
        std::make_pair(p_sub_set, imcs_to_smcs_.find(*it_min)->second));
  }

  // Check if the rare event approximation is requested.
  if (approx_ == "rare") {
    warnings_ += "Using the rare event approximation\n";
    bool rare_event_legit = true;
    std::map< std::set<std::string>, double >::iterator it_pr;
    for (it_pr = prob_of_min_sets_.begin();
         it_pr != prob_of_min_sets_.end(); ++it_pr) {
      // Check if a probability of a set does not exceed 0.1,
      // which is required for the rare event approximation to hold.
      if (rare_event_legit && (it_pr->second > 0.1)) {
        rare_event_legit = false;
        warnings_ += "The rare event approximation may be inaccurate for this"
            "\nfault tree analysis because one of minimal cut sets'"
            "\nprobability exceeded 0.1 threshold requirement.\n\n";
      }
      p_total_ += it_pr->second;
    }

  } else if (approx_ == "mcub") {
    warnings_ += "Using the MCUB approximation\n";
    double m = 1;
    std::map< std::set<std::string>, double >::iterator it;
    for (it = prob_of_min_sets_.begin(); it != prob_of_min_sets_.end();
         ++it) {
      m *= 1 - it->second;
    }
    p_total_ = 1 - m;

  } else {  // The default calculations.
    // Choose cut sets with high enough probabilities.
    p_total_ = FaultTreeAnalysis::ProbOr(nsums_, &mcs_for_prob);
  }

  // Calculate failure contributions of each primary event.
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator it_p;
  for (it_p = primary_events_.begin(); it_p != primary_events_.end();
       ++it_p) {
    double contrib_pos = 0;  // Total positive contribution of this event.
    double contrib_neg = 0;  // Negative event contribution.
    std::map< std::set<std::string>, double >::iterator it_pr;
    for (it_pr = prob_of_min_sets_.begin();
         it_pr != prob_of_min_sets_.end(); ++it_pr) {
      if (it_pr->first.count(it_p->first)) {
        contrib_pos += it_pr->second;
      } else if (it_pr->first.count("not " + it_p->first)) {
        contrib_neg += it_pr->second;
      }
    }
    imp_of_primaries_.insert(std::make_pair(it_p->first, contrib_pos));
    ordered_primaries_.insert(std::make_pair(contrib_pos, it_p->first));
    if (contrib_neg > 0) {
      imp_of_primaries_.insert(std::make_pair("not " + it_p->first,
                                              contrib_neg));
      ordered_primaries_.insert(std::make_pair(contrib_neg,
                                               "not " + it_p->first));
    }
  }
  // Duration of probability related operations.
  p_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);
}

void FaultTreeAnalysis::PreprocessTree(const GatePtr& gate) {
  std::string parent = gate->type();
  std::map<std::string, EventPtr>::const_iterator it;
  for (it = gate->children().begin(); it != gate->children().end(); ++it) {
    GatePtr child_gate = boost::dynamic_pointer_cast<scram::Gate>(it->second);
    if (!child_gate) continue;
    std::string child = child_gate->type();
    if (((parent == "and" || parent == "or") && parent == child) ||
        (child == "null") ||
        (parent == "nand" && child == "and") ||
        (parent == "nor" && child == "or")) {
      gate->MergeGate(child_gate);
      it = gate->children().begin();
    } else {
      FaultTreeAnalysis::PreprocessTree(child_gate);
    }
  }
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
  // Assumes sets are empty.
  assert(sets->empty());
  if (repeat_exp_.count(inter_index)) {
    std::vector<SupersetPtr>* repeat_set =
        &repeat_exp_.find(inter_index)->second;

    std::vector<SupersetPtr>::iterator it;
    for (it = repeat_set->begin(); it != repeat_set->end(); ++it) {
      SupersetPtr temp_set(new Superset);
      temp_set->InsertSet(*it);
      sets->push_back(temp_set);
    }
    return;
  }

  // Populate intermediate and primary events of the top.
  const std::map<std::string, EventPtr>* children =
      &int_to_inter_.find(std::abs(inter_index))->second->children();

  std::string gate = int_to_inter_.find(std::abs(inter_index))->second->type();

  // Iterator for children of top and intermediate events.
  std::map<std::string, EventPtr>::const_iterator it_children;
  std::vector<int> events_children;
  std::vector<int>::iterator it_child;

  for (it_children = children->begin();
       it_children != children->end(); ++it_children) {
    if (inter_events_.count(it_children->first)) {
      events_children.push_back(inter_to_int_.find(it_children->first)->second);
    } else {
      events_children.push_back(
          primary_to_int_.find(it_children->first)->second);
    }
  }

  // Type dependent logic.
  if (gate == "or") {
    assert(events_children.size() > 1);
    if (inter_index > 0) {
      FaultTreeAnalysis::SetOr(1, events_children, sets);
    } else {
      FaultTreeAnalysis::SetAnd(-1, events_children, sets);
    }
  } else if (gate == "and") {
    assert(events_children.size() > 1);
    if (inter_index > 0) {
      FaultTreeAnalysis::SetAnd(1, events_children, sets);
    } else {
      FaultTreeAnalysis::SetOr(-1, events_children, sets);
    }
  } else if (gate == "not") {
    int mult = 1;
    if (inter_index < 0) mult = -1;
    // Only one child is expected.
    assert(events_children.size() == 1);
    FaultTreeAnalysis::SetAnd(-1 * mult, events_children, sets);
  } else if (gate == "nor") {
    assert(events_children.size() > 1);
    if (inter_index > 0) {
      FaultTreeAnalysis::SetAnd(-1, events_children, sets);
    } else {
      FaultTreeAnalysis::SetOr(1, events_children, sets);
    }
  } else if (gate == "nand") {
    assert(events_children.size() > 1);
    if (inter_index > 0) {
      FaultTreeAnalysis::SetOr(-1, events_children, sets);
    } else {
      FaultTreeAnalysis::SetAnd(1, events_children, sets);
    }
  } else if (gate == "xor") {
    assert(events_children.size() == 2);
    SupersetPtr tmp_set_one(new scram::Superset());
    SupersetPtr tmp_set_two(new scram::Superset());
    if (inter_index > 0) {
      int j = 1;
      for (it_child = events_children.begin();
           it_child != events_children.end(); ++it_child) {
        if (*it_child > top_event_index_) {
          tmp_set_one->InsertGate(j * (*it_child));
          tmp_set_two->InsertGate(-1 * j * (*it_child));
        } else {
          tmp_set_one->InsertPrimary(j * (*it_child));
          tmp_set_two->InsertPrimary(-1 * j * (*it_child));
        }
        j = -1;
      }
    } else {
      for (it_child = events_children.begin();
           it_child != events_children.end(); ++it_child) {
        if (*it_child > top_event_index_) {
          tmp_set_one->InsertGate(*it_child);
          tmp_set_two->InsertGate(-1 * (*it_child));
        } else {
          tmp_set_one->InsertPrimary(*it_child);
          tmp_set_two->InsertPrimary(-1 * (*it_child));
        }
      }
    }
    sets->push_back(tmp_set_one);
    sets->push_back(tmp_set_two);
  } else if (gate == "null") {
    int mult = 1;
    if (inter_index < 0) mult = -1;
    // Only one child is expected.
    assert(events_children.size() == 1);
    FaultTreeAnalysis::SetAnd(mult, events_children, sets);
  } else if (gate == "inhibit") {
    assert(events_children.size() == 2);
    if (inter_index > 0) {
      FaultTreeAnalysis::SetAnd(1, events_children, sets);
    } else {
      FaultTreeAnalysis::SetOr(-1, events_children, sets);
    }
  } else if (gate == "vote" || gate == "atleast") {
    int vote_number =
        int_to_inter_.find(std::abs(inter_index))->second->vote_number();

    assert(vote_number > 1);
    assert(events_children.size() >= vote_number);
    std::set< std::set<int> > all_sets;
    int size = events_children.size();

    for (int j = 0; j < size; ++j) {
      std::set<int> set;
      set.insert(events_children[j]);
      all_sets.insert(set);
    }

    int mult = 1;
    if (inter_index < 0) {
      mult = -1;
      vote_number = size - vote_number + 1;  // The main trick for negation.
    }

    for (int i = 1; i < vote_number; ++i) {
      std::set< std::set<int> > tmp_sets;
      std::set< std::set<int> >::iterator it_sets;
      for (it_sets = all_sets.begin(); it_sets != all_sets.end(); ++it_sets) {
        for (int j = 0; j < size; ++j) {
          std::set<int> set = *it_sets;
          set.insert(events_children[j]);
          if (set.size() > i) {
            tmp_sets.insert(set);
          }
        }
      }
      all_sets = tmp_sets;
    }

    std::set< std::set<int> >::iterator it_sets;
    for (it_sets = all_sets.begin(); it_sets != all_sets.end(); ++it_sets) {
      SupersetPtr tmp_set_c(new scram::Superset());
      std::set<int>::iterator it;
      for (it = it_sets->begin(); it != it_sets->end(); ++it) {
        if (*it > top_event_index_) {
          tmp_set_c->InsertGate(*it * mult);
        } else {
          tmp_set_c->InsertPrimary(*it * mult);
        }
      }
      sets->push_back(tmp_set_c);
    }

  } else {
    boost::to_upper(gate);
    std::string msg = "No algorithm defined for " + gate;
    throw scram::ValueError(msg);
  }

  // Save the expanded sets in case this gate gets repeated.
  std::vector<SupersetPtr> repeat_set;
  std::vector<SupersetPtr>::iterator it;
  for (it = sets->begin(); it != sets->end(); ++it) {
    SupersetPtr temp_set(new Superset);
    temp_set->InsertSet(*it);
    repeat_set.push_back(temp_set);
  }
  repeat_exp_.insert(std::make_pair(inter_index, repeat_set));
}

void FaultTreeAnalysis::SetOr(int mult,
                              const std::vector<int>& events_children,
                              std::vector<SupersetPtr>* sets) {
  assert(mult == 1 || mult == -1);
  std::vector<int>::const_iterator it_child;
  for (it_child = events_children.begin();
       it_child != events_children.end(); ++it_child) {
    SupersetPtr tmp_set_c(new scram::Superset());
    if (*it_child > top_event_index_) {
      tmp_set_c->InsertGate(*it_child * mult);
    } else {
      tmp_set_c->InsertPrimary(*it_child * mult);
    }
    sets->push_back(tmp_set_c);
  }
}

void FaultTreeAnalysis::SetAnd(int mult,
                               const std::vector<int>& events_children,
                               std::vector<SupersetPtr>* sets) {
  assert(mult == 1 || mult == -1);
  SupersetPtr tmp_set_c(new scram::Superset());
  std::vector<int>::const_iterator it_child;
  for (it_child = events_children.begin();
       it_child != events_children.end(); ++it_child) {
    if (*it_child > top_event_index_) {
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
    int min_order) {
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
  imcs_.insert(imcs_.end(), temp_min_sets.begin(), temp_min_sets.end());
  min_order++;
  FaultTreeAnalysis::FindMcs(temp_sets, temp_min_sets, min_order);
}

// -------------------- Algorithm for Cut Sets and Probabilities -----------
void FaultTreeAnalysis::AssignIndices(const FaultTreePtr& fault_tree) {
  // Assign an index to each primary event, and populate relevant
  // databases.

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

  int j = 1;
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator itp;
  // Dummy primary event at index 0.
  int_to_primary_.push_back(PrimaryEventPtr(new PrimaryEvent("dummy")));
  iprobs_.push_back(0);
  for (itp = primary_events_.begin(); itp != primary_events_.end(); ++itp) {
    int_to_primary_.push_back(itp->second);
    primary_to_int_.insert(std::make_pair(itp->second->id(), j));
    if (prob_requested_) iprobs_.push_back(itp->second->p());
    ++j;
  }

  // Assign an index to each top and intermediate event and populate
  // relevant databases.
  top_event_index_ = j;
  int_to_inter_.insert(std::make_pair(j, top_event_));
  inter_to_int_.insert(std::make_pair(top_event_->id(), j));
  ++j;
  boost::unordered_map<std::string, GatePtr>::iterator iti;
  for (iti = inter_events_.begin(); iti != inter_events_.end(); ++iti) {
    int_to_inter_.insert(std::make_pair(j, iti->second));
    inter_to_int_.insert(std::make_pair(iti->second->id(), j));
    ++j;
  }
}

void FaultTreeAnalysis::SetsToString() {
  std::vector< std::set<int> >::iterator it_min;
  for (it_min = imcs_.begin(); it_min != imcs_.end(); ++it_min) {
    std::set<std::string> pr_set;
    std::set<int>::iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      if (*it_set < 0) {  // NOT logic.
        pr_set.insert("not " + int_to_primary_[std::abs(*it_set)]->id());
      } else {
        pr_set.insert(int_to_primary_[*it_set]->id());
      }
    }
    imcs_to_smcs_.insert(std::make_pair(*it_min, pr_set));
    min_cut_sets_.insert(pr_set);
  }
}

double FaultTreeAnalysis::ProbOr(int nsums,
                                 std::set< std::set<int> >* min_cut_sets) {
  assert(nsums >= 0);

  // Recursive implementation.
  if (min_cut_sets->empty()) return 0;

  if (nsums == 0) return 0;

  // Base case.
  if (min_cut_sets->size() == 1) {
    // Get only element in this set.
    return FaultTreeAnalysis::ProbAnd(*min_cut_sets->begin());
  }

  // Get one element.
  std::set< std::set<int> >::iterator it = min_cut_sets->begin();
  std::set<int> element_one(*it);

  // Delete element from the original set. WARNING: the iterator is invalidated.
  min_cut_sets->erase(it);
  std::set< std::set<int> > combo_sets;
  FaultTreeAnalysis::CombineElAndSet(element_one, *min_cut_sets, &combo_sets);

  return FaultTreeAnalysis::ProbAnd(element_one) +
         FaultTreeAnalysis::ProbOr(nsums, min_cut_sets) -
         FaultTreeAnalysis::ProbOr(nsums - 1, &combo_sets);
}

double FaultTreeAnalysis::ProbAnd(const std::set<int>& min_cut_set) {
  // Test just in case the min cut set is empty.
  if (min_cut_set.empty()) return 0;

  double p_sub_set = 1;  // 1 is for multiplication.
  std::set<int>::iterator it_set;
  for (it_set = min_cut_set.begin(); it_set != min_cut_set.end(); ++it_set) {
    if (*it_set > 0) {
      p_sub_set *= iprobs_[*it_set];
    } else {
      p_sub_set *= 1 - iprobs_[std::abs(*it_set)];  // Never zero.
    }
  }
  return p_sub_set;
}

void FaultTreeAnalysis::CombineElAndSet(const std::set<int>& el,
                                        const std::set< std::set<int> >& set,
                                        std::set< std::set<int> >* combo_set) {
  std::set< std::set<int> >::iterator it_set;
  for (it_set = set.begin(); it_set != set.end(); ++it_set) {
    bool include = true;  // Indicates that the resultant set is not null.
    std::set<int>::iterator it;
    for (it = el.begin(); it != el.end(); ++it) {
      if (it_set->count(-*it)) {
        include = false;
        break;  // A complement is found; the set is null.
      }
    }
    if (include) {
      std::set<int> member_set(*it_set);
      member_set.insert(el.begin(), el.end());
      combo_set->insert(combo_set->end(), member_set);
    }
  }
}

// ----------------------------------------------------------------------
// ----- Algorithm for Total Equation for Monte Carlo Simulation --------
// Generation of the representation of the original equation.
void FaultTreeAnalysis::MProbOr(int sign, int nsums,
                                std::set< std::set<int> >* min_cut_sets) {
  assert(sign != 0);
  assert(nsums >= 0);

  // Recursive implementation.
  if (min_cut_sets->empty()) return;

  if (nsums == 0) return;

  // Get one element.
  std::set< std::set<int> >::iterator it = min_cut_sets->begin();
  std::set<int> element_one = *it;

  // Delete element from the original set. WARNING: the iterator is invalidated.
  min_cut_sets->erase(it);

  // Put this element into the equation.
  if (sign > 0) {
    // This is a positive member.
    pos_terms_.push_back(element_one);
  } else {
    // This must be a negative member.
    neg_terms_.push_back(element_one);
  }

  std::set< std::set<int> > combo_sets;
  FaultTreeAnalysis::CombineElAndSet(element_one, *min_cut_sets, &combo_sets);
  FaultTreeAnalysis::MProbOr(sign, nsums, min_cut_sets);
  FaultTreeAnalysis::MProbOr(-sign, nsums - 1, &combo_sets);
}

void FaultTreeAnalysis::MSample() {}
// ----------------------------------------------------------------------

}  // namespace scram
