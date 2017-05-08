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
/// @file event_tree_analysis.cc
/// Implementation of event tree analysis facilities.

#include "event_tree_analysis.h"

#include "expression/numerical.h"  ///< @todo Move elsewhere or substitute.

namespace scram {
namespace core {

EventTreeAnalysis::EventTreeAnalysis(
    const mef::InitiatingEvent& initiating_event, const Settings& settings)
    : Analysis(settings), initiating_event_(initiating_event) {}

void EventTreeAnalysis::Analyze() noexcept {
  assert(initiating_event_.event_tree());
  SequenceCollector collector{initiating_event_};
  CollectSequences(initiating_event_.event_tree()->initial_state(), &collector);
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
    results_.push_back({*sequence.first, p_sequence});
  }
}

void EventTreeAnalysis::CollectSequences(const mef::Branch& initial_state,
                                         SequenceCollector* result) noexcept {
  struct Collector {
    void operator()(const mef::Sequence* sequence) const {
      result_->sequences[sequence].push_back(std::move(path_collector_));
    }

    void operator()(const mef::Fork* fork) const {
      for (const mef::Path& fork_path : fork->paths())
        Collector(*this)(&fork_path);  // NOLINT(runtime/explicit)
    }

    void operator()(const mef::Branch* branch) {
      class Visitor : public mef::InstructionVisitor {
       public:
        explicit Visitor(Collector* collector) : collector_(*collector) {}

        void Visit(const mef::CollectFormula* collect_formula) override {
          collector_.path_collector_.formulas.push_back(
              &collect_formula->formula());
        }

        void Visit(const mef::CollectExpression* collect_expression) override {
          collector_.path_collector_.expressions.push_back(
              &collect_expression->expression());
        }

       private:
        Collector& collector_;
      } visitor(this);

      for (const mef::Instruction* instruction : branch->instructions())
        instruction->Accept(&visitor);

      boost::apply_visitor(*this, branch->target());
    }

    SequenceCollector* result_;
    PathCollector path_collector_;
  };
  Collector{result}(&initial_state);  // NOLINT(whitespace/braces)
}

}  // namespace core
}  // namespace scram
