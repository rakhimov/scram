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
/// @file event.h
/// Contains event classes for fault trees.
#ifndef SCRAM_SRC_EVENT_H_
#define SCRAM_SRC_EVENT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "element.h"
#include "error.h"
#include "expression.h"

namespace scram {

/// @class Event
/// Abstract base class for general fault tree events.
class Event : public Element, public Role {
 public:
  /// Constructs a fault tree event with a specific id.
  /// It is assumed that names
  /// and other strings do not have
  /// leading and trailing whitespace characters.
  ///
  /// @param[in] name The identifying name with caps preserved.
  /// @param[in] base_path The series of containers to get this event.
  /// @param[in] is_public Whether or not the event is public.
  explicit Event(const std::string& name, const std::string& base_path = "",
                 bool is_public = true);

  virtual ~Event() = 0;  ///< Abstract class.

  /// @returns The id that is set upon the construction of this event.
  inline const std::string& id() const { return id_; }

  /// @returns The original name with capitalizations.
  inline const std::string& name() const { return name_; }

  /// @returns True if this node is orphan.
  inline bool orphan() const { return orphan_; }

  /// Sets the orphan state.
  ///
  /// @param[in] state True if this event is not used anywhere.
  inline void orphan(bool state) { orphan_ = state; }

 private:
  std::string id_;  ///< Id name of a event. It is in lower case.
  std::string name_;  ///< Original name with capitalizations preserved.
  bool orphan_;  ///< Indication of an orphan node.
};

/// @class PrimaryEvent
/// This is an abstract base class for events
/// that can cause failures.
/// This class represents Base, House, Undeveloped, and other events.
class PrimaryEvent : public Event {
 public:
  /// Constructs with id name and probability.
  ///
  /// @param[in] name The identifying name of this primary event.
  /// @param[in] base_path The series of containers to get this event.
  /// @param[in] is_public Whether or not the event is public.
  explicit PrimaryEvent(const std::string& name,
                        const std::string& base_path = "",
                        bool is_public = true);

  virtual ~PrimaryEvent() = 0;  ///< Abstract class.

  /// @returns A flag indicating if the event's expression is set.
  inline bool has_expression() const { return has_expression_; }

 protected:
  /// Flag to notify that expression for the event is defined.
  bool has_expression_;
};

/// @class HouseEvent
/// Representation of a house event in a fault tree.
class HouseEvent : public PrimaryEvent {
 public:
  /// Constructs with id name.
  ///
  /// @param[in] name The identifying name of this house event.
  /// @param[in] base_path The series of containers to get this event.
  /// @param[in] is_public Whether or not the event is public.
  explicit HouseEvent(const std::string& name,
                      const std::string& base_path = "",
                      bool is_public = true);

  /// Sets the state for House event.
  ///
  /// @param[in] constant False or True for the state of this house event.
  inline void state(bool constant) {
    PrimaryEvent::has_expression_ = true;
    state_ = constant;
  }

  /// @returns The true or false state of this house event.
  inline bool state() const { return state_; }

 private:
  /// Represents the state of the house event.
  /// Implies On or Off for True or False values of the probability.
  bool state_;
};

class Gate;

/// @class BasicEvent
/// Representation of a basic event in a fault tree.
class BasicEvent : public PrimaryEvent {
 public:
  typedef boost::shared_ptr<Expression> ExpressionPtr;
  typedef boost::shared_ptr<Gate> GatePtr;

  /// Constructs with id name.
  ///
  /// @param[in] name The identifying name of this basic event.
  /// @param[in] base_path The series of containers to get this event.
  /// @param[in] is_public Whether or not the event is public.
  explicit BasicEvent(const std::string& name,
                      const std::string& base_path = "",
                      bool is_public = true);

  virtual ~BasicEvent() {}

  /// Sets the expression of this basic event.
  ///
  /// @param[in] expression The expression to describe this event.
  inline void expression(const ExpressionPtr& expression) {
    assert(!expression_);
    PrimaryEvent::has_expression_ = true;
    expression_ = expression;
  }

  /// @returns The mean probability of this basic event.
  ///
  /// @note The user of this function should make sure
  ///       that the returned value is acceptable for calculations.
  ///
  /// @warning Undefined behavior if the expression is not set.
  inline double p() const {
    assert(expression_);
    return expression_->Mean();
  }

  /// Samples probability value from its probability distribution.
  ///
  /// @returns Sampled value.
  ///
  /// @note The user of this function should make sure
  ///       that the returned value is acceptable for calculations.
  ///
  /// @warning Undefined behavior if the expression is not set.
  inline double SampleProbability() {
    assert(expression_);
    return expression_->Sample();
  }

  /// Resets the sampling.
  inline void Reset() { expression_->Reset(); }

  /// @returns Indication if this event does not have uncertainty.
  inline bool IsConstant() { return expression_->IsConstant(); }

  /// Validates the probability expressions for the primary event.
  ///
  /// @throws ValidationError The expression for the basic event is invalid.
  void Validate() {
    if (expression_->Min() < 0 || expression_->Max() > 1) {
      throw ValidationError("Expression value is invalid.");
    }
  }

  /// Indicates if this basic event has been set to be in a CCF group.
  ///
  /// @returns true if in a CCF group.
  /// @returns false otherwise.
  inline bool HasCcf() const { return ccf_gate_ ? true : false; }

  /// @returns CCF group gate representing this basic event.
  inline const GatePtr& ccf_gate() const {
    assert(ccf_gate_);
    return ccf_gate_;
  }

  /// Sets the common cause failure group gate
  /// that can represent this basic event
  /// in analysis with common cause information.
  /// This information is expected to be provided by
  /// CCF group application.
  ///
  /// @param[in] gate CCF group gate.
  inline void ccf_gate(const GatePtr& gate) {
    assert(!ccf_gate_);
    ccf_gate_ = gate;
  }

 private:
  /// Expression that describes this basic event
  /// and provides numerical values for probability calculations.
  ExpressionPtr expression_;

  /// If this basic event is in a common cause group,
  /// CCF gate can serve as a replacement for the basic event
  /// for common cause analysis.
  GatePtr ccf_gate_;
};

class CcfGroup;

/// @class CcfEvent
/// A basic event that represents a multiple failure of
/// a group of events due to a common cause.
/// This event is generated out of a common cause group.
/// This class is a helper to report correctly the CCF events.
class CcfEvent : public BasicEvent {
 public:
  /// Constructs CCF event with specific name
  /// that is used for internal purposes.
  /// This name is formatted by the CcfGroup.
  /// The creator CCF group
  /// and names of the member events of this specific CCF event
  /// are saved for reporting.
  ///
  /// @param[in] name The identifying name of this CCF event.
  /// @param[in] ccf_group The CCF group that created this event.
  /// @param[in] member_names The names of members that this CCF event
  ///                         represents as multiple failure.
  CcfEvent(const std::string& name, const CcfGroup* ccf_group,
           const std::vector<std::string>& member_names);

  /// @returns Pointer to the CCF group that created this CCF event.
  inline const CcfGroup* ccf_group() const { return ccf_group_; }

  /// @returns Original names of members of this CCF event.
  inline const std::vector<std::string>& member_names() const {
    return member_names_;
  }

 private:
  const CcfGroup* ccf_group_;  ///< Pointer to the CCF group.
  /// Original names of basic events in this CCF event.
  std::vector<std::string> member_names_;
};

class Formula;  // To describe a gate's formula.

/// @class Gate
/// A representation of a gate in a fault tree.
class Gate : public Event {
 public:
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Constructs with an id and a gate.
  ///
  /// @param[in] name The identifying name with caps preserved.
  /// @param[in] base_path The series of containers to get this event.
  /// @param[in] is_public Whether or not the event is public.
  explicit Gate(const std::string& name, const std::string& base_path = "",
                bool is_public = true);

  /// @returns The formula of this gate.
  inline const FormulaPtr& formula() const { return formula_; }

  /// Sets the formula of this gate.
  ///
  /// @param[in] formula Boolean formula of this gate.
  inline void formula(const FormulaPtr& formula) {
    assert(!formula_);
    formula_ = formula;
  }

  /// This function is for cycle detection.
  ///
  /// @returns The connector between gates.
  inline Formula* connector() const { return &*formula_; }

  /// Checks if a gate is initialized correctly.
  ///
  /// @throws ValidationError Errors in the gate's logic or setup.
  void Validate();

  /// @returns The mark of this gate node.
  /// @returns Empty string for no mark.
  inline const std::string& mark() const { return mark_; }

  /// Sets the mark for this gate node.
  inline void mark(const std::string& new_mark) { mark_ = new_mark; }

 private:
  FormulaPtr formula_;  ///< Boolean formula of this gate.
  std::string mark_;  ///< The mark for traversal or toposort.
};

/// @class Formula
/// Boolean formula with operators and arguments.
/// Formulas are not expected to be shared.
class Formula {
 public:
  typedef boost::shared_ptr<Event> EventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<Formula> FormulaPtr;

  /// Constructs a formula.
  ///
  /// @param[in] type The logical operator for this Boolean formula.
  explicit Formula(const std::string& type);

  /// @returns The type of this formula.
  ///
  /// @throws LogicError The gate is not yet assigned.
  inline const std::string& type() const { return type_; }

  /// @returns The vote number if and only if the operator is ATLEAST.
  ///
  /// @throws LogicError The vote number is not yet assigned.
  int vote_number() const;

  /// Sets the vote number only for an ATLEAST formula.
  ///
  /// @param[in] vnumber The vote number.
  ///
  /// @throws InvalidArgument The vote number is invalid.
  /// @throws LogicError The vote number is assigned illegally.
  ///
  /// @note (Children number > vote number) should be checked
  ///       outside of this class.
  void vote_number(int vnumber);

  /// @returns The event arguments of this formula.
  inline const std::map<std::string, EventPtr>& event_args() const {
    return event_args_;
  }

  /// @returns The house event arguments of this formula.
  inline const std::vector<HouseEventPtr>& house_event_args() const {
    return house_event_args_;
  }

  /// @returns The basic event arguments of this formula.
  inline const std::vector<BasicEventPtr>& basic_event_args() const {
    return basic_event_args_;
  }

  /// @returns The gate arguments of this formula.
  inline const std::vector<GatePtr>& gate_args() const {
    return gate_args_;
  }

  /// @returns The formula arguments of this formula.
  inline const std::set<FormulaPtr>& formula_args() const {
    return formula_args_;
  }

  /// @returns The number of arguments.
  inline int num_args() const {
    return event_args_.size() + formula_args_.size();
  }

  /// Adds a house event into the arguments list.
  ///
  /// @param[in] house_event A pointer to an argument house event.
  ///
  /// @throws DuplicateArgumentError The argument is duplicate.
  void AddArgument(const HouseEventPtr& house_event);

  /// Adds a basic event into the arguments list.
  ///
  /// @param[in] basic_event A pointer to an argument basic event.
  ///
  /// @throws DuplicateArgumentError The argument is duplicate.
  void AddArgument(const BasicEventPtr& basic_event);

  /// Adds a gate into the arguments list.
  ///
  /// @param[in] gate A pointer to an argument gate.
  ///
  /// @throws DuplicateArgumentError The argument is duplicate.
  void AddArgument(const GatePtr& gate);

  /// Adds a formula into the arguments list.
  ///
  /// @param[in] formula A pointer to an argument formula.
  ///
  /// @throws LogicError The formula is being re-inserted.
  void AddArgument(const FormulaPtr& formula);

  /// Checks if a formula is initialized correctly with the number of arguments.
  ///
  /// @throws ValidationError There are problems with the operator or arguments.
  void Validate();

  /// @returns Gates as nodes.
  inline const std::vector<Gate*>& nodes() {
    if (gather_) Formula::GatherNodesAndConnectors();
    return nodes_;
  }

  /// @returns Formulae as connectors.
  inline const std::vector<Formula*>& connectors() {
    if (gather_) Formula::GatherNodesAndConnectors();
    return connectors_;
  }

 private:
  /// Formula types that require two or more arguments.
  static const std::set<std::string> kTwoOrMore_;
  /// Formula types that require exactly one argument.
  static const std::set<std::string> kSingle_;

  /// Gathers nodes and connectors from arguments of the gate.
  void GatherNodesAndConnectors();

  std::string type_;  ///< Logical operator.
  int vote_number_;  ///< Vote number for ATLEAST operator.
  std::map<std::string, EventPtr> event_args_;  ///< All event arguments.
  std::vector<HouseEventPtr> house_event_args_;  ///< House event arguments.
  std::vector<BasicEventPtr> basic_event_args_;  ///< Basic event arguments.
  std::vector<GatePtr> gate_args_;  ///< Arguments that are gates.
  /// Arguments that are formulas if this formula is nested.
  std::set<FormulaPtr> formula_args_;
  std::vector<Gate*> nodes_;  ///< Gate arguments as nodes.
  std::vector<Formula*> connectors_;  ///< Formulae as connectors.
  bool gather_;  ///< A flag to gather nodes and connectors.
};

}  // namespace scram

#endif  // SCRAM_SRC_EVENT_H_
