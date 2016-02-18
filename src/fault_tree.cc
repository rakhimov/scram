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

/// @file fault_tree.cc
/// Implementation of fault tree and component containers.

#include "fault_tree.h"

#include <utility>

#include <boost/algorithm/string.hpp>

#include "cycle.h"
#include "error.h"

namespace scram {

Component::Component(const std::string& name, const std::string& base_path,
                     bool is_public)
    : Role::Role(is_public, base_path),
      name_(name) {}

void Component::AddGate(const GatePtr& gate) {
  Component::AddEvent(gate, &gates_);
}

void Component::AddBasicEvent(const BasicEventPtr& basic_event) {
  Component::AddEvent(basic_event, &basic_events_);
}

void Component::AddHouseEvent(const HouseEventPtr& house_event) {
  Component::AddEvent(house_event, &house_events_);
}

void Component::AddParameter(const ParameterPtr& parameter) {
  std::string name = parameter->name();
  boost::to_lower(name);
  if (parameters_.count(name)) {
    throw ValidationError("Duplicate parameter " + parameter->name());
  }
  parameters_.emplace(name, parameter);
}

void Component::AddCcfGroup(const CcfGroupPtr& ccf_group) {
  std::string name = ccf_group->name();
  boost::to_lower(name);
  if (ccf_groups_.count(name)) {
    throw ValidationError("Duplicate CCF group " + ccf_group->name());
  }
  for (const std::pair<const std::string, BasicEventPtr>& member :
       ccf_group->members()) {
    if (gates_.count(member.first) || basic_events_.count(member.first) ||
        house_events_.count(member.first)) {
      throw ValidationError("Duplicate event " + member.second->name() +
                            " from CCF group " + ccf_group->name());
    }
  }
  for (const auto& member : ccf_group->members()) basic_events_.insert(member);
  ccf_groups_.emplace(name, ccf_group);
}

void Component::AddComponent(std::unique_ptr<Component> component) {
  std::string name = component->name();
  boost::to_lower(name);
  if (components_.count(name)) {
    throw ValidationError("Duplicate component " + component->name());
  }
  components_.emplace(name, std::move(component));
}

void Component::GatherGates(std::unordered_set<GatePtr>* gates) {
  for (const std::pair<const std::string, GatePtr>& gate : gates_) {
    gates->insert(gate.second);
  }
  for (const std::pair<const std::string, ComponentPtr>& comp : components_) {
    comp.second->GatherGates(gates);
  }
}

template <class Ptr, class Container>
void Component::AddEvent(const Ptr& event, Container* container) {
  std::string name = event->name();
  boost::to_lower(name);
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    throw ValidationError("Duplicate event " + event->name());
  }
  container->emplace(name, event);
}

FaultTree::FaultTree(const std::string& name) : Component::Component(name) {}

void FaultTree::CollectTopEvents() {
  top_events_.clear();
  std::unordered_set<GatePtr> gates;
  Component::GatherGates(&gates);
  // Detects top events.
  for (const GatePtr& gate : gates) {
    FaultTree::MarkNonTopGates(gate, gates);
  }
  for (const GatePtr& gate : gates) {
    if (gate->mark() != "non-top") top_events_.push_back(gate);
    gate->mark("");  // Cleaning up.
  }
}

void FaultTree::MarkNonTopGates(const GatePtr& gate,
                                const std::unordered_set<GatePtr>& gates) {
  if (gate->mark() == "non-top") return;
  FaultTree::MarkNonTopGates(gate->formula(), gates);
}

void FaultTree::MarkNonTopGates(const FormulaPtr& formula,
                                const std::unordered_set<GatePtr>& gates) {
  for (const GatePtr& gate : formula->gate_args()) {
    if (gates.count(gate)) {
      FaultTree::MarkNonTopGates(gate, gates);
      gate->mark("non-top");
    }
  }
  for (const FormulaPtr& arg : formula->formula_args()) {
    FaultTree::MarkNonTopGates(arg, gates);
  }
}

}  // namespace scram
