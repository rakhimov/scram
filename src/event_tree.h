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

/// @file event_tree.h
/// Event Tree facilities.

#ifndef SCRAM_SRC_EVENT_TREE_H_
#define SCRAM_SRC_EVENT_TREE_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/variant.hpp>

#include "element.h"
#include "event.h"
#include "expression.h"
#include "ext/variant.h"

namespace scram {
namespace mef {

class InstructionVisitor;

/// Instructions and rules for event tree paths.
class Instruction : private boost::noncopyable {
 public:
  virtual ~Instruction() = default;

  /// Applies the visitor to the object.
  virtual void Accept(InstructionVisitor* visitor) const = 0;
};

/// Default visit for instruction type of T.
template <class T>
class Visitable : public Instruction {
 public:
  /// Calls visit with the object pointer T*.
  void Accept(InstructionVisitor* visitor) const final;
};

/// The operation to change house-events.
class SetHouseEvent : public Visitable<SetHouseEvent> {
 public:
  /// @param[in] name  Non-empty public house-event name.
  /// @param[in] state  The new state for the given house-event.
  SetHouseEvent(std::string name, bool state)
      : name_(std::move(name)), state_(state) {}

  /// @returns The name of the house-event to apply this instruction.
  const std::string& name() const { return name_; }

  /// @returns The state of the target house-event to be changed into.
  bool state() const { return state_; }

 private:
  std::string name_;  ///< The public name of the house event.
  bool state_;  ///< The state for the house event.
};

/// The operation of collecting expressions for event tree sequences.
class CollectExpression : public Visitable<CollectExpression> {
 public:
  /// @param[in] expression  The expression to multiply
  ///                        the current sequence probability.
  explicit CollectExpression(Expression* expression)
      : expression_(expression) {}

  /// @returns The collected expression for value extraction.
  Expression& expression() const { return *expression_; }

 private:
  Expression* expression_;  ///< The probability expression to multiply.
};

/// The operation of connecting fault tree events into the event tree.
class CollectFormula : public Visitable<CollectFormula> {
 public:
  /// @param[in] formula  The valid formula to add into the sequence fault tree.
  explicit CollectFormula(FormulaPtr formula) : formula_(std::move(formula)) {}

  /// @returns The formula to include into the current product of the path.
  Formula& formula() const { return *formula_; }

 private:
  FormulaPtr formula_;  ///< The valid single formula for the collection.
};

/// Conditional application of instructions.
class IfThenElse : public Visitable<IfThenElse> {
 public:
  /// @param[in] expression  The expression to evaluate for truth.
  /// @param[in] then_instruction  The required instruction to execute.
  /// @param[in] else_instruction  An optional instruction for the false case.
  IfThenElse(Expression* expression, Instruction* then_instruction,
             Instruction* else_instruction = nullptr)
      : expression_(expression),
        then_instruction_(std::move(then_instruction)),
        else_instruction_(std::move(else_instruction)) {}

  /// @returns The conditional expression of the ternary instruction.
  Expression* expression() const { return expression_; }

  /// @returns The instruction to execute if the expression is true.
  Instruction* then_instruction() const { return then_instruction_; }

  /// @returns The instruction to execute if the expression is false.
  ///          nullptr if the else instruction is optional and not set.
  Instruction* else_instruction() const { return else_instruction_; }

 private:
  Expression* expression_;         ///< The condition source.
  Instruction* then_instruction_;  ///< The mandatory 'truth' instruction.
  Instruction* else_instruction_;  ///< The optional 'false' instruction.
};

/// Compound instructions.
class Block : public Visitable<Block> {
 public:
  /// @param[in] instructions  Instructions to be applied in this block.
  explicit Block(std::vector<Instruction*> instructions)
      : instructions_(std::move(instructions)) {}

  /// @returns The instructions to be applied in the block.
  const std::vector<Instruction*>& instructions() const {
    return instructions_;
  }

 private:
  std::vector<Instruction*> instructions_;  ///< Zero or more instructions.
};

/// A reusable collection of instructions.
class Rule : public Element,
             public Visitable<Rule>,
             public NodeMark,
             public Usage {
 public:
  using Element::Element;

  /// @param[in] instructions  One or more instructions for the sequence.
  void instructions(std::vector<Instruction*> instructions) {
    assert(!instructions.empty());
    instructions_ = std::move(instructions);
  }

  /// @returns The instructions to be applied in the rule.
  const std::vector<Instruction*>& instructions() const {
    return instructions_;
  }

 private:
  std::vector<Instruction*> instructions_;  ///< Instructions to execute.
};

using RulePtr = std::unique_ptr<Rule>;  ///< Unique rules in a model.

class EventTree;  // The target of the Link.

/// A link to another event tree in end-states only.
class Link : public Visitable<Link>, public NodeMark {
 public:
  /// @param[in] event_tree  The event tree to be linked in the end-sequence.
  explicit Link(const EventTree& event_tree) : event_tree_(event_tree) {}

  /// @returns The referenced event tree in the link.
  const EventTree& event_tree() const { return event_tree_; }

 private:
  const EventTree& event_tree_;  ///< The referenced event tree.
};

/// The base abstract class for instruction visitors.
class InstructionVisitor {
 public:
  virtual ~InstructionVisitor() = default;

  /// A set of required visitation functions for concrete visitors to implement.
  /// @{
  virtual void Visit(const SetHouseEvent*) = 0;
  virtual void Visit(const CollectExpression*) = 0;
  virtual void Visit(const CollectFormula*) = 0;
  virtual void Visit(const Link*) = 0;
  virtual void Visit(const IfThenElse* ite) {
    if (ite->expression()->value()) {
      ite->then_instruction()->Accept(this);
    } else if (ite->else_instruction()) {
      ite->else_instruction()->Accept(this);
    }
  }
  virtual void Visit(const Block* block) {
    for (const Instruction* instruction : block->instructions())
      instruction->Accept(this);
  }
  virtual void Visit(const Rule* rule) {
    for (const Instruction* instruction : rule->instructions())
      instruction->Accept(this);
  }
  /// @}
};

/// Visits only instructions and ignores non-instructions.
class NullVisitor : public InstructionVisitor {
 public:
  void Visit(const SetHouseEvent*) override {}
  void Visit(const CollectExpression*) override {}
  void Visit(const CollectFormula*) override {}
  void Visit(const Link*) override {}
  void Visit(const IfThenElse* ite) override {
    ite->then_instruction()->Accept(this);
    if (ite->else_instruction())
      ite->else_instruction()->Accept(this);
  }
};

template <class T>
void Visitable<T>::Accept(InstructionVisitor* visitor) const {
  visitor->Visit(static_cast<const T*>(this));
}

/// Representation of sequences in event trees.
class Sequence : public Element, public Usage {
 public:
  using Element::Element;

  /// @param[in] instructions  Zero or more instructions for the sequence.
  void instructions(std::vector<Instruction*> instructions) {
    instructions_ = std::move(instructions);
  }

  /// @returns The instructions to be applied at this sequence.
  const std::vector<Instruction*>& instructions() const {
    return instructions_;
  }

 private:
  /// Instructions to execute with the sequence.
  std::vector<Instruction*> instructions_;
};

/// Sequences are defined in event trees but referenced in other constructs.
using SequencePtr = std::unique_ptr<Sequence>;

class EventTree;  // Manages the order assignment to functional events.

/// Representation of functional events in event trees.
class FunctionalEvent : public Element, public Usage {
  friend class EventTree;

 public:
  using Element::Element;

  /// @returns The order of the functional event in the event tree.
  /// @returns 0 if no order has been assigned.
  int order() const { return order_; }

 private:
  /// Sets the functional event order.
  void order(int order) { order_ = order; }

  int order_ = 0;  ///< The order of the functional event.
};

/// Functional events are defined in and unique to event trees.
using FunctionalEventPtr = std::unique_ptr<FunctionalEvent>;

class Fork;
class NamedBranch;

/// The branch representation in event trees.
class Branch {
 public:
  /// The types of possible branch end-points.
  using Target = boost::variant<Sequence*, Fork*, NamedBranch*>;

  /// Sets the instructions to execute at the branch.
  void instructions(std::vector<Instruction*> instructions) {
    instructions_ = std::move(instructions);
  }

  /// @returns The instructions to execute at the branch.
  const std::vector<Instruction*>& instructions() const {
    return instructions_;
  }

  /// Sets the target for the branch.
  void target(Target target) { target_ = std::move(target); }

  /// @returns The target semantics or end-points of the branch.
  ///
  /// @pre The target has been set.
  const Target& target() const {
    assert(ext::as<bool>(target_));
    return target_;
  }

 private:
  std::vector<Instruction*> instructions_;  ///< Zero or more instructions.
  Target target_;  ///< The target semantics of the branch.
};

/// Named branches that can be referenced and reused.
class NamedBranch : public Element,
                    public Branch,
                    public NodeMark,
                    public Usage {
 public:
  using Element::Element;
};

using NamedBranchPtr = std::unique_ptr<NamedBranch>;  ///< Unique in event tree.

/// Functional-event state paths in event trees.
class Path : public Branch {
 public:
  /// @param[in] state  State identifier string for functional event.
  ///
  /// @throws LogicError  The string is empty or malformed.
  explicit Path(std::string state);

  /// @returns The state of a functional event.
  const std::string& state() const { return state_; }

 private:
  std::string state_;  ///< The state identifier.
};

/// Functional event forks.
class Fork {
 public:
  /// @param[in] functional_event  The source functional event.
  /// @param[in] paths  The fork paths with functional event states.
  ///
  /// @throws ValidationError  The path states are duplicated.
  Fork(const FunctionalEvent& functional_event, std::vector<Path> paths);

  /// @returns The functional event of the fork.
  const FunctionalEvent& functional_event() const { return functional_event_; }

  /// @returns The fork paths with functional event states.
  /// @{
  const std::vector<Path>& paths() const { return paths_; }
  std::vector<Path>& paths() { return paths_; }
  /// @}

 private:
  const FunctionalEvent& functional_event_;  ///< The fork source.
  std::vector<Path> paths_;  ///< The non-empty collection of fork paths.
};

/// Event Tree representation with MEF constructs.
class EventTree : public Element, public Usage, private boost::noncopyable {
 public:
  using Element::Element;

  /// @returns The initial state branch of the event tree.
  const Branch& initial_state() const { return initial_state_; }

  /// Sets the initial state of the event tree.
  void initial_state(Branch branch) { initial_state_ = std::move(branch); }

  /// @returns The container of event tree constructs of specific kind
  ///          with construct original names as keys.
  /// @{
  const ElementTable<FunctionalEventPtr>& functional_events() const {
    return functional_events_;
  }
  const ElementTable<NamedBranchPtr>& branches() const { return branches_; }
  /// @}

  /// Adds event tree constructs into the container.
  ///
  /// @param[in] element  A unique element defined in this event tree.
  ///
  /// @throws ValidationError  The element is already in this container.
  ///
  /// @{
  void Add(Sequence* element);
  void Add(FunctionalEventPtr element);
  void Add(NamedBranchPtr element);
  void Add(std::unique_ptr<Fork> element) {
    forks_.push_back(std::move(element));
  }
  /// @}

 private:
  Branch initial_state_;  ///< The starting point.

  /// Containers for unique event tree constructs defined in this event tree.
  /// @{
  ElementTable<Sequence*> sequences_;
  ElementTable<FunctionalEventPtr> functional_events_;
  ElementTable<NamedBranchPtr> branches_;
  /// @}
  std::vector<std::unique_ptr<Fork>> forks_;  ///< Lifetime management of forks.
};

using EventTreePtr = std::unique_ptr<EventTree>;  ///< Unique trees in a model.

/// Event-tree Initiating Event.
class InitiatingEvent : public Element, public Usage {
 public:
  using Element::Element;

  /// Associates an event tree to the initiating event.
  ///
  /// @param[in] event_tree  Fully initialized event tree container.
  void event_tree(EventTree* event_tree) {
    assert(!event_tree_ && event_tree && "Resetting or un-setting event tree.");
    event_tree_ = event_tree;
  }

  /// @returns The event tree of the initiating event.
  ///          nullptr if the event tree is not set.
  /// @{
  EventTree* event_tree() const { return event_tree_; }
  EventTree* event_tree() { return event_tree_; }
  /// @}

 private:
  EventTree* event_tree_ = nullptr;  ///< The optional event tree specification.
};

/// Unique initiating events in a model.
using InitiatingEventPtr = std::unique_ptr<InitiatingEvent>;

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_EVENT_TREE_H_
