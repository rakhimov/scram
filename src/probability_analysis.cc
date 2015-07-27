/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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
/// @file probability_analysis.cc
/// Implementations of functions to provide probability and importance
/// informations.
#include "probability_analysis.h"

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>

#include "error.h"
#include "logger.h"

namespace scram {

ProbabilityAnalysis::ProbabilityAnalysis(const std::string& approx,
                                         int num_sums,
                                         double cut_off,
                                         bool importance_analysis)
    : importance_analysis_(importance_analysis),
      warnings_(""),
      p_total_(0),
      p_rare_(0),
      coherent_(true),
      p_time_(-1),
      imp_time_(-1) {
  // Check for right number of sums.
  if (num_sums < 1) {
    std::string msg = "The number of sums in the probability calculation "
                      "cannot be less than one";
    throw InvalidArgument(msg);
  }
  num_sums_ = num_sums;

  // Check for valid cut-off probability.
  if (cut_off < 0 || cut_off > 1) {
    std::string msg = "The cut-off probability cannot be negative or"
                      " more than 1.";
    throw InvalidArgument(msg);
  }
  cut_off_ = cut_off;

  // Check the right approximation for probability calculations.
  if (approx != "no" && approx != "rare-event" && approx != "mcub") {
    std::string msg = "The probability approximation is not recognized.";
    throw InvalidArgument(msg);
  }
  approx_ = approx;
}

void ProbabilityAnalysis::UpdateDatabase(
    const boost::unordered_map<std::string, BasicEventPtr>& basic_events) {
  basic_events_ = basic_events;
  ProbabilityAnalysis::AssignIndices();
}

void ProbabilityAnalysis::Analyze(
    const std::set< std::set<std::string> >& min_cut_sets) {
  min_cut_sets_ = min_cut_sets;

  // Special case of unity with empty sets.
  if (min_cut_sets_.size() == 1 && min_cut_sets_.begin()->empty()) {
    warnings_ += "Probability for UNITY case.";
    p_total_ = 1;
    prob_of_min_sets_.insert(std::make_pair(*min_cut_sets_.begin(), 1));
    return;
  }

  ProbabilityAnalysis::IndexMcs(min_cut_sets_);

  using boost::container::flat_set;
  // Minimal cut sets with higher than cut-off probability.
  std::set< flat_set<int> > mcs_for_prob;
  // Iterate minimal cut sets and find probabilities for each set.
  std::vector< flat_set<int> >::const_iterator it_min;
  int i = 0;  // Indices for minimal cut sets in the vector.
  for (it_min = imcs_.begin(); it_min != imcs_.end(); ++i, ++it_min) {
    // Calculate a probability of a set with AND relationship.
    double p_sub_set = ProbabilityAnalysis::ProbAnd(*it_min);
    if (p_sub_set > cut_off_) {
      flat_set<int> mcs(*it_min);
      mcs_for_prob.insert(mcs_for_prob.end(), mcs);
    }

    // Update a container with minimal cut sets and probabilities.
    prob_of_min_sets_.insert(std::make_pair(imcs_to_smcs_[i], p_sub_set));
    p_rare_ += p_sub_set;
  }

  CLOCK(p_time);
  // Get the total probability.
  if (approx_ == "mcub") {
    if (!coherent_) {
      warnings_ += " The cut sets are not coherent and contain negation."
                   " The MCUB approximation may not hold.";
    }
    num_sums_ = 0;  // For reporting purposes.
    p_total_ = ProbabilityAnalysis::ProbMcub(imcs_);

  } else {
    // Check if the rare event approximation is requested.
    if (approx_ == "rare-event") {
      std::map< std::set<std::string>, double >::iterator it_pr;
      for (it_pr = prob_of_min_sets_.begin();
           it_pr != prob_of_min_sets_.end(); ++it_pr) {
        // Check if a probability of a set does not exceed 0.1,
        // which is required for the rare event approximation to hold.
        if (it_pr->second > 0.1) {
          warnings_ += " The rare event approximation may be inaccurate for"
                       " this analysis because one of minimal cut sets'"
                       " probability exceeds the 0.1 threshold requirement.";
          break;
        }
      }
      num_sums_ = 1;  // Only first series is used for the rare event case.
    }
    // The default calculations.
    // Choose cut sets with high enough probabilities.
    if (num_sums_ > mcs_for_prob.size()) num_sums_ = mcs_for_prob.size();
    ProbabilityAnalysis::ProbOr(1, num_sums_, &mcs_for_prob);
    p_total_ = ProbabilityAnalysis::CalculateTotalProbability();
  }
  p_time_ = DUR(p_time);
  CLOCK(imp_time);
  if (importance_analysis_) ProbabilityAnalysis::PerformImportanceAnalysis();
  imp_time_ = DUR(imp_time);
}

void ProbabilityAnalysis::AssignIndices() {
  // Cleanup the previous information.
  int_to_basic_.clear();
  basic_to_int_.clear();
  iprobs_.clear();
  // Indexation of events.
  int j = 1;
  boost::unordered_map<std::string, BasicEventPtr>::iterator itp;
  // Dummy basic event at index 0.
  int_to_basic_.push_back(BasicEventPtr(new BasicEvent("dummy")));
  iprobs_.push_back(0);
  for (itp = basic_events_.begin(); itp != basic_events_.end(); ++itp) {
    int_to_basic_.push_back(itp->second);
    basic_to_int_.insert(std::make_pair(itp->second->id(), j));
    iprobs_.push_back(itp->second->p());
    ++j;
  }
}

void ProbabilityAnalysis::IndexMcs(
    const std::set<std::set<std::string> >& min_cut_sets) {
  // Update databases of minimal cut sets with indexed events.
  std::set< std::set<std::string> >::const_iterator it;
  for (it = min_cut_sets.begin(); it != min_cut_sets.end(); ++it) {
    using boost::container::flat_set;
    flat_set<int> mcs_with_indices;  // Minimal cut set with indices.
    std::set<std::string>::const_iterator it_set;
    for (it_set = it->begin(); it_set != it->end(); ++it_set) {
      std::vector<std::string> names;
      boost::split(names, *it_set, boost::is_any_of(" "),
                   boost::token_compress_on);
      assert(names.size() >= 1);
      if (names[0] == "not") {
        std::string comp_name = *it_set;
        boost::replace_first(comp_name, "not ", "");
        // This must be a complement of an event.
        assert(basic_to_int_.count(comp_name));

        if (coherent_) coherent_ = false;  // Detected non-coherence.

        mcs_with_indices.insert(mcs_with_indices.begin(),
                                -basic_to_int_.find(comp_name)->second);
        mcs_basic_events_.insert(basic_to_int_.find(comp_name)->second);

      } else {
        assert(basic_to_int_.count(*it_set));
        mcs_with_indices.insert(mcs_with_indices.end(),
                                basic_to_int_.find(*it_set)->second);
        mcs_basic_events_.insert(basic_to_int_.find(*it_set)->second);
      }
    }
    imcs_.push_back(mcs_with_indices);
    imcs_to_smcs_.push_back(*it);
  }
}

double ProbabilityAnalysis::ProbMcub(
    const std::vector< boost::container::flat_set<int> >& min_cut_sets) {
  using boost::container::flat_set;
  std::vector< flat_set<int> >::const_iterator it_min;
  double m = 1;
  for (it_min = min_cut_sets.begin(); it_min != min_cut_sets.end(); ++it_min) {
    // Calculate a probability of a set with AND relationship.
    m *= 1 - ProbabilityAnalysis::ProbAnd(*it_min);
  }
  return 1 - m;
}

void ProbabilityAnalysis::ProbOr(
    int sign,
    int num_sums,
    std::set< boost::container::flat_set<int> >* min_cut_sets) {
  assert(sign != 0);
  assert(num_sums >= 0);

  // Recursive implementation.
  if (min_cut_sets->empty()) return;

  if (num_sums == 0) return;

  using boost::container::flat_set;

  // Put this element into the equation.
  if (sign > 0) {
    // This is a positive member.
    pos_terms_.push_back(*min_cut_sets->begin());
  } else {
    // This must be a negative member.
    neg_terms_.push_back(*min_cut_sets->begin());
  }

  // Delete element from the original set.
  min_cut_sets->erase(min_cut_sets->begin());

  std::set< flat_set<int> > combo_sets;
  ProbabilityAnalysis::CombineElAndSet(
      (sign > 0) ? pos_terms_.back() : neg_terms_.back(),
      *min_cut_sets, &combo_sets);

  ProbabilityAnalysis::ProbOr(sign, num_sums, min_cut_sets);
  ProbabilityAnalysis::ProbOr(-sign, num_sums - 1, &combo_sets);
}

double ProbabilityAnalysis::ProbAnd(
    const boost::container::flat_set<int>& min_cut_set) {
  // Test just in case the min cut set is empty.
  if (min_cut_set.empty()) return 0;

  using boost::container::flat_set;
  double p_sub_set = 1;  // 1 is for multiplication.
  flat_set<int>::const_iterator it_set;
  for (it_set = min_cut_set.begin(); it_set != min_cut_set.end(); ++it_set) {
    if (*it_set > 0) {
      p_sub_set *= iprobs_[*it_set];
    } else {
      p_sub_set *= 1 - iprobs_[std::abs(*it_set)];  // Never zero.
    }
  }
  return p_sub_set;
}

void ProbabilityAnalysis::CombineElAndSet(
    const boost::container::flat_set<int>& el,
    const std::set< boost::container::flat_set<int> >& set,
    std::set< boost::container::flat_set<int> >* combo_set) {
  using boost::container::flat_set;
  std::set< flat_set<int> >::iterator it_set;
  for (it_set = set.begin(); it_set != set.end(); ++it_set) {
    bool include = true;  // Indicates that the resultant set is not null.
    if (!coherent_) {
      flat_set<int>::const_iterator it;
      for (it = el.begin(); it != el.end(); ++it) {
        if (it_set->count(-*it)) {
          include = false;
          break;  // A complement is found; the set is null.
        }
      }
    }
    if (include) {
      flat_set<int> member_set(*it_set);
      member_set.insert(el.begin(), el.end());
      combo_set->insert(combo_set->end(), member_set);
    }
  }
}

double ProbabilityAnalysis::CalculateTotalProbability() {
  using boost::container::flat_set;
  double pos = 0;
  double neg = 0;
  std::vector< flat_set<int> >::iterator it_s;
  for (it_s = pos_terms_.begin(); it_s != pos_terms_.end(); ++it_s) {
    pos += ProbabilityAnalysis::ProbAnd(*it_s);
  }
  for (it_s = neg_terms_.begin(); it_s != neg_terms_.end(); ++it_s) {
    neg += ProbabilityAnalysis::ProbAnd(*it_s);
  }
  return pos - neg;
}

void ProbabilityAnalysis::PerformImportanceAnalysis() {
  // The main data for all the importance types is P(top/event) or
  // P(top/Not event).
  std::set<int>::iterator it;
  for (it = mcs_basic_events_.begin(); it != mcs_basic_events_.end(); ++it) {
    // Calculate P(top/event)
    iprobs_[*it] = 1;
    double p_e = 0;
    if (approx_ == "mcub") {
      p_e = ProbabilityAnalysis::ProbMcub(imcs_);
    } else {  // For the rare event and default cases.
      p_e = ProbabilityAnalysis::CalculateTotalProbability();
    }

    // Calculate P(top/Not event)
    iprobs_[*it] = 0;
    double p_not_e = 0;
    if (approx_ == "mcub") {
      p_not_e = ProbabilityAnalysis::ProbMcub(imcs_);
    } else {  // For the rare event and default cases.
      p_not_e = ProbabilityAnalysis::CalculateTotalProbability();
    }
    // Restore the probability.
    iprobs_[*it] = int_to_basic_[*it]->p();

    // Mapped vector for importance factors.
    std::vector<double> imp;
    // Fussel-Vesely Diagnosis importance factor.
    imp.push_back(1 - p_not_e / p_total_);
    // Birnbaum Marginal importance factor.
    imp.push_back(p_e - p_not_e);
    // Critical Importance factor.
    imp.push_back(imp[1] * iprobs_[*it] / p_not_e);
    // Risk Reduction Worth.
    imp.push_back(p_total_ / p_not_e);
    // Risk Achievement Worth.
    imp.push_back(p_e / p_total_);

    importance_.insert(std::make_pair(int_to_basic_[*it]->id(), imp));
  }
}

}  // namespace scram
