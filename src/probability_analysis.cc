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
/// Implementations of functions to provide
/// probability and importance informations.

#include "probability_analysis.h"

#include <boost/algorithm/string.hpp>

#include "boolean_graph.h"
#include "error.h"
#include "logger.h"
#include "preprocessor.h"

namespace scram {

ProbabilityAnalysis::ProbabilityAnalysis(const Settings& settings)
    : kSettings_(settings),
      warnings_(""),
      p_total_(0),
      p_rare_(0),
      current_mark_(false),
      coherent_(true),
      p_time_(0),
      imp_time_(0) {}

ProbabilityAnalysis::ProbabilityAnalysis(const GatePtr& root,
                                         const Settings& settings)
    : ProbabilityAnalysis::ProbabilityAnalysis(settings) {
  top_event_ = root;
}

void ProbabilityAnalysis::UpdateDatabase(
    const std::unordered_map<std::string, BasicEventPtr>& basic_events) {
  basic_events_ = basic_events;  /// @todo Remove.
  for (const auto& entry : basic_events) {  /// @todo Remove.
    ordered_basic_events_.push_back(entry.second);
  }
  ProbabilityAnalysis::AssignIndices();
}

void ProbabilityAnalysis::Analyze(
    const std::set< std::set<std::string> >& min_cut_sets) noexcept {
  min_cut_sets_ = min_cut_sets;

  // Special case of unity with empty sets.
  if (min_cut_sets_.size() == 1 && min_cut_sets_.begin()->empty()) {
    warnings_ += " Probability is for UNITY case.";
    p_total_ = 1;
    p_rare_ = 1;
    prob_of_min_sets_.emplace(*min_cut_sets_.begin(), 1);
    return;
  }

  ProbabilityAnalysis::IndexMcs(min_cut_sets_);

  using boost::container::flat_set;
  // Minimal cut sets with higher than cut-off probability.
  std::set<flat_set<int>> mcs_for_prob;
  int i = 0;  // Indices for minimal cut sets in the vector.
  for (const flat_set<int>& cut_set : imcs_) {  /// @todo Remove.
    // Calculate a probability of a set with AND relationship.
    double p_sub_set = ProbabilityAnalysis::ProbAnd(cut_set);
    // Choose cut sets with high enough probabilities.
    if (p_sub_set > kSettings_.cut_off()) {
      flat_set<int> mcs(cut_set);
      mcs_for_prob.insert(mcs_for_prob.end(), mcs);
    }

    // Update a container with minimal cut sets and probabilities.
    prob_of_min_sets_.emplace(imcs_to_smcs_[i], p_sub_set);
    p_rare_ += p_sub_set;
    ++i;
  }

  CLOCK(p_time);
  LOG(DEBUG3) << "Calculating probabilities...";
  LOG(DEBUG3) << "Cut sets above cut-off level: " << mcs_for_prob.size();
  // Get the total probability.
  if (kSettings_.approx() == "mcub") {
    if (!coherent_) {
      warnings_ += " The cut sets are not coherent and contain negation."
                   " The MCUB approximation may not hold.";
    }
    p_total_ = ProbabilityAnalysis::ProbMcub(imcs_);

  } else if (kSettings_.approx() == "rare-event") {
    std::map<std::set<std::string>, double>::iterator it_pr;
    for (it_pr = prob_of_min_sets_.begin(); it_pr != prob_of_min_sets_.end();
         ++it_pr) {
      // Check if a probability of a set does not exceed 0.1,
      // which is required for the rare event approximation to hold.
      if (it_pr->second > 0.1) {
        warnings_ += " The rare event approximation may be inaccurate for"
            " this analysis because one of minimal cut sets'"
            " probability exceeds the 0.1 threshold requirement.";
        break;
      }
    }
    p_total_ = ProbabilityAnalysis::ProbRareEvent(imcs_);
  } else {
    assert(top_event_);
    p_total_ = ProbabilityAnalysis::CalculateTotalProbability();
  }
  LOG(DEBUG3) << "Finished probability calculations in " << DUR(p_time);
  p_time_ = DUR(p_time);
  if (kSettings_.importance_analysis()) {
    CLOCK(imp_time);
    LOG(DEBUG3) << "Calculating importance factors...";
    ProbabilityAnalysis::PerformImportanceAnalysis();
    LOG(DEBUG3) << "Calculated importance factors in " << DUR(imp_time);
    imp_time_ = DUR(imp_time);
  }
}

void ProbabilityAnalysis::AssignIndices() noexcept {
  if (top_event_) {
    CLOCK(ft_creation);
    BooleanGraph* graph =
        new BooleanGraph(top_event_, kSettings_.ccf_analysis());
    LOG(DEBUG2) << "Boolean graph is created in " << DUR(ft_creation);

    CLOCK(prep_time);  // Overall preprocessing time.
    LOG(DEBUG2) << "Preprocessing...";
    Preprocessor* preprocessor = new PreprocessorBdd(graph);
    preprocessor->Run();
    delete preprocessor;  // No exceptions are expected.
    LOG(DEBUG2) << "Finished preprocessing in " << DUR(prep_time);

    CLOCK(bdd_time);  // BDD based calculation time.
    LOG(DEBUG2) << "Creating BDD for ProbabilityAnalysis...";
    bdd_graph_ = std::unique_ptr<Bdd>(new Bdd(graph));
    ordered_basic_events_ = graph->basic_events();
    delete graph;  /// @todo This is dangerous. BDD has invalid graph pointer.
    LOG(DEBUG2) << "BDD is created in " << DUR(bdd_time);
  }
  // Cleanup the previous information.
  index_to_basic_.clear();
  id_to_index_.clear();
  var_probs_.clear();
  // Dummy basic event at index 0.
  index_to_basic_.push_back(std::make_shared<BasicEvent>("dummy"));
  var_probs_.push_back(-1);
  // Indexation of events.
  int j = 1;
  for (const BasicEventPtr& event : ordered_basic_events_) {
    index_to_basic_.push_back(event);
    id_to_index_.emplace(event->id(), j);
    var_probs_.push_back(event->p());
    ++j;
  }
}

void ProbabilityAnalysis::IndexMcs(
    const std::set<std::set<std::string> >& min_cut_sets) noexcept {
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
        assert(id_to_index_.count(comp_name));

        if (coherent_) coherent_ = false;  // Detected non-coherence.

        mcs_with_indices.insert(mcs_with_indices.begin(),
                                -id_to_index_.find(comp_name)->second);
        mcs_basic_events_.insert(id_to_index_.find(comp_name)->second);

      } else {
        assert(id_to_index_.count(*it_set));
        mcs_with_indices.insert(mcs_with_indices.end(),
                                id_to_index_.find(*it_set)->second);
        mcs_basic_events_.insert(id_to_index_.find(*it_set)->second);
      }
    }
    imcs_.push_back(mcs_with_indices);
    imcs_to_smcs_.push_back(*it);
  }
}

double ProbabilityAnalysis::ProbMcub(
    const std::vector<FlatSet>& min_cut_sets) noexcept {
  double m = 1;
  for (const auto& cut_set : min_cut_sets) {
    m *= 1 - ProbabilityAnalysis::ProbAnd(cut_set);
  }
  return 1 - m;
}

double ProbabilityAnalysis::ProbRareEvent(
    const std::vector<FlatSet>& min_cut_sets) noexcept {
  double sum = 0;
  for (const auto& cut_set : min_cut_sets) {
    sum += ProbabilityAnalysis::ProbAnd(cut_set);
  }
  return sum;
}

double ProbabilityAnalysis::ProbAnd(const FlatSet& cut_set) noexcept {
  if (cut_set.empty()) return 0;
  double p_sub_set = 1;  // 1 is for multiplication.
  for (int member : cut_set) {
    if (member > 0) {
      p_sub_set *= var_probs_[member];
    } else {
      p_sub_set *= 1 - var_probs_[std::abs(member)];  // Never zero.
    }
  }
  return p_sub_set;
}

double ProbabilityAnalysis::CalculateTotalProbability() noexcept {
  assert(top_event_);
  CLOCK(calc_time);  // BDD based calculation time.
  LOG(DEBUG2) << "Calculating probability with BDD...";
  current_mark_ = !current_mark_;
  double prob = ProbabilityAnalysis::CalculateProbability(bdd_graph_->root(),
                                                          current_mark_);
  if (bdd_graph_->complement_root()) prob = 1 - prob;
  LOG(DEBUG2) << "Calculated probability " << prob << " in " << DUR(calc_time);
  return prob;
}

double ProbabilityAnalysis::CalculateProbability(const VertexPtr& vertex,
                                                 bool mark) noexcept {
  if (vertex->terminal()) return 1;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark() == mark) return ite->prob();
  ite->mark(mark);
  double var_prob = 0;
  if (ite->module()) {
    const Bdd::Function& res = bdd_graph_->gates().find(ite->index())->second;
    var_prob = ProbabilityAnalysis::CalculateProbability(res.vertex, mark);
    if (res.complement) var_prob = 1 - var_prob;
  } else {
    var_prob = var_probs_[ite->index()];
  }
  double high = ProbabilityAnalysis::CalculateProbability(ite->high(), mark);
  double low = ProbabilityAnalysis::CalculateProbability(ite->low(), mark);
  if (ite->complement_edge()) low = 1 - low;
  ite->prob(var_prob * high + (1 - var_prob) * low);
  return ite->prob();
}

void ProbabilityAnalysis::PerformImportanceAnalysis() noexcept {
  // The main data for all the importance types is P(top/event) or
  // P(top/Not event).
  std::set<int>::iterator it;
  for (it = mcs_basic_events_.begin(); it != mcs_basic_events_.end(); ++it) {
    // Calculate P(top/event)
    var_probs_[*it] = 1;
    double p_e = 0;
    if (kSettings_.approx() == "mcub") {
      p_e = ProbabilityAnalysis::ProbMcub(imcs_);
    } else if (kSettings_.approx() == "rare-event") {
      p_e = ProbabilityAnalysis::ProbRareEvent(imcs_);
    } else {
      p_e = ProbabilityAnalysis::CalculateTotalProbability();
    }

    // Calculate P(top/Not event)
    var_probs_[*it] = 0;
    double p_not_e = 0;
    if (kSettings_.approx() == "mcub") {
      p_not_e = ProbabilityAnalysis::ProbMcub(imcs_);
    } else if (kSettings_.approx() == "rare-event") {
      p_not_e = ProbabilityAnalysis::ProbRareEvent(imcs_);
    } else {
      p_not_e = ProbabilityAnalysis::CalculateTotalProbability();
    }
    // Restore the probability.
    var_probs_[*it] = index_to_basic_[*it]->p();

    ImportanceFactors imp;
    imp.dif = 1 - p_not_e / p_total_;  // Diagnosis importance factor.
    imp.mif = p_e - p_not_e;  // Birnbaum Marginal importance factor.
    imp.cif = imp.mif * var_probs_[*it] / p_not_e;  // Critical factor.
    imp.rrw = p_total_ / p_not_e;  // Risk Reduction Worth.
    imp.raw = p_e / p_total_;  // Risk Achievement Worth.

    importance_.emplace(index_to_basic_[*it]->id(), std::move(imp));
  }
}

}  // namespace scram
