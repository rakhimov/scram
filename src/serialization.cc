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

/// @file serialization.cc

#include "serialization.h"

#include <memory>

#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_file_open_mode.hpp>

#include "element.h"
#include "event.h"
#include "expression.h"
#include "expression/constant.h"
#include "expression/exponential.h"
#include "fault_tree.h"
#include "xml_stream.h"

namespace scram {
namespace mef {

void Serialize(const Model& model, const std::string& file) {
  std::unique_ptr<std::FILE, decltype(&std::fclose)> fp(
      std::fopen(file.c_str(), "w"), &std::fclose);
  try {
    if (!fp) {
      SCRAM_THROW(IOError("Cannot open the output file for serialization."))
          << boost::errinfo_errno(errno) << boost::errinfo_file_open_mode("w");
    }
    Serialize(model, fp.get());
  } catch (IOError& err) {
    err << boost::errinfo_file_name(file);
    throw;
  }
}

namespace {  // The serialization helper functions for each model construct.

void SerializeLabelAndAttributes(const Element& element,
                                 xml::StreamElement* xml_element) {
  if (element.label().empty() == false)
    xml_element->AddChild("label").AddText(element.label());
  if (element.attributes().empty() == false) {
    xml::StreamElement attributes_container =
        xml_element->AddChild("attributes");
    for (const Attribute& attribute : element.attributes()) {
      xml::StreamElement attribute_element =
          attributes_container.AddChild("attribute");
      assert(attribute.name.empty() == false);
      attribute_element.SetAttribute("name", attribute.name);
      assert(attribute.value.empty() == false);
      attribute_element.SetAttribute("value", attribute.value);
      if (attribute.type.empty() == false)
        attribute_element.SetAttribute("type", attribute.type);
    }
  }
}

void SerializeElement(const Element& element, xml::StreamElement* xml_element) {
  xml_element->SetAttribute("name", element.name());
  SerializeLabelAndAttributes(element, xml_element);
}

void Serialize(const Formula& formula, xml::StreamElement* parent) {
  assert(formula.formula_args().empty());
  struct ArgStreamer {
    void operator()(const Gate* gate) const {
      xml->AddChild("gate").SetAttribute("name", gate->name());
    }
    void operator()(const BasicEvent* basic_event) const {
      xml->AddChild("basic-event").SetAttribute("name", basic_event->name());
    }
    void operator()(const HouseEvent* house_event) const {
      xml->AddChild("house-event").SetAttribute("name", house_event->name());
    }

    xml::StreamElement* xml;
  };
  if (formula.type() == kNull) {
    assert(formula.event_args().size() == 1);
    boost::apply_visitor(ArgStreamer{parent}, formula.event_args().front());
  } else {
    xml::StreamElement type_element = [&formula, &parent] {
      switch (formula.type()) {
        case kNot:
          return parent->AddChild("not");
        case kAnd:
          return parent->AddChild("and");
        case kOr:
          return parent->AddChild("or");
        case kNand:
          return parent->AddChild("nand");
        case kNor:
          return parent->AddChild("nor");
        case kXor:
          return parent->AddChild("xor");
        case kVote:
          return [&formula, &parent] {
            xml::StreamElement atleast = parent->AddChild("atleast");
            atleast.SetAttribute("min", formula.vote_number());
            return atleast;
          }();  // Wrap NRVO into RVO for GCC.
        default:
          assert(false && "Unexpected formula");
      }
    }();
    for (const Formula::EventArg& arg : formula.event_args())
      boost::apply_visitor(ArgStreamer{&type_element}, arg);
  }
}

void Serialize(const Gate& gate, xml::StreamElement* parent) {
  assert(gate.role() == RoleSpecifier::kPublic);
  xml::StreamElement gate_element = parent->AddChild("define-gate");
  SerializeElement(gate, &gate_element);
  Serialize(gate.formula(), &gate_element);
}

void Serialize(const FaultTree& fault_tree, xml::StreamElement* parent) {
  assert(fault_tree.components().empty());
  assert(fault_tree.role() == RoleSpecifier::kPublic);
  xml::StreamElement ft_element = parent->AddChild("define-fault-tree");
  SerializeElement(fault_tree, &ft_element);
  for (Gate* gate : fault_tree.gates())
    Serialize(*gate, &ft_element);
}

void Serialize(const Expression& expression, xml::StreamElement* parent) {
  if (const auto* constant =
          dynamic_cast<const ConstantExpression*>(&expression)) {
    /// @todo Track the original value type of the constant expression.
    parent->AddChild("float").SetAttribute(
        "value", const_cast<ConstantExpression*>(constant)->value());
  } else if (const auto* exponential =
                 dynamic_cast<const Exponential*>(&expression)) {
    xml::StreamElement xml = parent->AddChild("exponential");
    assert(exponential->args().size() == 2);
    for (const Expression* arg : exponential->args())
      Serialize(*arg, &xml);
  } else {
    assert(false && "Unsupported expression");
  }
}

void Serialize(const BasicEvent& basic_event, xml::StreamElement* parent) {
  assert(basic_event.role() == RoleSpecifier::kPublic);
  xml::StreamElement be_element = parent->AddChild("define-basic-event");
  SerializeElement(basic_event, &be_element);
  if (basic_event.HasExpression())
    Serialize(basic_event.expression(), &be_element);
}

void Serialize(const HouseEvent& house_event, xml::StreamElement* parent) {
  assert(house_event.role() == RoleSpecifier::kPublic);
  assert(&house_event != &HouseEvent::kTrue &&
         &house_event != &HouseEvent::kFalse);
  xml::StreamElement he_element = parent->AddChild("define-house-event");
  SerializeElement(house_event, &he_element);
  he_element.AddChild("constant")
      .SetAttribute("value", house_event.state() ? "true" : "false");
}

}  // namespace

void Serialize(const Model& model, std::FILE* out) {
  xml::Stream xml_stream(out);
  xml::StreamElement root = xml_stream.root("opsa-mef");
  if (!model.HasDefaultName())
    root.SetAttribute("name", model.name());
  SerializeLabelAndAttributes(model, &root);
  /// @todo Implement serialization for the following unsupported constructs.
  assert(model.ccf_groups().empty());
  assert(model.parameters().empty());
  assert(model.initiating_events().empty());
  assert(model.event_trees().empty());
  assert(model.sequences().empty());
  assert(model.rules().empty());

  for (const FaultTreePtr& fault_tree : model.fault_trees())
    Serialize(*fault_tree, &root);

  xml::StreamElement model_data = root.AddChild("model-data");
  for (const BasicEventPtr& basic_event : model.basic_events())
    Serialize(*basic_event, &model_data);
  for (const HouseEventPtr& house_event : model.house_events())
    Serialize(*house_event, &model_data);
}

}  // namespace mef
}  // namespace scram
