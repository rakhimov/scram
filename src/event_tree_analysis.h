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

#include <unordered_map>
#include <vector>

#include "analysis.h"
#include "event_tree.h"
#include "settings.h"

namespace scram {
namespace core {

/// Event tree analysis functionality.
class EventTreeAnalysis : public Analysis {
 public:
  /// The analysis results binding to the unique analysis target.
  struct Result {
    const mef::Sequence& sequence;  ///< The analysis sequence.
    double p_sequence;  ///< Sequence probability.
  };

  /// @param[in] initiating_event  The unique initiating event.
  /// @param[in] settings  The analysis settings.
  ///
  /// @pre The initiating event has its event tree.
  EventTreeAnalysis(const mef::InitiatingEvent& initiating_event,
                    const Settings& settings);

  /// Analyzes an event tree given the initiating event.
  void Analyze() noexcept;

  /// @returns The initiating event of the event tree analysis.
  const mef::InitiatingEvent& initiating_event() const {
    return initiating_event_;
  }

  /// @returns The results of the event tree analysis.
  const std::vector<Result>& results() const { return results_; }

 private:
  /// Expressions and formulas collected in an event tree path.
  struct PathCollector {
    std::vector<mef::Expression*> expressions;  ///< Multiplication arguments.
    std::vector<mef::Formula*> formulas;  ///< AND connective formulas.
  };

  /// Walks the event tree paths and collects sequences.
  struct SequenceCollector {
    const mef::InitiatingEvent& initiating_event;  ///< The analysis initiator.
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
  std::vector<Result> results_;  ///< Analyzed sequences.
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_EVENT_TREE_ANALYSIS_H_
