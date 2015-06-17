/// @file event.cc
/// Implementation of Event Class.
#include "event.h"

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>

namespace scram {

Event::Event(std::string id, std::string name)
    : id_(id),
      name_(name),
      orphan_(true),
      container_("") {}

Gate::Gate(std::string id)
    : Event(id),
      mark_("") {}

void Gate::Validate() {
  // Detect inhibit flavor.
  if (formula_->type() == "and" && this->HasAttribute("flavor")) {
    const Attribute* attr = &this->GetAttribute("flavor");
    if (attr->value == "inhibit") {
      if (formula_->num_args() != 2) {
        throw ValidationError(this->name() + "INHIBIT gate must have only 2 children");
      }

      std::stringstream msg;
      msg << "";
      bool conditional_found = false;
      std::map<std::string, boost::shared_ptr<Event> >::const_iterator it;
      for (it = formula_->event_args().begin();
           it != formula_->event_args().end(); ++it) {
        if (!boost::dynamic_pointer_cast<BasicEvent>(it->second)) continue;
        if (!it->second->HasAttribute("flavor")) continue;
        std::string type = it->second->GetAttribute("flavor").value;
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

void Formula::Validate() {
  // Formulas that should have two or more arguments.
  std::set<std::string> two_or_more;
  two_or_more.insert("and");
  two_or_more.insert("or");
  two_or_more.insert("nand");
  two_or_more.insert("nor");

  // Formulas that should have only one argument.
  std::set<std::string> single;
  single.insert("null");
  single.insert("not");
  assert(two_or_more.count(type_) || single.count(type_) ||
         type_ == "atleast" || type_ == "xor");

  std::string form = type_;  // Copying for manipulations.

  int size = formula_args_.size() + event_args_.size();
  std::stringstream msg;
  if (two_or_more.count(form) && size < 2) {
    boost::to_upper(form);
    msg << form << " formula must have 2 or more arguments.";

  } else if (single.count(form) && size != 1) {
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

int Formula::vote_number() {
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

void Formula::AddArgument(const boost::shared_ptr<Event>& event) {
  if (event_args_.count(event->id())) {
    std::string msg = "Trying to re-insert an event as an argument";
    throw LogicError(msg);
  }
  event_args_.insert(std::make_pair(event->id(), event));
}

void Formula::AddArgument(const boost::shared_ptr<Formula>& formula) {
  if (formula_args_.count(formula)) {
    std::string msg = "Trying to re-insert a formula as an argument";
    throw LogicError(msg);
  }
  formula_args_.insert(formula);
}

const std::map<std::string, boost::shared_ptr<Event> >& Formula::event_args() {
  if (event_args_.empty() && formula_args_.empty()) {
    throw LogicError("Formula does not have arguments.");
  }
  return event_args_;
}

const std::set<boost::shared_ptr<Formula> >& Formula::formula_args() {
  if (event_args_.empty() && formula_args_.empty()) {
    throw LogicError("Formula does not have arguments.");
  }
  return formula_args_;
}

void Formula::GatherNodesAndConnectors() {
  assert(nodes_.empty());
  assert(connectors_.empty());
  std::map<std::string, boost::shared_ptr<Event> >::iterator it;
  for (it = event_args_.begin(); it != event_args_.end(); ++it) {
    Gate* ptr = dynamic_cast<Gate*>(&*it->second);
    if (ptr) {
      nodes_.push_back(ptr);
    }
  }
  std::set<boost::shared_ptr<Formula> >::iterator it_f;
  for (it_f = formula_args_.begin(); it_f != formula_args_.end();
       ++it_f) {
    connectors_.push_back(&**it_f);
  }
  gather_ = false;
}

}  // namespace scram
