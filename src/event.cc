/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

/// @file event.cc
/// Implementation of Event Class and its derived classes.

#include "event.h"

#include <sstream>

#include <boost/algorithm/string.hpp>

#include "ccf_group.h"

namespace scram {

Event::Event(const std::string& name, const std::string& base_path,
             bool is_public)
    : Role::Role(is_public, base_path),
      name_(name),
      orphan_(true) {
  assert(name != "");
  id_ = is_public ? name : base_path + "." + name;  // Unique combination.
  boost::to_lower(id_);
}

Event::~Event() = default;
PrimaryEvent::~PrimaryEvent() = default;

CcfEvent::CcfEvent(const std::string& name, const CcfGroup* ccf_group,
                   const std::vector<std::string>& member_names)
    : BasicEvent(name, ccf_group->base_path(), ccf_group->is_public()),
      ccf_group_(ccf_group),
      member_names_(member_names) {}

void Gate::Validate() {
  // Detect inhibit flavor.
  if (formula_->type() == "and" && Element::HasAttribute("flavor")) {
    const Attribute* attr = &Element::GetAttribute("flavor");
    if (attr->value == "inhibit") {
      if (formula_->num_args() != 2) {
        throw ValidationError(Event::name() +
                              "INHIBIT gate must have only 2 children");
      }
      std::stringstream msg;
      msg << "";
      bool conditional_found = false;
      for (const BasicEventPtr& event : formula_->basic_event_args()) {
        if (!event->HasAttribute("flavor")) continue;
        std::string type = event->GetAttribute("flavor").value;
        if (type != "conditional") continue;
        if (!conditional_found) {
          conditional_found = true;
        } else {
          msg << Event::name() << " : INHIBIT gate must have"
              << " exactly one conditional event.\n";
        }
      }
      if (!conditional_found) {
        msg << Event::name()
            << " : INHIBIT gate is missing a conditional event.\n";
      }
      if (!msg.str().empty()) throw ValidationError(msg.str());
    }
  }
}

const std::set<std::string> Formula::kTwoOrMore_ = {{"and"}, {"or"}, {"nand"},
                                                    {"nor"}};

const std::set<std::string> Formula::kSingle_ = {{"not"}, {"null"}};

Formula::Formula(const std::string& type)
      : type_(type),
        vote_number_(0),
        gather_(true) {}

int Formula::vote_number() const {
  if (!vote_number_) throw LogicError("Vote number is not set.");
  return vote_number_;
}

void Formula::vote_number(int number) {
  if (type_ != "atleast") {
    std::string msg = "Vote number can only be defined for 'atleast' formulas. "
                      "The operator of this formula is " + type_ + ".";
    throw LogicError(msg);
  } else if (number < 2) {
    std::string msg = "Vote number cannot be less than 2.";
    throw InvalidArgument(msg);
  } else if (vote_number_) {
    std::string msg = "Trying to re-assign a vote number";
    throw LogicError(msg);
  }
  vote_number_ = number;
}

void Formula::AddArgument(FormulaPtr formula) {
  formula_args_.emplace_back(std::move(formula));
}

void Formula::Validate() {
  assert(kTwoOrMore_.count(type_) || kSingle_.count(type_) ||
         type_ == "atleast" || type_ == "xor");

  std::string form = type_;  // Copying for manipulations.

  int size = formula_args_.size() + event_args_.size();
  std::stringstream msg;
  if (kTwoOrMore_.count(form) && size < 2) {
    boost::to_upper(form);
    msg << form << " formula must have 2 or more arguments.";

  } else if (kSingle_.count(form) && size != 1) {
    boost::to_upper(form);
    msg << form << " formula must have only one argument.";

  } else if (form == "xor" && size != 2) {
    boost::to_upper(form);
    msg << form << " formula must have exactly 2 arguments.";

  } else if (form == "atleast" && size <= vote_number_) {
    boost::to_upper(form);
    msg << form << " formula must have more arguments than its vote number "
        << vote_number_ << ".";
  }
  if (!msg.str().empty()) throw ValidationError(msg.str());
}

void Formula::GatherNodesAndConnectors() {
  assert(nodes_.empty());
  assert(connectors_.empty());
  for (const GatePtr& gate : gate_args_) {
    nodes_.push_back(gate.get());
  }
  for (const FormulaPtr& formula : formula_args_) {
    connectors_.push_back(formula.get());
  }
  gather_ = false;
}

}  // namespace scram
