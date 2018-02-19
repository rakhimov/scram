/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// Implementation of risk analysis handler.

#include "risk_analysis.h"

#include "bdd.h"
#include "expression/random_deviate.h"
#include "ext/scope_guard.h"
#include "fault_tree.h"
#include "logger.h"
#include "mocus.h"
#include "zbdd.h"

namespace scram::core {

RiskAnalysis::RiskAnalysis(mef::Model* model, const Settings& settings)
    : Analysis(settings), model_(model) {}

void RiskAnalysis::Analyze() noexcept {
  assert(results_.empty() && "Rerunning the analysis.");
  // Set the seed for the pseudo-random number generator if given explicitly.
  // Otherwise it defaults to the implementation dependent value.
  if (Analysis::settings().seed() >= 0)
    mef::RandomDeviate::seed(Analysis::settings().seed());

  if (model_->alignments().empty()) {
    RunAnalysis();
  } else {
    for (const mef::Alignment& alignment : model_->alignments()) {
      for (const mef::Phase& phase : alignment.phases())
        RunAnalysis(Context{alignment, phase});
    }
  }
}

void RiskAnalysis::RunAnalysis(std::optional<Context> context) noexcept {
  std::vector<std::pair<mef::HouseEvent*, bool>> house_events;
  /// Restores the model after application of the context.
  ext::scope_guard restorator(
      [&house_events, this, init_time = model_->mission_time().value()] {
        model_->mission_time().value(init_time);
        Analysis::settings().mission_time(init_time);

        for (const std::pair<mef::HouseEvent*, bool>& entry : house_events)
          entry.first->state(entry.second);
      });

  if (context) {
    double mission_time =
        context->phase.time_fraction() * model_->mission_time().value();
    model_->mission_time().value(mission_time);
    Analysis::settings().mission_time(mission_time);

    for (const mef::SetHouseEvent* instruction :
         context->phase.instructions()) {
      auto it = model_->table<mef::HouseEvent>().find(instruction->name());
      assert(it != model_->table<mef::HouseEvent>().end() &&
             "Invalid instruction.");
      mef::HouseEvent& house_event = *it;
      if (house_event.state() != instruction->state()) {
        house_events.emplace_back(&house_event, house_event.state());
        house_event.state(instruction->state());
      }
    }
  }

  for (const mef::InitiatingEvent& initiating_event :
       model_->initiating_events()) {
    if (initiating_event.event_tree()) {
      LOG(INFO) << "Running event tree analysis: " << initiating_event.name();
      auto eta = std::make_unique<EventTreeAnalysis>(
          initiating_event, Analysis::settings(), model_->context());
      eta->Analyze();
      for (EventTreeAnalysis::Result& result : eta->sequences()) {
        const mef::Sequence& sequence = result.sequence;
        LOG(INFO) << "Running analysis for sequence: " << sequence.name();
        results_.push_back(
            {{std::pair<const mef::InitiatingEvent&, const mef::Sequence&>{
                  initiating_event, sequence},
              context}});
        RunAnalysis(*result.gate, &results_.back());
        if (result.is_expression_only) {
          results_.back().fault_tree_analysis = nullptr;
          results_.back().importance_analysis = nullptr;
        }
        if (Analysis::settings().probability_analysis())
          result.p_sequence = results_.back().probability_analysis->p_total();
        LOG(INFO) << "Finished analysis for sequence: " << sequence.name();
      }
      event_tree_results_.push_back(
          {initiating_event, context, std::move(eta)});
      LOG(INFO) << "Finished event tree analysis: " << initiating_event.name();
    }
  }

  for (const mef::FaultTree& ft : model_->fault_trees()) {
    for (const mef::Gate* target : ft.top_events()) {
      LOG(INFO) << "Running analysis for gate: " << target->id();
      results_.push_back({{target, context}});
      RunAnalysis(*target, &results_.back());
      LOG(INFO) << "Finished analysis for gate: " << target->id();
    }
  }
}

void RiskAnalysis::RunAnalysis(const mef::Gate& target,
                               Result* result) noexcept {
  switch (Analysis::settings().algorithm()) {
    case Algorithm::kBdd:
      return RunAnalysis<Bdd>(target, result);
    case Algorithm::kZbdd:
      return RunAnalysis<Zbdd>(target, result);
    case Algorithm::kMocus:
      return RunAnalysis<Mocus>(target, result);
  }
}

template <class Algorithm>
void RiskAnalysis::RunAnalysis(const mef::Gate& target,
                               Result* result) noexcept {
  auto fta = std::make_unique<FaultTreeAnalyzer<Algorithm>>(
      target, Analysis::settings(), model_);
  fta->Analyze();
  if (Analysis::settings().probability_analysis()) {
    switch (Analysis::settings().approximation()) {
      case Approximation::kNone:
        RunAnalysis<Algorithm, Bdd>(fta.get(), result);
        break;
      case Approximation::kRareEvent:
        RunAnalysis<Algorithm, RareEventCalculator>(fta.get(), result);
        break;
      case Approximation::kMcub:
        RunAnalysis<Algorithm, McubCalculator>(fta.get(), result);
    }
  }
  result->fault_tree_analysis = std::move(fta);
}

template <class Algorithm, class Calculator>
void RiskAnalysis::RunAnalysis(FaultTreeAnalyzer<Algorithm>* fta,
                               Result* result) noexcept {
  auto pa = std::make_unique<ProbabilityAnalyzer<Calculator>>(
      fta, &model_->mission_time());
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

}  // namespace scram::core
