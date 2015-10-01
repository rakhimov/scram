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

#include "error.h"
#include "logger.h"
#include "preprocessor.h"

namespace scram {

ProbabilityAnalysis::ProbabilityAnalysis(const GatePtr& root,
                                         const Settings& settings)
    : Analysis::Analysis(settings),
      top_event_(root),
      p_total_(0),
      current_mark_(false) {}

ProbabilityAnalysis::ProbabilityAnalysis(const FaultTreeAnalysis* fta)
    : Analysis::Analysis(fta->settings()),
      top_event_(fta->top_event()),
      p_total_(0),
      current_mark_(false) {}

void ProbabilityAnalysis::AssignIndices() noexcept {
  CLOCK(ft_creation);
  bool_graph_ = std::unique_ptr<BooleanGraph>(
      new BooleanGraph(top_event_, kSettings_.ccf_analysis()));
  LOG(DEBUG2) << "Boolean graph is created in " << DUR(ft_creation);

  if (kSettings_.approximation() == "no") {
    CLOCK(prep_time);  // Overall preprocessing time.
    LOG(DEBUG2) << "Preprocessing...";
    Preprocessor* preprocessor = new CustomPreprocessor<Bdd>(bool_graph_.get());
    preprocessor->Run();
    delete preprocessor;  // No exceptions are expected.
    LOG(DEBUG2) << "Finished preprocessing in " << DUR(prep_time);

    CLOCK(bdd_time);  // BDD based calculation time.
    LOG(DEBUG2) << "Creating BDD for ProbabilityAnalysis...";
    bdd_graph_ = std::unique_ptr<Bdd>(new Bdd(bool_graph_.get()));
    LOG(DEBUG2) << "BDD is created in " << DUR(bdd_time);
  }

  // Dummy basic event at index 0.
  index_to_basic_.push_back(std::make_shared<BasicEvent>("dummy"));
  var_probs_.push_back(-1);
  // Indexation of events.
  int j = 1;
  for (const BasicEventPtr& event : bool_graph_->basic_events()) {
    basic_events_.emplace(event->id(), event);  /// @todo Remove.
    index_to_basic_.push_back(event);
    id_to_index_.emplace(event->id(), j);
    var_probs_.push_back(event->p());
    ++j;
  }
}

void ProbabilityAnalysis::IndexMcs(
    const std::set<std::set<std::string>>& min_cut_sets) noexcept {
  // Update databases of minimal cut sets with indexed events.
  for (const auto& cut_set : min_cut_sets) {
    CutSet mcs_with_indices;  // Minimal cut set with indices.
    for (const auto& id : cut_set) {
      std::vector<std::string> names;
      boost::split(names, id, boost::is_any_of(" "), boost::token_compress_on);
      assert(names.size() >= 1);
      if (names[0] == "not") {
        std::string comp_name = id;
        boost::replace_first(comp_name, "not ", "");
        // This must be a complement of an event.
        assert(id_to_index_.count(comp_name));
        mcs_with_indices.push_back(-id_to_index_.find(comp_name)->second);
        mcs_basic_events_.insert(id_to_index_.find(comp_name)->second);

      } else {
        assert(id_to_index_.count(id));
        mcs_with_indices.push_back(id_to_index_.find(id)->second);
        mcs_basic_events_.insert(id_to_index_.find(id)->second);
      }
    }
    imcs_.push_back(mcs_with_indices);
  }
}

double ProbabilityAnalysis::ProbMcub(
    const std::vector<CutSet>& min_cut_sets) noexcept {
  double m = 1;
  for (const auto& cut_set : min_cut_sets) {
    m *= 1 - ProbabilityAnalysis::ProbAnd(cut_set);
  }
  return 1 - m;
}

double ProbabilityAnalysis::ProbRareEvent(
    const std::vector<CutSet>& min_cut_sets) noexcept {
  double sum = 0;
  for (const auto& cut_set : min_cut_sets) {
    sum += ProbabilityAnalysis::ProbAnd(cut_set);
  }
  return sum;
}

double ProbabilityAnalysis::ProbAnd(const CutSet& cut_set) noexcept {
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
  double prob = ProbabilityAnalysis::CalculateProbability(
      bdd_graph_->root().vertex, current_mark_);
  if (bdd_graph_->root().complement) prob = 1 - prob;
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

}  // namespace scram
