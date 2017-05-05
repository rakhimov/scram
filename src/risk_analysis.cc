/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file risk_analysis.cc
/// Implementation of risk analysis handler.

#include "risk_analysis.h"

#include <boost/range/algorithm/find_if.hpp>

#include "bdd.h"
#include "fault_tree.h"
#include "expression/numerical.h"  ///< @todo Move elsewhere or substitute.
#include "logger.h"
#include "mocus.h"
#include "random.h"
#include "zbdd.h"

namespace scram {
namespace core {

RiskAnalysis::RiskAnalysis(std::shared_ptr<const mef::Model> model,
                           const Settings& settings)
    : Analysis(settings),
      model_(std::move(model)) {}

void RiskAnalysis::Analyze() noexcept {
  assert(results_.empty() && "Rerunning the analysis.");
  // Set the seed for the pseudo-random number generator if given explicitly.
  // Otherwise it defaults to the implementation dependent value.
  if (Analysis::settings().seed() >= 0)
    Random::seed(Analysis::settings().seed());

  for (const mef::InitiatingEventPtr& initiating_event :
       model_->initiating_events()) {
    if (initiating_event->event_tree()) {
      LOG(INFO) << "Running event tree analysis: " << initiating_event->name();
      event_tree_results_.push_back(Analyze(*initiating_event));
      LOG(INFO) << "Finished event tree analysis: " << initiating_event->name();
    }
  }

  for (const mef::FaultTreePtr& ft : model_->fault_trees()) {
    for (const mef::Gate* target : ft->top_events()) {
      LOG(INFO) << "Running analysis: " << target->id();
      RunAnalysis(*target);
      LOG(INFO) << "Finished analysis: " << target->id();
    }
  }
}

void RiskAnalysis::RunAnalysis(const mef::Gate& target) noexcept {
  switch (Analysis::settings().algorithm()) {
    case Algorithm::kBdd:
      RunAnalysis<Bdd>(target);
      break;
    case Algorithm::kZbdd:
      RunAnalysis<Zbdd>(target);
      break;
    case Algorithm::kMocus:
      RunAnalysis<Mocus>(target);
  }
}

template <class Algorithm>
void RiskAnalysis::RunAnalysis(const mef::Gate& target) noexcept {
  auto fta =
      std::make_unique<FaultTreeAnalyzer<Algorithm>>(target,
                                                     Analysis::settings());
  fta->Analyze();
  Result result{target};
  if (Analysis::settings().probability_analysis()) {
    switch (Analysis::settings().approximation()) {
      case Approximation::kNone:
        RunAnalysis<Algorithm, Bdd>(fta.get(), &result);
        break;
      case Approximation::kRareEvent:
        RunAnalysis<Algorithm, RareEventCalculator>(fta.get(), &result);
        break;
      case Approximation::kMcub:
        RunAnalysis<Algorithm, McubCalculator>(fta.get(), &result);
    }
  }
  result.fault_tree_analysis = std::move(fta);
  results_.push_back(std::move(result));
}

template <class Algorithm, class Calculator>
void RiskAnalysis::RunAnalysis(FaultTreeAnalyzer<Algorithm>* fta,
                               Result* result) noexcept {
  auto pa = std::make_unique<ProbabilityAnalyzer<Calculator>>(
      fta, model_->mission_time().get());
  pa->Analyze();
  if (Analysis::settings().importance_analysis()) {
    auto ia = std::make_unique<ImportanceAnalyzer<Calculator>>(pa.get());
    ia->Analyze();
    result->importance_analysis = std::move(ia);
  }
  if (Analysis::settings().uncertainty_analysis()) {
    auto ua = std::make_unique<UncertaintyAnalyzer<Calculator>>(pa.get());
    ua->Analyze();
    result->uncertainty_analysis = std::move(ua);
  }
  result->probability_analysis = std::move(pa);
}

RiskAnalysis::EventTreeResult RiskAnalysis::Analyze(
    const mef::InitiatingEvent& initiating_event) noexcept {
  assert(initiating_event.event_tree());
  SequenceCollector collector{initiating_event};
  CollectSequences(initiating_event.event_tree()->initial_state(), &collector);
  EventTreeResult result{initiating_event};
  for (auto& sequence : collector.sequences) {
    double p_sequence = 0;
    for (PathCollector& path_collector : sequence.second) {
      if (path_collector.expressions.empty())
        continue;
      if (path_collector.expressions.size() == 1) {
        p_sequence += path_collector.expressions.front()->value();
      } else {
        p_sequence += mef::Mul(std::move(path_collector.expressions)).value();
      }
    }
    result.sequences.emplace_back(*sequence.first, p_sequence);
  }
  return result;
}

void RiskAnalysis::CollectSequences(const mef::Branch& initial_state,
                                    SequenceCollector* result) noexcept {
  struct Collector {
    void operator()(const mef::Sequence* sequence) const {
      result_->sequences[sequence].push_back(std::move(path_collector_));
    }

    void operator()(const mef::Branch* branch) {
      for (const mef::InstructionPtr& instruction : branch->instructions()) {
        /// @todo Rework without dynamic casts.
        if (auto* collect_formula =
                dynamic_cast<mef::CollectFormula*>(instruction.get())) {
          path_collector_.formulas.push_back(&collect_formula->formula());
        } else {
          auto& expression =
              static_cast<mef::CollectExpression&>(*instruction).expression();
          path_collector_.expressions.push_back(&expression);
        }
      }
      boost::apply_visitor(*this, branch->target());
    }
    void operator()(const mef::Fork* fork) const {
      for (const mef::Path& fork_path : fork->paths())
        Collector(*this)(&fork_path);  // NOLINT(runtime/explicit)
    }
    SequenceCollector* result_;
    PathCollector path_collector_;
  };
  Collector{result}(&initial_state);  // NOLINT(whitespace/braces)
}

}  // namespace core
}  // namespace scram
