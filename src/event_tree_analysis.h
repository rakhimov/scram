/*
 * Copyright (C) 2017 Olzhas Rakhimov
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
/// @file event_tree_analysis.h
/// Event tree analysis facilities.

#ifndef SCRAM_SRC_EVENT_TREE_ANALYSIS_H_
#define SCRAM_SRC_EVENT_TREE_ANALYSIS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis.h"
#include "event.h"
#include "event_tree.h"
#include "expression.h"
#include "expression/test_event.h"
#include "settings.h"

namespace scram {
namespace core {

/// Event tree analysis functionality.
class EventTreeAnalysis : public Analysis {
 public:
  /// The analysis results binding to the unique analysis target.
  struct Result {
    const mef::Sequence& sequence;  ///< The analysis sequence.
    std::unique_ptr<mef::Gate> gate;  ///< The collected formulas into a gate.
    bool is_expression_only;  ///< Indicator for expression only event trees.
    double p_sequence;  ///< To be assigned by analyses: @todo Remove
  };

  /// @param[in] initiating_event  The unique initiating event.
  /// @param[in] settings  The analysis settings.
  /// @param[in] context  The context to communicate with test-events.
  ///
  /// @pre The initiating event has its event tree.
  EventTreeAnalysis(const mef::InitiatingEvent& initiating_event,
                    const Settings& settings, mef::Context* context);

  /// Analyzes an event tree given the initiating event.
  void Analyze() noexcept;

  /// @returns The initiating event of the event tree analysis.
  const mef::InitiatingEvent& initiating_event() const {
    return initiating_event_;
  }

  /// @returns The results of the event tree analysis.
  /// @{
  const std::vector<Result>& sequences() const { return sequences_; }
  std::vector<Result>& sequences() { return sequences_; }
  /// @}

 private:
  /// Expressions and formulas collected in an event tree path.
  struct PathCollector {
    PathCollector() = default;
    PathCollector(const PathCollector&);  ///< Must deep copy formulas.
    std::vector<mef::Expression*> expressions;  ///< Multiplication arguments.
    std::vector<mef::FormulaPtr> formulas;  ///< AND connective formulas.
    std::unordered_map<std::string, bool> set_instructions;  ///< House events.
  };

  /// Walks the event tree paths and collects sequences.
  struct SequenceCollector {
    const mef::InitiatingEvent& initiating_event;  ///< The analysis initiator.
    mef::Context& context;  ///< The collection context.
    /// Sequences with collected paths.
    std::unordered_map<const mef::Sequence*, std::vector<PathCollector>>
        sequences;
  };

  /// Walks the branch and collects sequences with expressions if any.
  ///
  /// @param[in] initial_state  The branch to start the traversal.
  /// @param[in,out] result  The result container for sequences.
  ///
  /// @post The sequences in the result are joined and unique.
  void CollectSequences(const mef::Branch& initial_state,
                        SequenceCollector* result) noexcept;

  const mef::InitiatingEvent& initiating_event_;  ///< The analysis initiator.
  std::vector<Result> sequences_;  ///< Gathered sequences.
  /// Newly created expressions.
  std::vector<std::unique_ptr<mef::Expression>> expressions_;
  std::vector<std::unique_ptr<mef::Event>> events_;  ///< Newly created events.
  mef::Context* context_;  ///< The communication channel with test-events.
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_EVENT_TREE_ANALYSIS_H_
