/// @file event.cc
/// Implementation of Event Class.
#include "event.h"

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>

namespace scram {

Event::Event(std::string id, std::string name) : id_(id), name_(name) {}

void Event::AddParent(const boost::shared_ptr<Gate>& parent) {
  if (parents_.count(parent->id())) {
    std::string msg = "Trying to re-insert existing parent for " + this->name();
    throw LogicError(msg);
  }
  parents_.insert(std::make_pair(parent->id(), parent));
}

const std::map<std::string, boost::shared_ptr<Gate> >& Event::parents() {
  if (parents_.empty()) {
    std::string msg = this->name() + " does not have parents.";
    throw LogicError(msg);
  }
  return parents_;
}

Gate::Gate(std::string id, std::string type)
    : Event(id),
      type_(type),
      vote_number_(-1),
      gather_(true),
      mark_("") {}

const std::string& Gate::type() {
  if (type_ == "NONE") {
    std::string msg = "Gate type is not set for " + this->name() + " gate.";
    throw LogicError(msg);
  }
  return type_;
}

void Gate::type(std::string type) {
  if (type_ != "NONE") {
    std::string msg = "Trying to re-assign a gate type for " +
                      this->name() + " gate.";
    throw LogicError(msg);
  }
  type_ = type;
}

int Gate::vote_number() {
  if (vote_number_ == -1) {
    std::string msg = "Vote number is not set for " + this->name() + " gate.";
    throw LogicError(msg);
  }
  return vote_number_;
}

void Gate::vote_number(int vnumber) {
  if (type_ != "atleast") {
    // This line calls type() function which may throw an exception if
    // the type of this gate is not yet set.
    std::string msg = "Vote number can only be defined for the ATLEAST gate. " +
                      this->name() + " gate is " + this->type() + ".";
    throw LogicError(msg);
  } else if (vnumber < 2) {
    std::string msg = "Vote number cannot be less than 2.";
    throw InvalidArgument(msg);
  } else if (vote_number_ != -1) {
    std::string msg = "Trying to re-assign a vote number for " +
                      this->name() + " gate.";
    throw LogicError(msg);
  }
  vote_number_ = vnumber;
}

void Gate::AddChild(const boost::shared_ptr<Event>& child) {
  if (children_.count(child->id())) {
    std::string msg = "Trying to re-insert a child for " + this->name() +
                      " gate.";
    throw LogicError(msg);
  }
  children_.insert(std::make_pair(child->id(), child));
}

const std::map<std::string, boost::shared_ptr<Event> >& Gate::children() {
  if (children_.empty()) {
    std::string msg = this->name() + " gate does not have children.";
    throw LogicError(msg);
  }
  return children_;
}

void Gate::Validate() {
  std::stringstream msg;
  // Gates that should have two or more children.
  std::set<std::string> two_or_more;
  two_or_more.insert("and");
  two_or_more.insert("or");
  two_or_more.insert("nand");
  two_or_more.insert("nor");

  // Gates that should have only one child.
  std::set<std::string> single;
  single.insert("null");
  single.insert("not");
  assert(two_or_more.count(type_) || single.count(type_) ||
         type_ == "atleast" || type_ == "xor");

  std::string gate = type_;  // Copying for manipulations.

  // Detect inhibit gate.
  if (gate == "and" && this->HasAttribute("flavor")) {
    const Attribute* attr = &this->GetAttribute("flavor");
    if (attr->value == "inhibit") gate = "inhibit";
  }

  int size = children_.size();
  // Gate dependent logic.
  if (two_or_more.count(gate) && size < 2) {
    boost::to_upper(gate);
    msg << this->name() << " : " << gate << " gate must have 2 or more "
        << "children.\n";

  } else if (single.count(gate) && size != 1) {
    boost::to_upper(gate);
    msg << this->name() << " : " << gate << " gate must have only one child.";

  } else if ((gate == "xor" || gate == "inhibit") && size != 2) {
    boost::to_upper(gate);
    msg << this->name() << " : " << gate
        << " gate must have exactly 2 children.\n";

  } else if (gate == "inhibit") {
    msg << Gate::CheckInhibitGate();

  } else if (gate == "atleast" && size <= vote_number_) {
    boost::to_upper(gate);
    msg << this->name() << " : " << gate
        << " gate must have more children than its vote number "
        << vote_number_ << ".";
  }
  if (!msg.str().empty()) throw ValidationError(msg.str());
}

std::string Gate::CheckInhibitGate() {
  std::stringstream msg;
  msg << "";
  bool conditional_found = false;
  std::map<std::string, boost::shared_ptr<Event> >::iterator it;
  for (it = children_.begin(); it != children_.end(); ++it) {
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
  return msg.str();
}

void Gate::GatherNodesAndConnectors() {
  assert(nodes_.empty());
  assert(connectors_.empty());
  std::map<std::string, boost::shared_ptr<Event> >::iterator it;
  for (it = children_.begin(); it != children_.end(); ++it) {
    Gate* ptr = dynamic_cast<Gate*>(&*it->second);
    if (ptr) {
      nodes_.push_back(ptr);
    }
  }
  gather_ = false;
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
