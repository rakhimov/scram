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
/// Contains event classes for fault trees.

#pragma once

#include <cstdint>

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "element.h"
#include "expression.h"

namespace scram::mef {

/// Abstract base class for general fault tree events.
class Event : public Id, public Usage {
 public:
  static constexpr const char* kTypeString = "event";  ///< For error messages.

  using Id::Id;

  virtual ~Event() = 0;  ///< Abstract class.
};

/// Representation of a house event in a fault tree.
///
/// @note House Events with unset/uninitialized expressions default to False.
class HouseEvent : public Event {
 public:
  static constexpr const char* kTypeString = "house event";  ///< In errors.

  static HouseEvent kTrue;  ///< Literal True event.
  static HouseEvent kFalse;  ///< Literal False event.

  using Event::Event;

  HouseEvent(HouseEvent&&);  ///< For the (N)RVO only (undefined!).

  /// Sets the state for House event.
  ///
  /// @param[in] constant  False or True for the state of this house event.
  void state(bool constant) { state_ = constant; }

  /// @returns The true or false state of this house event.
  bool state() const { return state_; }

 private:
  /// Represents the state of the house event.
  /// Implies On or Off for True or False values of the probability.
  bool state_ = false;
};

class Gate;

/// Representation of a basic event in a fault tree.
class BasicEvent : public Event {
 public:
  static constexpr const char* kTypeString = "basic event";  ///< In errors.

  using Event::Event;

  virtual ~BasicEvent() = default;

  /// @returns true if the probability expression is set.
  bool HasExpression() const { return expression_ != nullptr; }

  /// Sets the expression of this basic event.
  ///
  /// @param[in] expression  The expression to describe this event.
  ///                        nullptr to remove unset the expression.
  void expression(Expression* expression) { expression_ = expression; }

  /// @returns The previously set expression for analysis purposes.
  ///
  /// @pre The expression has been set.
  Expression& expression() const {
    assert(expression_ && "The basic event's expression is not set.");
    return *expression_;
  }

  /// @returns The mean probability of this basic event.
  ///
  /// @pre The expression has been set.
  ///
  /// @note The user of this function should make sure
  ///       that the returned value is acceptable for calculations.
  double p() const noexcept {
    assert(expression_ && "The basic event's expression is not set.");
    return expression_->value();
  }

  /// Validates the probability expressions for the primary event.
  ///
  /// @pre The probability expression is set.
  ///
  /// @throws DomainError  The expression for the basic event is invalid.
  void Validate() const;

  /// Indicates if this basic event has been set to be in a CCF group.
  ///
  /// @returns true if in a CCF group.
  bool HasCcf() const { return ccf_gate_ != nullptr; }

  /// @returns CCF group gate representing this basic event.
  const Gate& ccf_gate() const {
    assert(ccf_gate_);
    return *ccf_gate_;
  }

  /// Sets the common cause failure group gate
  /// that can represent this basic event
  /// in analysis with common cause information.
  /// This information is expected to be provided by
  /// CCF group application.
  ///
  /// @param[in] gate  CCF group gate.
  void ccf_gate(std::unique_ptr<Gate> gate) {
    assert(!ccf_gate_);
    ccf_gate_ = std::move(gate);
  }

 private:
  /// Expression that describes this basic event
  /// and provides numerical values for probability calculations.
  Expression* expression_ = nullptr;

  /// If this basic event is in a common cause group,
  /// CCF gate can serve as a replacement for the basic event
  /// for common cause analysis.
  std::unique_ptr<Gate> ccf_gate_;
};

class Formula;  // To describe a gate's formula.

/// A representation of a gate in a fault tree.
class Gate : public Event, public NodeMark {
 public:
  static constexpr const char* kTypeString = "gate";  ///< Type for errors only.

  using Event::Event;

  /// @returns true if the gate formula has been set.
  bool HasFormula() const { return formula_ != nullptr; }

  /// @returns The formula of this gate.
  ///
  /// @pre The gate has its formula initialized.
  ///
  /// @{
  const Formula& formula() const {
    assert(formula_ && "Gate formula is not set.");
    return *formula_;
  }
  Formula& formula() {
    return const_cast<Formula&>(std::as_const(*this).formula());
  }
  /// @}

  /// Sets the formula of this gate.
  ///
  /// @param[in] formula  The new Boolean formula of this gate.
  ///
  /// @returns The old formula.
  std::unique_ptr<Formula> formula(std::unique_ptr<Formula> formula) {
    assert(formula && "Cannot unset formula.");
    formula_.swap(formula);
    return formula;
  }

 private:
  std::unique_ptr<Formula> formula_;  ///< Boolean formula of this gate.
};

/// Logical connectives for formulas.
/// The ordering is the same as analysis connectives in the PDAG.
enum Connective : std::uint8_t {
  kAnd = 0,
  kOr,
  kAtleast,  ///< Combination, K/N, atleast, or Vote gate representation.
  kXor,  ///< Exclusive OR gate with two inputs only.
  kNot,  ///< Boolean negation.
  kNand,  ///< Not AND.
  kNor,  ///< Not OR.
  kNull,  ///< Single argument pass-through without logic.

  // Rarely used connectives specific to the MEF.
  kIff,  ///< Equality with two inputs only.
  kImply,  ///< Implication with two inputs only.
  kCardinality  ///< General quantifier of events.
};

/// The number of connectives in the enum.
const int kNumConnectives = 11;

/// String representations of the connectives.
/// The ordering is the same as the Connective enum.
const char* const kConnectiveToString[] = {"and", "or",    "atleast",    "xor",
                                           "not", "nand",  "nor",        "null",
                                           "iff", "imply", "cardinality"};

/// Boolean formula with connectives and arguments.
/// Formulas are not expected to be shared.
class Formula {
 public:
  /// Argument events of a formula.
  using ArgEvent = std::variant<Gate*, BasicEvent*, HouseEvent*>;

  /// Formula argument with a complement flag.
  struct Arg {
    bool complement;  ///< Negation of the argument event.
    ArgEvent event;  ///< The event in the formula.
  };

  /// The set of formula arguments.
  class ArgSet {
   public:
    /// Default constructor of an empty argument set.
    ArgSet() = default;

    /// Constructors from initializer lists and iterator ranges of args.
    /// @{
    ArgSet(std::initializer_list<Arg> init_list)
        : ArgSet(init_list.begin(), init_list.end()) {}

    ArgSet(std::initializer_list<ArgEvent> init_list)
        : ArgSet(init_list.begin(), init_list.end()) {}

    template <typename Iterator>
    ArgSet(Iterator first1, Iterator last1) {
      for (; first1 != last1; ++first1)
        Add(*first1);
    }
    /// @}

    /// Adds an event into the arguments set.
    ///
    /// @param[in] event  An argument event.
    /// @param[in] complement  Indicate the negation of the argument event.
    ///
    /// @throws DuplicateElementError  The argument event is duplicate.
    void Add(ArgEvent event, bool complement = false);

    /// Overload to add formula argument with a structure.
    void Add(Arg arg) { Add(arg.event, arg.complement); }

    /// Removes an event from the formula.
    ///
    /// @param[in] event  The argument event of this formula.
    ///
    /// @throws LogicError  The argument is not in the set.
    void Remove(ArgEvent event);

    /// @returns The underlying container with the data.
    /// @{
    const std::vector<Arg>& data() const { return args_; }
    std::vector<Arg>& data() { return args_; }
    /// @}

    /// @returns The number of arguments in the set.
    std::size_t size() const { return args_.size(); }

    /// @return true if the set is empty.
    bool empty() const { return args_.empty(); }

   private:
    std::vector<Arg> args_;  ///< The underlying data container.
  };

  /// @param[in] connective  The logical connective for this Boolean formula.
  /// @param[in] args  The arguments of the formula.
  /// @param[in] min_number  The min number relevant to the connective.
  /// @param[in] max_number  The max number relevant to the connective.
  ///
  /// @throws ValidityError  Invalid arguments or setup for the connective.
  /// @throws LogicError  Invalid nesting of complement or constant args.
  /// @throws LogicError  Negative values for min or max number.
  Formula(Connective connective, ArgSet args,
          std::optional<int> min_number = {},
          std::optional<int> max_number = {});

  /// Copy semantics only.
  /// @{
  Formula(const Formula&) = default;
  Formula& operator=(const Formula&) = default;
  /// @}

  /// @returns The connective of this formula.
  Connective connective() const { return connective_; }

  /// @returns The min number for "atleast"/"cardinality" connective.
  std::optional<int> min_number() const;

  /// @returns The max number of "cardinality" connective.
  std::optional<int> max_number() const;

  /// @returns The arguments of this formula.
  const std::vector<Arg>& args() const { return args_.data(); }

  /// Swaps an argument event with another one.
  ///
  /// @param[in] current  The current argument event in this formula.
  /// @param[in] other  The replacement argument event.
  ///
  /// @post Strong exception safety guarantees.
  /// @post The complement flag is preserved.
  /// @post The position is preserved.
  ///
  /// @throws DuplicateElementError  The replacement argument is duplicate.
  /// @throws LogicError  The current argument does not belong to this formula.
  /// @throws LogicError  The replacement would result in invalid setup.
  void Swap(ArgEvent current, ArgEvent other);

 private:
  /// Validates the min and max numbers relevant to the connective.
  ///
  /// @param[in] min_number  The number to be used for connective min number.
  /// @param[in] max_number  The number to be used for connective max number.
  ///
  /// @throws LogicError  The min or max number is invalid or not applicable.
  void ValidateMinMaxNumber(std::optional<int> min_number,
                            std::optional<int> max_number);

  /// Validates the formula connective setup.
  ///
  /// @param[in] min_number  The number to be used for connective min number.
  /// @param[in] max_number  The number to be used for connective max number.
  ///
  /// @throws ValidityError  The connective setup is invalid.
  ///
  /// @pre The connective error info must be tagged outside of this function.
  void ValidateConnective(std::optional<int> min_number,
                          std::optional<int> max_number);

  /// Checks if the formula argument results in invalid nesting.
  ///
  /// @param[in] arg  The argument in the formula.
  ///
  /// @throws LogicError  Invalid nesting of complement or constant args.
  void ValidateNesting(const Arg& arg);

  Connective connective_;  ///< Logical connective.
  std::uint16_t min_number_;  ///< Min number for "atleast"/"cardinality".
  std::uint16_t max_number_;  ///< Max number for "cardinality".
  ArgSet args_;  ///< All events.
};

using FormulaPtr = std::unique_ptr<Formula>;  ///< Convenience alias.

/// Comparison of formula arguments.
inline bool operator==(const Formula::Arg& lhs,
                       const Formula::Arg& rhs) noexcept {
  return lhs.complement == rhs.complement && lhs.event == rhs.event;
}

}  // namespace scram::mef
