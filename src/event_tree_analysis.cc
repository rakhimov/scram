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

#include "expression/numerical.h"
#include "ext/find_iterator.h"

namespace scram {
namespace core {

EventTreeAnalysis::EventTreeAnalysis(
    const mef::InitiatingEvent& initiating_event, const Settings& settings,
    mef::Context* context)
    : Analysis(settings),
      initiating_event_(initiating_event),
      context_(context) {}

namespace {  // The model cloning functions.

/// Clones the formula without any instruction application.
std::unique_ptr<mef::Formula> Clone(const mef::Formula& formula) noexcept {
  auto new_formula = std::make_unique<mef::Formula>(formula.type());
  for (const mef::Formula::EventArg& arg : formula.event_args())
    new_formula->AddArgument(arg);
  for (const mef::FormulaPtr& arg : formula.formula_args())
    new_formula->AddArgument(Clone(*arg));
  return new_formula;
}

/// Clones the formula by applying the set-instructions.
///
/// @param[in] formula  The formula to be deep-cloned.
/// @param[in] set_instructions  The set instructions to change arguments.
/// @param[in] clones  The storage container for newly created clones.
///
/// @returns The deep-copy of the argument formula with new (changed) arguments.
std::unique_ptr<mef::Formula> Clone(
    const mef::Formula& formula,
    const std::unordered_map<std::string, bool>& set_instructions,
    std::vector<std::unique_ptr<mef::Event>>* clones) noexcept {
  auto new_formula = std::make_unique<mef::Formula>(formula.type());
  struct {
    mef::Formula::EventArg operator()(mef::BasicEvent* arg) { return arg; }
    mef::Formula::EventArg operator()(mef::HouseEvent* arg) {
      if (auto it = ext::find(set_house, arg->id())) {
        if (it->second == arg->state())
          return arg;
        auto clone = std::make_unique<mef::HouseEvent>(
            arg->name(), "__clone__." + arg->id(),
            mef::RoleSpecifier::kPrivate);
        clone->state(it->second);
        auto* ptr = clone.get();
        event_register->emplace_back(std::move(clone));
        return ptr;
      }
      return arg;
    }
    mef::Formula::EventArg operator()(mef::Gate* arg) {
      if (set_house.empty())
        return arg;
      auto clone = std::make_unique<mef::Gate>(
          arg->name(), "__clone__." + arg->id(), mef::RoleSpecifier::kPrivate);
      clone->formula(Clone(arg->formula(), set_house, event_register));
      auto* ptr = clone.get();
      event_register->emplace_back(std::move(clone));
      return ptr;
    }

    const std::unordered_map<std::string, bool>& set_house;
    std::vector<std::unique_ptr<mef::Event>>* event_register;
  } cloner{set_instructions, clones};

  for (const mef::Formula::EventArg& arg : formula.event_args())
    new_formula->AddArgument(boost::apply_visitor(cloner, arg));
  for (const mef::FormulaPtr& arg : formula.formula_args())
    new_formula->AddArgument(Clone(*arg, set_instructions, clones));
  return new_formula;
}

}  // namespace

EventTreeAnalysis::PathCollector::PathCollector(const PathCollector& other)
    : expressions(other.expressions),
      set_instructions(other.set_instructions) {
  for (const mef::FormulaPtr& formula : other.formulas)
    formulas.push_back(core::Clone(*formula));
}

void EventTreeAnalysis::Analyze() noexcept {
  assert(initiating_event_.event_tree());
  SequenceCollector collector{initiating_event_, *context_};
  CollectSequences(initiating_event_.event_tree()->initial_state(), &collector);
  for (auto& sequence : collector.sequences) {
    auto gate = std::make_unique<mef::Gate>("__" + sequence.first->name());
    std::vector<mef::FormulaPtr> gate_formulas;
    std::vector<mef::Expression*> arg_expressions;
    for (PathCollector& path_collector : sequence.second) {
      if (path_collector.formulas.size() == 1) {
        gate_formulas.push_back(std::move(path_collector.formulas.front()));
      } else if (path_collector.formulas.size() > 1) {
        auto and_formula = std::make_unique<mef::Formula>(mef::kAnd);
        for (mef::FormulaPtr& arg_formula : path_collector.formulas)
          and_formula->AddArgument(std::move(arg_formula));
        gate_formulas.push_back(std::move(and_formula));
      }
      if (path_collector.expressions.size() == 1) {
        arg_expressions.push_back(path_collector.expressions.front());
      } else if (path_collector.expressions.size() > 1) {
        expressions_.push_back(
            std::make_unique<mef::Mul>(std::move(path_collector.expressions)));
        arg_expressions.push_back(expressions_.back().get());
      }
    }
    assert(gate_formulas.empty() || arg_expressions.empty());
    bool is_expression_only = !arg_expressions.empty();
    if (gate_formulas.size() == 1) {
      gate->formula(std::move(gate_formulas.front()));
    } else if (gate_formulas.size() > 1) {
      auto or_formula = std::make_unique<mef::Formula>(mef::kOr);
      for (mef::FormulaPtr& arg_formula : gate_formulas)
        or_formula->AddArgument(std::move(arg_formula));
      gate->formula(std::move(or_formula));
    } else if (!arg_expressions.empty()) {
      auto event =
          std::make_unique<mef::BasicEvent>("__" + sequence.first->name());
      if (arg_expressions.size() == 1) {
        event->expression(arg_expressions.front());
      } else if (arg_expressions.size() > 1) {
        expressions_.push_back(
            std::make_unique<mef::Add>(std::move(arg_expressions)));
        event->expression(expressions_.back().get());
      }
      gate->formula(std::make_unique<mef::Formula>(mef::kNull));
      gate->formula().AddArgument(event.get());
      events_.push_back(std::move(event));
    } else {
      gate->formula(std::make_unique<mef::Formula>(mef::kNull));
      gate->formula().AddArgument(&mef::HouseEvent::kTrue);
    }
    sequences_.push_back(
        {*sequence.first, std::move(gate), is_expression_only});
  }
}

void EventTreeAnalysis::CollectSequences(const mef::Branch& initial_state,
                                         SequenceCollector* result) noexcept {
  struct Collector {
    class Visitor : public mef::InstructionVisitor {
     public:
      explicit Visitor(Collector* collector) : collector_(*collector) {}

      void Visit(const mef::SetHouseEvent* house_event) override {
        collector_.path_collector_.set_instructions[house_event->name()] =
            house_event->state();
      }

      void Visit(const mef::Link* link) override {
        is_linked_ = true;
        Collector continue_connector(collector_);
        auto save = std::move(collector_.result_->context.functional_events);
        continue_connector(&link->event_tree().initial_state());
        collector_.result_->context.functional_events = std::move(save);
      }

      void Visit(const mef::CollectFormula* collect_formula) override {
        collector_.path_collector_.formulas.push_back(
            core::Clone(collect_formula->formula(),
                        collector_.path_collector_.set_instructions,
                        collector_.clones_));
      }

      void Visit(const mef::CollectExpression* collect_expression) override {
        collector_.path_collector_.expressions.push_back(
            &collect_expression->expression());
      }

      bool is_linked() const { return is_linked_; }

     private:
      Collector& collector_;
      bool is_linked_ = false;  /// Indicate that sequences not be registered.
    };

    void operator()(const mef::Sequence* sequence) {
      Visitor visitor(this);
      for (const mef::Instruction* instruction : sequence->instructions())
        instruction->Accept(&visitor);
      if (!visitor.is_linked())
        result_->sequences[sequence].push_back(std::move(path_collector_));
    }

    void operator()(const mef::Fork* fork) const {
      const std::string& name = fork->functional_event().name();
      assert(result_->context.functional_events.count(name) == false);
      std::string& state = result_->context.functional_events[name];
      assert(state.empty());
      for (const mef::Path& fork_path : fork->paths()) {
        state = fork_path.state();
        Collector(*this)(&fork_path);  // NOLINT(runtime/explicit)
      }
      result_->context.functional_events.erase(name);
    }

    void operator()(const mef::Branch* branch) {
      Visitor visitor(this);
      for (const mef::Instruction* instruction : branch->instructions())
        instruction->Accept(&visitor);

      boost::apply_visitor(*this, branch->target());
    }

    SequenceCollector* result_;
    std::vector<std::unique_ptr<mef::Event>>* clones_;
    PathCollector path_collector_;
  };
  context_->functional_events.clear();
  context_->initiating_event = initiating_event_.name();
  Collector{result, &events_}(&initial_state);  // NOLINT(whitespace/braces)
}

}  // namespace core
}  // namespace scram
