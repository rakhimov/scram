/// @file uncertainty_analysis.cc
/// Implements the functionality to run Monte Carlo uncertainty analysis.
#include "uncertainty_analysis.h"

#include <ctime>

#include <boost/algorithm/string.hpp>

#include "error.h"

namespace scram {

UncertaintyAnalysis::UncertaintyAnalysis(int nsums) : p_time_(-1) {
  // Check for right number of sums.
  if (nsums < 1) {
    std::string msg = "The number of sums in the probability calculation "
                      "cannot be less than one";
    throw scram::ValueError(msg);
  }
  nsums_ = nsums;
}

void UncertaintyAnalysis::UpdateDatabase(
    const boost::unordered_map<std::string, PrimaryEventPtr>& primary_events) {
  primary_events_ = primary_events;
  UncertaintyAnalysis::AssignIndices();
}

void UncertaintyAnalysis::Analyze(
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
  }

  // Timing Initialization
  std::clock_t start_time;
  start_time = std::clock();

  // Maximum number of sums in the series.
  if (nsums_ > imcs_.size()) nsums_ = imcs_.size();

  std::set<std::set<int> > iset(imcs_.begin(), imcs_.end());
  // Generate the equation.
  UncertaintyAnalysis::MProbOr(1, nsums_, &iset);
  // Sample probabilities and generate data.
  UncertaintyAnalysis::MSample();

  // Duration of the calculations.
  p_time_ = (std::clock() - start_time) / static_cast<double>(CLOCKS_PER_SEC);
}

void UncertaintyAnalysis::AssignIndices() {
  // Cleanup the previous information.
  int_to_primary_.clear();
  primary_to_int_.clear();
  // Indexation of events.
  int j = 1;
  boost::unordered_map<std::string, PrimaryEventPtr>::iterator itp;
  // Dummy primary event at index 0.
  int_to_primary_.push_back(PrimaryEventPtr(new PrimaryEvent("dummy")));
  for (itp = primary_events_.begin(); itp != primary_events_.end(); ++itp) {
    int_to_primary_.push_back(itp->second);
    primary_to_int_.insert(std::make_pair(itp->second->id(), j));
    ++j;
  }
}

void UncertaintyAnalysis::CombineElAndSet(const std::set<int>& el,
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

// Generation of the representation of the original equation.
void UncertaintyAnalysis::MProbOr(int sign, int nsums,
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
  UncertaintyAnalysis::CombineElAndSet(element_one, *min_cut_sets, &combo_sets);
  UncertaintyAnalysis::MProbOr(sign, nsums, min_cut_sets);
  UncertaintyAnalysis::MProbOr(-sign, nsums - 1, &combo_sets);
}

void UncertaintyAnalysis::MSample() {}

}  // namespace scram
