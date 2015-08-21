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

Event::~Event() {}  // Empty body for pure virtual destructor.

PrimaryEvent::~PrimaryEvent() {}  // Empty body for pure virtual destructor.

CcfEvent::CcfEvent(const std::string& name, const CcfGroup* ccf_group,
                   const std::vector<std::string>& member_names)
    : BasicEvent(name, ccf_group->base_path(), ccf_group->is_public()),
      ccf_group_(ccf_group),
      member_names_(member_names) {}

void Gate::Validate() {
  // Detect inhibit flavor.
  if (formula_->type() == "and" && this->HasAttribute("flavor")) {
    const Attribute* attr = &this->GetAttribute("flavor");
    if (attr->value == "inhibit") {
      if (formula_->num_args() != 2) {
        throw ValidationError(this->name() +
                              "INHIBIT gate must have only 2 children");
      }
      std::stringstream msg;
      msg << "";
      bool conditional_found = false;
      typedef std::shared_ptr<BasicEvent> BasicEventPtr;
      std::vector<BasicEventPtr>::const_iterator it;
      for (it = formula_->basic_event_args().begin();
           it != formula_->basic_event_args().end(); ++it) {
        BasicEventPtr event = *it;
        if (!event->HasAttribute("flavor")) continue;
        std::string type = event->GetAttribute("flavor").value;
        if (type != "conditional") continue;
        if (!conditional_found) {
          conditional_found = true;
        } else {
          msg << this->name() << " : " << "INHIBIT"
              << " gate must have exactly one conditional event.\n";
        }
      }
      if (!conditional_found) {
        msg << this->name() << " : " << "INHIBIT"
            << " gate is missing a conditional event.\n";
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
        vote_number_(-1),
        gather_(true) {}

int Formula::vote_number() const {
  if (vote_number_ == -1) {
    std::string msg = "Vote number is not set for this formula.";
    throw LogicError(msg);
  }
  return vote_number_;
}

void Formula::vote_number(int vnumber) {
  if (type_ != "atleast") {
    std::string msg = "Vote number can only be defined for ATLEAST operator. "
                      "The operator of this formula is " + type_ + ".";
    throw LogicError(msg);
  } else if (vnumber < 2) {
    std::string msg = "Vote number cannot be less than 2.";
    throw InvalidArgument(msg);
  } else if (vote_number_ != -1) {
    std::string msg = "Trying to re-assign a vote number";
    throw LogicError(msg);
  }
  vote_number_ = vnumber;
}

void Formula::AddArgument(const HouseEventPtr& house_event) {
  if (event_args_.count(house_event->id())) {
    throw DuplicateArgumentError("Duplicate argument " + house_event->name());
  }
  event_args_.insert(std::make_pair(house_event->id(), house_event));
  house_event_args_.push_back(house_event);
}

void Formula::AddArgument(const BasicEventPtr& basic_event) {
  if (event_args_.count(basic_event->id())) {
    throw DuplicateArgumentError("Duplicate argument " + basic_event->name());
  }
  event_args_.insert(std::make_pair(basic_event->id(), basic_event));
  basic_event_args_.push_back(basic_event);
}

void Formula::AddArgument(const GatePtr& gate) {
  if (event_args_.count(gate->id())) {
    throw DuplicateArgumentError("Duplicate argument " + gate->name());
  }
  event_args_.insert(std::make_pair(gate->id(), gate));
  gate_args_.push_back(gate);
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
  std::vector<GatePtr>::iterator it_g;
  for (it_g = gate_args_.begin(); it_g != gate_args_.end(); ++it_g) {
    nodes_.push_back(it_g->get());
  }
  std::vector<FormulaPtr>::iterator it_f;
  for (it_f = formula_args_.begin(); it_f != formula_args_.end(); ++it_f) {
    connectors_.push_back(it_f->get());
  }
  gather_ = false;
}

}  // namespace scram
