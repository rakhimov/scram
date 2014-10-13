/// @file probability_analysis.cc
/// Implementations of functions to provide probability and importance
/// informations.
#include "probability_analysis.h"

#include <ctime>

#include <boost/algorithm/string.hpp>

#include "error.h"

namespace scram {

ProbabilityAnalysis::ProbabilityAnalysis(std::string approx, int nsums,
                                         double cut_off)
    : warnings_(""),
      p_total_(0),
      num_prob_mcs_(-1),
      p_time_(-1) {
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
}


void ProbabilityAnalysis::UpdateDatabase(
    const boost::unordered_map<std::string, PrimaryEventPtr>& primary_events) {
  /// @todo Strange bottleneck upon direct assignment.
  primary_events_.clear();
  primary_events_.insert(primary_events.begin(), primary_events.end());
  ProbabilityAnalysis::AssignIndices();
}

void ProbabilityAnalysis::Analyze(
    const std::set< std::set<std::string> >& min_cut_sets) {
  min_cut_sets_ = min_cut_sets;
  // Update databases of minimal cut sets with indexed events.
  std::set< std::set<std::string> >::const_iterator it;
  for (it = min_cut_sets.begin(); it != min_cut_sets.end(); ++it) {
    std::set<int> mcs_with_indices;  // Minimal cut set with indices.
    std::set<std::string>::const_iterator it_set;
    for (it_set = it->begin(); it_set != it->end(); ++it_set) {
      std::vector<std::string> names;
      boost::split(names, *it_set, boost::is_any_of(" "),
                   boost::token_compress_on);
      assert(names.size() == 1 || names.size() == 2);
      if (names.size() == 1) {
        assert(primary_to_int_.count(names[0]));
        mcs_with_indices.insert(primary_to_int_.find(names[0])->second);
      } else {
        // This must be a complement of an event.
        assert(names[0] == "not");
        assert(primary_to_int_.count(names[1]));
        mcs_with_indices.insert(-primary_to_int_.find(names[1])->second);
      }
    }
    imcs_.push_back(mcs_with_indices);
    imcs_to_smcs_.insert(std::make_pair(mcs_with_indices, *it));
  }

  /// Minimal cut sets with higher than cut-off probability.
  std::set< std::set<int> > mcs_for_prob;
  // Iterate minimal cut sets and find probabilities for each set.
  std::vector< std::set<int> >::const_iterator it_min;
  for (it_min = imcs_.begin(); it_min != imcs_.end(); ++it_min) {
    // Calculate a probability of a set with AND relationship.
    double p_sub_set = ProbabilityAnalysis::ProbAnd(*it_min);
    if (p_sub_set > cut_off_) mcs_for_prob.insert(*it_min);

    // Update a container with minimal cut sets and probabilities.
    prob_of_min_sets_.insert(
        std::make_pair(imcs_to_smcs_.find(*it_min)->second, p_sub_set));
    ordered_min_sets_.insert(
        std::make_pair(p_sub_set, imcs_to_smcs_.find(*it_min)->second));
  }

  num_prob_mcs_ = mcs_for_prob.size();

  // Timing Initialization
  std::clock_t start_time;
  start_time = std::clock();

  // Get the total probability.
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
    if (nsums_ > mcs_for_prob.size()) nsums_ = mcs_for_prob.size();
    p_total_ = ProbabilityAnalysis::ProbOr(nsums_, &mcs_for_prob);
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
  // Duration of the calculations.
  p_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);
}

void ProbabilityAnalysis::AssignIndices() {
  // Cleanup the previous information.
  int_to_primary_.clear();
  primary_to_int_.clear();
  iprobs_.clear();
  // Indexation of events.
  int j = 1;
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator itp;
  // Dummy primary event at index 0.
  int_to_primary_.push_back(PrimaryEventPtr(new PrimaryEvent("dummy")));
  iprobs_.push_back(0);
  for (itp = primary_events_.begin(); itp != primary_events_.end(); ++itp) {
    int_to_primary_.push_back(itp->second);
    primary_to_int_.insert(std::make_pair(itp->second->id(), j));
    iprobs_.push_back(itp->second->p());
    ++j;
  }
}

double ProbabilityAnalysis::ProbOr(int nsums,
                                   std::set< std::set<int> >* min_cut_sets) {
  assert(nsums >= 0);

  // Recursive implementation.
  if (min_cut_sets->empty()) return 0;

  if (nsums == 0) return 0;

  // Base case.
  if (min_cut_sets->size() == 1) {
    // Get only element in this set.
    return ProbabilityAnalysis::ProbAnd(*min_cut_sets->begin());
  }

  // Get one element.
  std::set< std::set<int> >::iterator it = min_cut_sets->begin();
  std::set<int> element_one(*it);

  // Delete element from the original set. WARNING: the iterator is invalidated.
  min_cut_sets->erase(it);
  std::set< std::set<int> > combo_sets;
  ProbabilityAnalysis::CombineElAndSet(element_one, *min_cut_sets, &combo_sets);

  return ProbabilityAnalysis::ProbAnd(element_one) +
      ProbabilityAnalysis::ProbOr(nsums, min_cut_sets) -
      ProbabilityAnalysis::ProbOr(nsums - 1, &combo_sets);
}

double ProbabilityAnalysis::ProbAnd(const std::set<int>& min_cut_set) {
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

void ProbabilityAnalysis::CombineElAndSet(const std::set<int>& el,
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

}  // namespace scram
