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
#ifndef SCRAM_TESTS_PROBABILITY_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_PROBABILITY_ANALYSIS_TESTS_H_

#include "probability_analysis.h"

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace scram;

class ProbabilityAnalysisTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    prob_analysis = new ProbabilityAnalysis();
  }

  virtual void TearDown() {
    delete prob_analysis;
  }

  double ProbAnd(const std::set<int>& min_cut_set) {
    using boost::container::flat_set;
    flat_set<int> mcs(min_cut_set.begin(), min_cut_set.end());
    return prob_analysis->ProbAnd(mcs);
  }

  double ProbOr(std::set< std::set<int> >* min_cut_sets) {
    return ProbabilityAnalysisTest::ProbOr(1000, min_cut_sets);
  }

  double ProbOr(int nsums, std::set< std::set<int> >* min_cut_sets) {
    using boost::container::flat_set;
    std::set< flat_set<int> > mcs;
    std::set< std::set<int> >::iterator it;
    for (it = min_cut_sets->begin(); it != min_cut_sets->end(); ++it) {
      flat_set<int> fs(it->begin(), it->end());
      mcs.insert(mcs.end(), fs);
    }
    prob_analysis->pos_terms_.clear();
    prob_analysis->neg_terms_.clear();
    prob_analysis->ProbOr(1, nsums, &mcs);
    return prob_analysis->CalculateTotalProbability();
  }

  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >* combo_set) {
    prob_analysis->coherent_ = false;  // General case.
    using boost::container::flat_set;
    std::set< flat_set<int> > mcs;
    std::set< std::set<int> >::const_iterator it;
    for (it = set.begin(); it != set.end(); ++it) {
      flat_set<int> fs(it->begin(), it->end());
      mcs.insert(mcs.end(), fs);
    }
    std::set< flat_set<int> > f_comb;
    flat_set<int> f_el(el.begin(), el.end());
    prob_analysis->CombineElAndSet(f_el, mcs, &f_comb);
    std::set< flat_set<int> >::iterator fit;
    for (fit = f_comb.begin(); fit != f_comb.end(); ++fit) {
      std::set<int> st(fit->begin(), fit->end());
      combo_set->insert(st);
    }
  }

  void AddPrimaryIntProb(double prob) {
    prob_analysis->iprobs_.push_back(prob);
  }

  // Members
  ProbabilityAnalysis* prob_analysis;
};

#endif  // SCRAM_TESTS_PROBABILITY_ANALYSIS_TESTS_H_
