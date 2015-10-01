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
      warnings_(""),
      p_total_(0),
      current_mark_(false),
      p_time_(0),
      imp_time_(0) {}

ProbabilityAnalysis::ProbabilityAnalysis(const FaultTreeAnalysis* fta)
    : Analysis::Analysis(fta->settings()),
      top_event_(fta->top_event()),
      warnings_(""),
      p_total_(0),
      current_mark_(false),
      p_time_(0),
      imp_time_(0) {}

void ProbabilityAnalysis::Analyze(
    const std::set<std::set<std::string>>& min_cut_sets) noexcept {
  assert(top_event_ && "The fault tree is undefined.");
  assert(!bool_graph_ && "Re-running analysis.");
  // Special case of unity with empty sets.
  if (min_cut_sets.size() == 1 && min_cut_sets.begin()->empty()) {
    warnings_ += " Probability is for UNITY case.";
    p_total_ = 1;
    return;
  }
  ProbabilityAnalysis::AssignIndices();
  ProbabilityAnalysis::IndexMcs(min_cut_sets);

  CLOCK(p_time);
  LOG(DEBUG3) << "Calculating probabilities...";
  // Get the total probability.
  if (kSettings_.approximation() == "mcub") {
    /// @todo Detect the conditions.
    warnings_ += " The MCUB approximation may not hold"
                 " if the fault tree is not coherent"
                 " or there are many common events.";
    p_total_ = ProbabilityAnalysis::ProbMcub(imcs_);

  } else if (kSettings_.approximation() == "rare-event") {
    /// @todo Check if a probability of any cut set exceeds 0.1.
    warnings_ += " The rare event approximation may be inaccurate for analysis"
                 " if minimal cut sets' probabilities exceed 0.1.";
    p_total_ = ProbabilityAnalysis::ProbRareEvent(imcs_);
  } else {
    p_total_ = ProbabilityAnalysis::CalculateTotalProbability();
  }

  assert(p_total_ >= 0 && "The total probability is negative.");
  if (p_total_ > 1) {
    warnings_ += " Probability value exceeded 1 and was adjusted to 1.";
    p_total_ = 1;
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

void ProbabilityAnalysis::PerformImportanceAnalysis() noexcept {
  if (kSettings_.approximation() == "no") {
    ProbabilityAnalysis::PerformImportanceAnalysisBdd();
    return;
  }
  // The main data for all the importance types is P(top/event) or
  // P(top/Not event).
  for (int index : mcs_basic_events_) {
    // Calculate P(top/event)
    var_probs_[index] = 1;
    double p_e = 0;
    if (kSettings_.approximation() == "mcub") {
      p_e = ProbabilityAnalysis::ProbMcub(imcs_);
    } else {
      assert(kSettings_.approximation() == "rare-event");
      p_e = ProbabilityAnalysis::ProbRareEvent(imcs_);
    }
    assert(p_e >= 0);
    if (p_e > 1) p_e = 1;

    // Calculate P(top/Not event)
    var_probs_[index] = 0;
    double p_not_e = 0;
    if (kSettings_.approximation() == "mcub") {
      p_not_e = ProbabilityAnalysis::ProbMcub(imcs_);
    } else {
      assert(kSettings_.approximation() == "rare-event");
      p_not_e = ProbabilityAnalysis::ProbRareEvent(imcs_);
    }
    assert(p_not_e >= 0);
    if (p_not_e > 1) p_not_e = 1;
    // Restore the probability.
    var_probs_[index] = index_to_basic_[index]->p();

    ImportanceFactors imp;
    double p_var = var_probs_[index];
    imp.mif = p_e - p_not_e;  // Birnbaum Marginal importance factor.
    imp.cif = p_var * imp.mif / p_total_;  // Critical factor.
    imp.raw = 1 + (1 - p_var) * imp.mif / p_total_;  // Risk Achievement Worth.
    imp.dif = p_var * imp.raw;  // Diagnosis importance factor.
    imp.rrw = p_total_ / (p_total_ - p_var * imp.mif);  // Risk Reduction Worth.

    importance_.emplace(index_to_basic_[index]->id(), std::move(imp));
  }
}

void ProbabilityAnalysis::PerformImportanceAnalysisBdd() noexcept {
  for (int index : mcs_basic_events_) {
    int order = bdd_graph_->index_to_order().find(index)->second;
    ImportanceFactors imp;
    current_mark_ = !current_mark_;
    imp.mif = ProbabilityAnalysis::CalculateMif(bdd_graph_->root().vertex,
                                                order, current_mark_);
    if (bdd_graph_->root().complement) imp.mif = -imp.mif;

    double p_var = var_probs_[index];
    imp.cif = p_var * imp.mif / p_total_;
    imp.raw = 1 + (1 - p_var) * imp.mif / p_total_;
    imp.dif = p_var * imp.raw;
    imp.rrw = p_total_ / (p_total_ - p_var * imp.mif);
    importance_.emplace(index_to_basic_[index]->id(), std::move(imp));

    current_mark_ = !current_mark_;
    ProbabilityAnalysis::ClearMarks(bdd_graph_->root().vertex, current_mark_);
  }
}

double ProbabilityAnalysis::CalculateMif(const VertexPtr& vertex, int order,
                                         bool mark) noexcept {
  if (vertex->terminal()) return 0;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark() == mark) return ite->factor();
  ite->mark(mark);
  if (ite->order() > order) {
    if (!ite->module()) {
      ite->factor(0);
    } else {  /// @todo Detect if the variable is in the module.
      // The assumption is
      // that the order of a module is always larger
      // than the order of its variables.
      double high = ProbabilityAnalysis::RetrieveProbability(ite->high());
      double low = ProbabilityAnalysis::RetrieveProbability(ite->low());
      if (ite->complement_edge()) low = 1 - low;
      const Bdd::Function& res = bdd_graph_->gates().find(ite->index())->second;
      double mif = ProbabilityAnalysis::CalculateMif(res.vertex, order, mark);
      if (res.complement) mif = -mif;
      ite->factor((high - low) * mif);
    }
  } else if (ite->order() == order) {
    assert(!ite->module() && "A variable can't be a module.");
    double high = ProbabilityAnalysis::RetrieveProbability(ite->high());
    double low = ProbabilityAnalysis::RetrieveProbability(ite->low());
    if (ite->complement_edge()) low = 1 - low;
    ite->factor(high - low);
  } else  {
    assert(ite->order() < order);
    double var_prob = 0;
    if (ite->module()) {
      const Bdd::Function& res = bdd_graph_->gates().find(ite->index())->second;
      var_prob = ProbabilityAnalysis::RetrieveProbability(res.vertex);
      if (res.complement) var_prob = 1 - var_prob;
    } else {
      var_prob = var_probs_[ite->index()];
    }
    double high = ProbabilityAnalysis::CalculateMif(ite->high(), order, mark);
    double low = ProbabilityAnalysis::CalculateMif(ite->low(), order, mark);
    if (ite->complement_edge()) low = -low;
    ite->factor(var_prob * high + (1 - var_prob) * low);
  }
  return ite->factor();
}

double ProbabilityAnalysis::RetrieveProbability(
    const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return 1;
  return Ite::Ptr(vertex)->prob();
}

void ProbabilityAnalysis::ClearMarks(const VertexPtr& vertex,
                                     bool mark) noexcept {
  if (vertex->terminal()) return;
  ItePtr ite = Ite::Ptr(vertex);
  if (ite->mark() == mark) return;
  ite->mark(mark);
  if (ite->module()) {
    const Bdd::Function& res = bdd_graph_->gates().find(ite->index())->second;
    ProbabilityAnalysis::ClearMarks(res.vertex, mark);
  }
  ProbabilityAnalysis::ClearMarks(ite->high(), mark);
  ProbabilityAnalysis::ClearMarks(ite->low(), mark);
}

}  // namespace scram
