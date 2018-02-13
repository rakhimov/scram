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
/// Implementation of input file processing into analysis constructs.

#include "initializer.h"

#include <functional>  // std::mem_fn
#include <sstream>
#include <type_traits>

#include <boost/exception/errinfo_at_line.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/algorithm.hpp>

#include "cycle.h"
#include "env.h"
#include "error.h"
#include "expression/boolean.h"
#include "expression/conditional.h"
#include "expression/exponential.h"
#include "expression/extern.h"
#include "expression/numerical.h"
#include "expression/random_deviate.h"
#include "expression/test_event.h"
#include "ext/algorithm.h"
#include "ext/find_iterator.h"
#include "logger.h"

namespace scram::mef {

namespace {  // Helper function and wrappers for MEF initializations.

/// Maps string to the role specifier.
///
/// @param[in] s  Non-empty, valid role specifier string.
///
/// @returns Role specifier attribute for elements.
RoleSpecifier GetRole(const std::string_view& s) {
  assert(!s.empty());
  assert(s == "public" || s == "private");
  return s == "public" ? RoleSpecifier::kPublic : RoleSpecifier::kPrivate;
}

/// Takes into account the parent role upon producing element role.
///
/// @param[in] s  Potentially empty role specifier string.
/// @param[in] parent_role  The role to be inherited.
///
/// @returns The role for the element under consideration.
RoleSpecifier GetRole(const std::string_view& s, RoleSpecifier parent_role) {
  return s.empty() ? parent_role : GetRole(s);
}

/// Attaches attributes and a label to the elements of the analysis.
/// These attributes are not XML attributes
/// but the Open-PSA format defined arbitrary attributes
/// and a label that can be attached to many analysis elements.
///
/// @param[in] xml_element  XML element.
/// @param[out] element  The object that needs attributes and label.
///
/// @throws ValidityError  Invalid attribute setting.
void AttachLabelAndAttributes(const xml::Element& xml_element,
                              Element* element) {
  if (std::optional<xml::Element> label = xml_element.child("label")) {
    assert(element->label().empty() && "Resetting element label.");
    element->label(std::string(label->text()));
  }

  std::optional<xml::Element> attributes = xml_element.child("attributes");
  if (!attributes)
    return;
  for (const xml::Element& attribute : attributes->children()) {
    assert(attribute.name() == "attribute");
    try {
      element->AddAttribute({std::string(attribute.attribute("name")),
                             std::string(attribute.attribute("value")),
                             std::string(attribute.attribute("type"))});
    } catch (ValidityError& err) {
      err << boost::errinfo_at_line(attribute.line());
      throw;
    }
  }
}

/// Constructs Element of type T from an XML element.
template <class T>
std::enable_if_t<std::is_base_of_v<Element, T>, std::unique_ptr<T>>
ConstructElement(const xml::Element& xml_element) {
  auto element =
      std::make_unique<T>(std::string(xml_element.attribute("name")));
  AttachLabelAndAttributes(xml_element, element.get());
  return element;
}

/// Constructs Element of type T with a role from an XML element.
template <class T>
std::enable_if_t<std::is_base_of_v<Role, T>, std::unique_ptr<T>>
ConstructElement(const xml::Element& xml_element, const std::string& base_path,
                 RoleSpecifier base_role) {
  auto element =
      std::make_unique<T>(std::string(xml_element.attribute("name")), base_path,
                          GetRole(xml_element.attribute("role"), base_role));
  AttachLabelAndAttributes(xml_element, element.get());
  return element;
}

/// Filters the data for MEF Element definitions.
///
/// @param[in] xml_element  The XML element with the construct definition.
///
/// @returns A range of XML child elements of MEF Element constructs.
auto GetNonAttributeElements(const xml::Element& xml_element) {
  return xml_element.children() |
         boost::adaptors::filtered([](const xml::Element& child) {
           std::string_view name = child.name();
           return name != "label" && name != "attributes";
         });
}

template <>
std::unique_ptr<Phase>
ConstructElement<Phase>(const xml::Element& xml_element) {
  std::unique_ptr<Phase> element;
  try {
    element = std::make_unique<Phase>(
        std::string(xml_element.attribute("name")),
        *xml_element.attribute<double>("time-fraction"));
  } catch (ValidityError& err) {
    err << boost::errinfo_at_line(xml_element.line());
    throw;
  }
  AttachLabelAndAttributes(xml_element, element.get());
  return element;
}

}  // namespace

Initializer::Initializer(const std::vector<std::string>& xml_files,
                         core::Settings settings, bool allow_extern,
                         xml::Validator* extra_validator)
    : settings_(std::move(settings)),
      allow_extern_(allow_extern),
      extra_validator_(extra_validator) {
  BLOG(WARNING, allow_extern_) << "Enabling external dynamic libraries";
  ProcessInputFiles(xml_files);
}

void Initializer::CheckFileExistence(
    const std::vector<std::string>& xml_files) {
  for (auto& xml_file : xml_files) {
    if (boost::filesystem::exists(xml_file) == false) {
      SCRAM_THROW(IOError("Input file doesn't exist."))
          << boost::errinfo_file_name(xml_file);
    }
  }
}

void Initializer::CheckDuplicateFiles(
    const std::vector<std::string>& xml_files) {
  namespace fs = boost::filesystem;
  // Collection of input file locations in canonical path.
  std::unordered_set<std::string> full_paths;
  for (auto& xml_file : xml_files) {
    auto [it, is_unique] = full_paths.emplace(fs::canonical(xml_file).string());
    if (!is_unique)
      SCRAM_THROW(IOError("Duplicate input file"))
          << boost::errinfo_file_name(*it);
  }
}

void Initializer::ProcessInputFiles(const std::vector<std::string>& xml_files) {
  static xml::Validator validator(env::input_schema());

  CLOCK(input_time);
  LOG(DEBUG1) << "Processing input files";
  CheckFileExistence(xml_files);
  CheckDuplicateFiles(xml_files);
  for (const auto& xml_file : xml_files) {
    CLOCK(parse_time);
    LOG(DEBUG3) << "Parsing " << xml_file << " ...";
    xml::Document document(xml_file, &validator);
    if (extra_validator_)
      extra_validator_->validate(document);
    documents_.emplace_back(std::move(document));
    LOG(DEBUG3) << "Parsed " << xml_file << " in " << DUR(parse_time);
  }
  CLOCK(def_time);
  for (const xml::Document& document : documents_) {
    try {
      ProcessInputFile(document);
    } catch (ValidityError& err) {
      err << boost::errinfo_file_name(document.root().filename());
      throw;
    }
  }
  ProcessTbdElements();
  LOG(DEBUG2) << "Element definition time " << DUR(def_time);
  LOG(DEBUG1) << "Input files are processed in " << DUR(input_time);

  CLOCK(valid_time);
  LOG(DEBUG1) << "Validating the initialization";
  // Check if the initialization is successful.
  ValidateInitialization();
  LOG(DEBUG1) << "Validation is finished in " << DUR(valid_time);

  CLOCK(setup_time);
  LOG(DEBUG1) << "Setting up for the analysis";
  // Perform setup for analysis using configurations from the input files.
  SetupForAnalysis();
  EnsureNoCcfSubstitutions();
  EnsureSubstitutionsWithApproximations();
  LOG(DEBUG1) << "Setup time " << DUR(setup_time);
}

template <class T>
void Initializer::Register(std::unique_ptr<T> element,
                           const xml::Element& xml_element) {
  try {
    model_->Add(std::move(element));
  } catch (ValidityError& err) {
    err << boost::errinfo_at_line(xml_element.line());
    throw;
  }
}

/// Specializations for element registrations.
/// @{
template <>
Gate* Initializer::Register(const xml::Element& gate_node,
                            const std::string& base_path,
                            RoleSpecifier container_role) {
  std::unique_ptr<Gate> ptr =
      ConstructElement<Gate>(gate_node, base_path, container_role);
  auto* gate = ptr.get();
  Register(std::move(ptr), gate_node);
  path_gates_.insert(gate);
  tbd_.emplace_back(gate, gate_node);
  return gate;
}

template <>
BasicEvent* Initializer::Register(const xml::Element& event_node,
                                  const std::string& base_path,
                                  RoleSpecifier container_role) {
  std::unique_ptr<BasicEvent> ptr =
      ConstructElement<BasicEvent>(event_node, base_path, container_role);
  auto* basic_event = ptr.get();
  Register(std::move(ptr), event_node);
  path_basic_events_.insert(basic_event);
  tbd_.emplace_back(basic_event, event_node);
  return basic_event;
}

template <>
HouseEvent* Initializer::Register(const xml::Element& event_node,
                                  const std::string& base_path,
                                  RoleSpecifier container_role) {
  std::unique_ptr<HouseEvent> ptr =
      ConstructElement<HouseEvent>(event_node, base_path, container_role);
  auto* house_event = ptr.get();
  Register(std::move(ptr), event_node);
  path_house_events_.insert(house_event);

  // Only Boolean xml.
  if (std::optional<xml::Element> constant = event_node.child("constant")) {
    house_event->state(*constant->attribute<bool>("value"));
  }
  return house_event;
}

template <>
Parameter* Initializer::Register(const xml::Element& param_node,
                                 const std::string& base_path,
                                 RoleSpecifier container_role) {
  std::unique_ptr<Parameter> ptr =
      ConstructElement<Parameter>(param_node, base_path, container_role);
  auto* parameter = ptr.get();
  Register(std::move(ptr), param_node);
  path_parameters_.insert(parameter);
  tbd_.emplace_back(parameter, param_node);

  // Attach units.
  std::string_view unit = param_node.attribute("unit");
  if (!unit.empty()) {
    int pos = boost::find(kUnitsToString, unit) - std::begin(kUnitsToString);
    assert(pos < kNumUnits && "Unexpected unit kind.");
    parameter->unit(static_cast<Units>(pos));
  }
  return parameter;
}

template <>
CcfGroup* Initializer::Register(const xml::Element& ccf_node,
                                const std::string& base_path,
                                RoleSpecifier container_role) {
  auto ptr = [&]() -> std::unique_ptr<CcfGroup> {
    std::string_view model = ccf_node.attribute("model");
    if (model == "beta-factor")
      return ConstructElement<BetaFactorModel>(ccf_node, base_path,
                                               container_role);
    if (model == "MGL")
      return ConstructElement<MglModel>(ccf_node, base_path, container_role);
    if (model == "alpha-factor")
      return ConstructElement<AlphaFactorModel>(ccf_node, base_path,
                                                container_role);
    assert(model == "phi-factor" && "Unrecognized CCF model.");
    return ConstructElement<PhiFactorModel>(ccf_node, base_path,
                                            container_role);
  }();
  auto* ccf_group = ptr.get();
  Register(std::move(ptr), ccf_node);

  ProcessCcfMembers(*ccf_node.child("members"), ccf_group);

  tbd_.emplace_back(ccf_group, ccf_node);
  return ccf_group;
}

template <>
Sequence* Initializer::Register(const xml::Element& xml_node,
                                const std::string& /*base_path*/,
                                RoleSpecifier /*container_role*/) {
  std::unique_ptr<Sequence> ptr = ConstructElement<Sequence>(xml_node);
  auto* sequence = ptr.get();
  Register(std::move(ptr), xml_node);
  tbd_.emplace_back(sequence, xml_node);
  return sequence;
}
/// @}

void Initializer::ProcessInputFile(const xml::Document& document) {
  xml::Element root = document.root();
  assert(root.name() == "opsa-mef");

  if (!model_) {  // Create only one model for multiple files.
    model_ = ConstructElement<Model>(root);
    model_->mission_time().value(settings_.mission_time());
  }

  for (const xml::Element& node : root.children()) {
    if (node.name() == "define-initiating-event") {
      std::unique_ptr<InitiatingEvent> initiating_event =
          ConstructElement<InitiatingEvent>(node);
      auto* ref_ptr = initiating_event.get();
      Register(std::move(initiating_event), node);
      tbd_.emplace_back(ref_ptr, node);

    } else if (node.name() == "define-rule") {
      std::unique_ptr<Rule> rule = ConstructElement<Rule>(node);
      auto* ref_ptr = rule.get();
      Register(std::move(rule), node);
      tbd_.emplace_back(ref_ptr, node);

    } else if (node.name() == "define-event-tree") {
      DefineEventTree(node);

    } else if (node.name() == "define-fault-tree") {
      DefineFaultTree(node);

    } else if (node.name() == "define-CCF-group") {
      Register<CcfGroup>(node, "", RoleSpecifier::kPublic);

    } else if (node.name() == "define-alignment") {
      std::unique_ptr<Alignment> alignment = ConstructElement<Alignment>(node);
      auto* address = alignment.get();
      Register(std::move(alignment), node);
      tbd_.emplace_back(address, node);

    } else if (node.name() == "define-substitution") {
      std::unique_ptr<Substitution> substitution =
          ConstructElement<Substitution>(node);
      auto* address = substitution.get();
      Register(std::move(substitution), node);
      tbd_.emplace_back(address, node);

    } else if (node.name() == "model-data") {
      ProcessModelData(node);

    } else if (node.name() == "define-extern-library") {
      if (!allow_extern_) {
        SCRAM_THROW(
            IllegalOperation("Loading external libraries is disallowed!"))
            << boost::errinfo_file_name(node.filename())
            << boost::errinfo_at_line(node.line());
      }
      DefineExternLibraries(node);
    }
  }
}

/// Specializations for elements defined after registration.
/// @{
template <>
void Initializer::Define(const xml::Element& gate_node, Gate* gate) {
  auto formulas = GetNonAttributeElements(gate_node);
  // Assumes that there are no attributes and labels.
  assert(!formulas.empty() && ++formulas.begin() == formulas.end());
  assert(!gate->HasFormula() && "Resetting gate formula");
  gate->formula(GetFormula(*formulas.begin(), gate->base_path()));
}

template <>
void Initializer::Define(const xml::Element& event_node,
                         BasicEvent* basic_event) {
  auto expressions = GetNonAttributeElements(event_node);
  if (!expressions.empty()) {
    assert(basic_event->HasExpression() == false && "Resetting expressions.");
    basic_event->expression(
        GetExpression(*expressions.begin(), basic_event->base_path()));
  } else if (settings_.probability_analysis()) {
    SCRAM_THROW(ValidityError("The basic event does not have an expression."))
        << errinfo_element(basic_event->id(), "basic event")
        << boost::errinfo_at_line(event_node.line());
  }
}

template <>
void Initializer::Define(const xml::Element& param_node, Parameter* parameter) {
  auto expressions = GetNonAttributeElements(param_node);
  assert(!expressions.empty() && ++expressions.begin() == expressions.end());
  parameter->expression(
      GetExpression(*expressions.begin(), parameter->base_path()));
}

template <>
void Initializer::Define(const xml::Element& ccf_node, CcfGroup* ccf_group) {
  for (const xml::Element& element : ccf_node.children()) {
    std::string_view name = element.name();
    if (name == "distribution") {
      ccf_group->AddDistribution(
          GetExpression(*element.child(), ccf_group->base_path()));

    } else if (name == "factor") {
      DefineCcfFactor(element, ccf_group);

    } else if (name == "factors") {
      for (const xml::Element& factor_node : element.children())
        DefineCcfFactor(factor_node, ccf_group);
    }
  }
}

template <>
void Initializer::Define(const xml::Element& xml_node, Sequence* sequence) {
  std::vector<Instruction*> instructions;
  for (const xml::Element& node : GetNonAttributeElements(xml_node)) {
    instructions.emplace_back(GetInstruction(node));
  }
  sequence->instructions(std::move(instructions));
}

template <>
void Initializer::Define(const xml::Element& et_node, EventTree* event_tree) {
  for (const xml::Element& node : et_node.children("define-branch")) {
    auto it = ext::find(event_tree->table<NamedBranch>(),
                        std::string(node.attribute("name")));
    assert(it);
    DefineBranch(GetNonAttributeElements(node), event_tree, &*it);
  }
  Branch initial_state;
  DefineBranch(et_node.child("initial-state")->children(), event_tree,
               &initial_state);
  event_tree->initial_state(std::move(initial_state));
}

template <>
void Initializer::Define(const xml::Element& xml_node,
                         InitiatingEvent* initiating_event) {
  std::string_view event_tree_name = xml_node.attribute("event-tree");
  if (!event_tree_name.empty()) {
    try {
      auto& event_tree = model_->Get<EventTree>(event_tree_name);
      initiating_event->event_tree(&event_tree);
      initiating_event->usage(true);
      event_tree.usage(true);

    } catch (Error& err) {
      err << boost::errinfo_at_line(xml_node.line());
      throw;
    }
  }
}

template <>
void Initializer::Define(const xml::Element& rule_node, Rule* rule) {
  std::vector<Instruction*> instructions;
  for (const xml::Element& xml_node : GetNonAttributeElements(rule_node))
    instructions.push_back(GetInstruction(xml_node));
  rule->instructions(std::move(instructions));
}

template <>
void Initializer::Define(const xml::Element& xml_node, Alignment* alignment) {
  for (const xml::Element& node : xml_node.children("define-phase")) {
    try {
      std::unique_ptr<Phase> phase = ConstructElement<Phase>(node);
      std::vector<SetHouseEvent*> instructions;
      for (const xml::Element& arg : node.children("set-house-event")) {
        instructions.push_back(
            static_cast<SetHouseEvent*>(GetInstruction(arg)));
      }
      phase->instructions(std::move(instructions));
      alignment->Add(std::move(phase));
    } catch (ValidityError& err) {
      err << boost::errinfo_at_line(node.line());
      throw;
    }
  }
  try {
    alignment->Validate();
  } catch (ValidityError& err) {
    err << boost::errinfo_at_line(xml_node.line());
    throw;
  }
}

template <>
void Initializer::Define(const xml::Element& xml_node,
                         Substitution* substitution) {
  substitution->hypothesis(
      GetFormula(xml_node.child("hypothesis")->child().value(), ""));

  if (std::optional<xml::Element> source = xml_node.child("source")) {
    for (const xml::Element& basic_event : source->children()) {
      assert(basic_event.name() == "basic-event");
      try {
        BasicEvent* event = GetBasicEvent(basic_event.attribute("name"), "");
        substitution->Add(event);
        event->usage(true);
      } catch (ValidityError& err) {
        err << boost::errinfo_at_line(basic_event.line());
        throw;
      }
    }
    assert(substitution->source().empty() == false);
  }

  xml::Element target = xml_node.child("target")->child().value();
  if (target.name() == "basic-event") {
    try {
      BasicEvent* event = GetBasicEvent(target.attribute("name"), "");
      substitution->target(event);
      event->usage(true);
    } catch (ValidityError& err) {
      err << boost::errinfo_at_line(target.line());
      throw;
    }
  } else {
    assert(target.name() == "constant");
    substitution->target(target.attribute<bool>("value").value());
  }

  try {
    substitution->Validate();
    std::string_view type = xml_node.attribute("type");
    if (!type.empty()) {
      std::optional<Substitution::Type> deduced_type = substitution->type();
      int pos = std::distance(kSubstitutionTypeToString,
                              boost::find(kSubstitutionTypeToString, type));
      assert(pos < 3 && "Unexpected substitution type string.");
      if (!deduced_type ||
          static_cast<Substitution::Type>(pos) != deduced_type.value())
        SCRAM_THROW(ValidityError(
            "The declared substitution type does not match the deduced one."));
    }
  } catch (ValidityError& err) {
    err << boost::errinfo_at_line(xml_node.line());
    throw;
  }
}
/// @}

void Initializer::ProcessTbdElements() {
  for (const xml::Document& document : documents_) {
    xml::Element root = document.root();
    for (const xml::Element& node : root.children("define-extern-function")) {
      try {
        DefineExternFunction(node);
      } catch (ValidityError& err) {
        err << boost::errinfo_file_name(root.filename());
        throw;
      }
    }
  }

  for (const auto& [tbd_element, xml_element] : tbd_) {
    try {
      std::visit(
          [this, &xml_element](auto* tbd_construct) {
            this->Define(xml_element, tbd_construct);
          },
          tbd_element);
    } catch (ValidityError& err) {
      err << boost::errinfo_file_name(xml_element.filename());
      throw;
    }
  }
}

void Initializer::DefineEventTree(const xml::Element& et_node) {
  std::unique_ptr<EventTree> event_tree = ConstructElement<EventTree>(et_node);
  for (const xml::Element& node : et_node.children()) {
    if (node.name() == "define-sequence") {
      event_tree->Add(
          Register<Sequence>(node, event_tree->name(), RoleSpecifier::kPublic));
    } else {
      try {
        if (node.name() == "define-branch") {
          event_tree->Add(ConstructElement<NamedBranch>(node));
        } else if (node.name() == "define-functional-event") {
          std::unique_ptr<FunctionalEvent> event =
              ConstructElement<FunctionalEvent>(node);
          event->order(event_tree->functional_events().size() + 1);
          event_tree->Add(std::move(event));
        }
      } catch (ValidityError& err) {
        err << boost::errinfo_at_line(node.line());
        throw;
      }
    }
  }
  EventTree* tbd_element = event_tree.get();
  Register(std::move(event_tree), et_node);
  // Save only after registration.
  tbd_.emplace_back(tbd_element, et_node);
}

void Initializer::DefineFaultTree(const xml::Element& ft_node) {
  std::unique_ptr<FaultTree> fault_tree = ConstructElement<FaultTree>(ft_node);
  RegisterFaultTreeData(ft_node, fault_tree->name(), fault_tree.get());
  Register(std::move(fault_tree), ft_node);
}

std::unique_ptr<Component> Initializer::DefineComponent(
    const xml::Element& component_node, const std::string& base_path,
    RoleSpecifier container_role) {
  std::unique_ptr<Component> component =
      ConstructElement<Component>(component_node, base_path, container_role);
  RegisterFaultTreeData(component_node, base_path + "." + component->name(),
                        component.get());
  return component;
}

void Initializer::RegisterFaultTreeData(const xml::Element& ft_node,
                                        const std::string& base_path,
                                        Component* component) {
  for (const xml::Element& node : ft_node.children()) {
    if (node.name() == "define-basic-event") {
      component->Add(Register<BasicEvent>(node, base_path, component->role()));

    } else if (node.name() == "define-parameter") {
      component->Add(Register<Parameter>(node, base_path, component->role()));

    } else if (node.name() == "define-gate") {
      component->Add(Register<Gate>(node, base_path, component->role()));

    } else if (node.name() == "define-house-event") {
      component->Add(Register<HouseEvent>(node, base_path, component->role()));

    } else if (node.name() == "define-CCF-group") {
      component->Add(Register<CcfGroup>(node, base_path, component->role()));

    } else if (node.name() == "define-component") {
      std::unique_ptr<Component> sub =
          DefineComponent(node, base_path, component->role());
      try {
        component->Add(std::move(sub));
      } catch (ValidityError& err) {
        err << boost::errinfo_at_line(node.line());
        throw;
      }
    }
  }
}

void Initializer::ProcessModelData(const xml::Element& model_data) {
  for (const xml::Element& node : model_data.children()) {
    if (node.name() == "define-basic-event") {
      Register<BasicEvent>(node, "", RoleSpecifier::kPublic);
    } else if (node.name() == "define-parameter") {
      Register<Parameter>(node, "", RoleSpecifier::kPublic);
    } else if (node.name() == "define-house-event") {
      Register<HouseEvent>(node, "", RoleSpecifier::kPublic);
    }
  }
}

std::unique_ptr<Formula> Initializer::GetFormula(
    const xml::Element& formula_node, const std::string& base_path) {
  Connective formula_type = [&formula_node]() {
    if (formula_node.has_attribute("name") || formula_node.name() == "constant")
      return kNull;
    int pos = boost::find(kConnectiveToString, formula_node.name()) -
              std::begin(kConnectiveToString);
    assert(pos < kNumConnectives && "Unexpected connective.");
    return static_cast<Connective>(pos);
  }();

  Formula::ArgSet arg_set;

  auto add_event = [this, &arg_set, &base_path](const xml::Element& element,
                                                bool complement) {
    std::string_view element_type = [&element] {
      // This is for the case "<event name="id" type="type"/>".
      std::string_view type = element.attribute("type");
      return type.empty() ? element.name() : type;
    }();

    std::string_view name = element.attribute("name");
    assert(!name.empty() && "Not an appropriate XML element for arg Event.");

    try {
      auto arg_event = [this, &element_type, &name,
                        &base_path]() -> Formula::ArgEvent {
        if (element_type == "event") {  // Undefined type yet.
          return GetEvent(name, base_path);

        } else if (element_type == "gate") {
          return GetGate(name, base_path);

        } else if (element_type == "basic-event") {
          return GetBasicEvent(name, base_path);

        } else {
          assert(element_type == "house-event");
          return GetHouseEvent(name, base_path);
        }
      }();
      arg_set.Add(arg_event, complement);
    } catch (ValidityError& err) {
      err << boost::errinfo_at_line(element.line());
      throw;
    }
  };

  auto add_arg = [this, &arg_set, &add_event](const xml::Element& element) {
    if (element.name() == "constant") {
      arg_set.Add(*element.attribute<bool>("value") ? &HouseEvent::kTrue
                                                    : &HouseEvent::kFalse);
      return;
    }

    if (element.name() == "not") {
      assert(element.children().size() == 1);
      add_event(element.child().value(), /*complement=*/true);
    } else {
      add_event(element, /*complement=*/false);
    }
  };

  // Process arguments of this formula.
  if (formula_type == kNull) {  // Special case of pass-through.
    add_arg(formula_node);
  } else {
    for (const xml::Element& node : formula_node.children())
      add_arg(node);
  }

  try {
    return std::make_unique<Formula>(formula_type, std::move(arg_set),
                                     formula_node.attribute<int>("min"),
                                     formula_node.attribute<int>("max"));
  } catch (ValidityError& err) {
    err << boost::errinfo_at_line(formula_node.line());
    throw;
  }
}

void Initializer::DefineBranchTarget(const xml::Element& target_node,
                                     EventTree* event_tree, Branch* branch) {
  try {
    if (target_node.name() == "fork") {
      auto& functional_event = event_tree->Get<FunctionalEvent>(
          target_node.attribute("functional-event"));
      std::vector<Path> paths;
      for (const xml::Element& path_element : target_node.children("path")) {
        paths.emplace_back(std::string(path_element.attribute("state")));
        DefineBranch(path_element.children(), event_tree, &paths.back());
      }
      assert(!paths.empty());
      try {
        auto fork = std::make_unique<Fork>(functional_event, std::move(paths));
        branch->target(fork.get());
        event_tree->Add(std::move(fork));
        functional_event.usage(true);
      } catch (ValidityError& err) {
        err << errinfo_container(event_tree->name(), "event tree");
        throw;
      }
    } else if (target_node.name() == "sequence") {
      auto& sequence = model_->Get<Sequence>(target_node.attribute("name"));
      branch->target(&sequence);
      sequence.usage(true);
    } else {
      assert(target_node.name() == "branch");
      auto& named_branch =
          event_tree->Get<NamedBranch>(target_node.attribute("name"));
      branch->target(&named_branch);
      named_branch.usage(true);
    }
  } catch (ValidityError& err) {
    err << boost::errinfo_at_line(target_node.line());
    throw;
  }
}

template <class SinglePassRange>
void Initializer::DefineBranch(const SinglePassRange& xml_nodes,
                               EventTree* event_tree, Branch* branch) {
  assert(!xml_nodes.empty() && "At least the branch target must be defined.");

  std::vector<Instruction*> instructions;
  for (auto it = xml_nodes.begin(), it_end = xml_nodes.end(); it != it_end;) {
    auto it_cur = it++;
    if (it == it_end) {
      DefineBranchTarget(*it_cur, event_tree, branch);
    } else {
      instructions.emplace_back(GetInstruction(*it_cur));
    }
  }
  branch->instructions(std::move(instructions));
}

Instruction* Initializer::GetInstruction(const xml::Element& xml_element) {
  std::string_view node_name = xml_element.name();
  auto invoke = [&xml_element](auto&& action) {
    try {
      return action();
    } catch (UndefinedElement& err) {
      err << boost::errinfo_at_line(xml_element.line());
      throw;
    }
  };

  if (node_name == "rule") {
    return invoke([&xml_element, this] {
      auto& rule = model_->Get<Rule>(xml_element.attribute("name"));
      rule.usage(true);
      return &rule;
    });
  }

  auto register_instruction = [this](std::unique_ptr<Instruction> instruction) {
    auto* ret_ptr = instruction.get();
    model_->Add(std::move(instruction));
    return ret_ptr;
  };

  if (node_name == "event-tree") {
    return invoke([&] {
      auto& event_tree = model_->Get<EventTree>(xml_element.attribute("name"));
      event_tree.usage(true);
      links_.push_back(static_cast<Link*>(
          register_instruction(std::make_unique<Link>(event_tree))));
      return links_.back();
    });
  }

  if (node_name == "collect-expression") {
    return register_instruction(std::make_unique<CollectExpression>(
        GetExpression(*xml_element.child(), "")));
  }

  if (node_name == "collect-formula") {
    return register_instruction(
        std::make_unique<CollectFormula>(GetFormula(*xml_element.child(), "")));
  }

  if (node_name == "if") {
    xml::Element::Range args = xml_element.children();
    auto it = args.begin();
    Expression* if_expression = GetExpression(*it++, "");
    Instruction* then_instruction = GetInstruction(*it++);
    Instruction* else_instruction =
        it == args.end() ? nullptr : GetInstruction(*it);

    return register_instruction(std::make_unique<IfThenElse>(
        if_expression, then_instruction, else_instruction));
  }

  if (node_name == "block") {
    std::vector<Instruction*> instructions;
    for (const xml::Element& xml_node : xml_element.children())
      instructions.push_back(GetInstruction(xml_node));
    return register_instruction(
        std::make_unique<Block>(std::move(instructions)));
  }

  if (node_name == "set-house-event") {
    std::string_view name = xml_element.attribute("name");
    if (!model_->house_events().count(name)) {
      SCRAM_THROW(UndefinedElement())
          << errinfo_element(std::string(name), "house event")
          << boost::errinfo_at_line(xml_element.line());
    }
    return register_instruction(std::make_unique<SetHouseEvent>(
        std::string(name), *xml_element.child()->attribute<bool>("value")));
  }

  assert(false && "Unknown instruction type.");
}

template <class T, int N>
struct Initializer::Extractor {
  /// Extracts expressions
  /// to be passed to the constructor of expression T.
  ///
  /// @param[in] args  XML elements containing the arguments.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] init  The host Initializer.
  ///
  /// @returns The extracted expression.
  ///
  /// @pre The XML args container size equals N.
  std::unique_ptr<T> operator()(const xml::Element::Range& args,
                                const std::string& base_path,
                                Initializer* init) {
    static_assert(N > 0, "The number of arguments can't be fewer than 1.");
    return (*this)(args.begin(), args.end(), base_path, init);
  }

  /// Extracts and accumulates expressions
  /// to be passed to the constructor of expression T.
  ///
  /// @tparam Ts  Expression types.
  ///
  /// @param[in] it  The iterator in the argument container.
  /// @param[in] it_end  The end sentinel iterator of the argument container.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] init  The host Initializer.
  /// @param[in] expressions  Accumulated argument expressions.
  ///
  /// @returns The extracted expression.
  ///
  /// @pre The XML container has enough arguments.
  template <class... Ts>
  std::unique_ptr<T> operator()(xml::Element::Range::iterator it,
                                xml::Element::Range::iterator it_end,
                                const std::string& base_path, Initializer* init,
                                Ts&&... expressions) {
    static_assert(N >= 0);

    if constexpr (N == 0) {
      static_assert(sizeof...(Ts), "Unintended use case.");
      assert(it == it_end && "Too many arguments in the args container.");
      return std::make_unique<T>(std::forward<Ts>(expressions)...);

    } else {
      assert(it != it_end && "Not enough arguments in the args container.");
      return Extractor<T, N - 1>()(std::next(it), it_end, base_path, init,
                                   std::forward<Ts>(expressions)...,
                                   init->GetExpression(*it, base_path));
    }
  }
};

/// Specialization of Extractor to extract all expressions into arg vector.
template <class T>
struct Initializer::Extractor<T, -1> {
  /// Constructs an expression with a variable number of arguments.
  ///
  /// @param[in] args  XML elements containing the arguments.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] init  The host Initializer.
  ///
  /// @returns The constructed expression.
  std::unique_ptr<T> operator()(const xml::Element::Range& args,
                                const std::string& base_path,
                                Initializer* init) {
    std::vector<Expression*> expr_args;
    for (const xml::Element& node : args) {
      expr_args.push_back(init->GetExpression(node, base_path));
    }
    return std::make_unique<T>(std::move(expr_args));
  }
};

namespace {  // Expression extraction helper functions.

/// @returns The number of constructor arguments for Expression types.
/// @{
template <class T, class A, class... As>
constexpr int count_args() {
  if constexpr (std::is_constructible_v<T, A, As...>) {
    return 1 + sizeof...(As);
  } else {
    return count_args<T, A, A, As...>();
  }
}

template <class T>
constexpr std::enable_if_t<std::is_base_of_v<Expression, T>, int> num_args() {
  static_assert(!std::is_default_constructible_v<T>, "No zero args.");
  if constexpr (std::is_constructible_v<T, std::vector<Expression*>>) {
    return -1;
  } else {
    return count_args<T, Expression*>();
  }
}
/// @}

}  // namespace

template <class T>
std::unique_ptr<Expression>
Initializer::Extract(const xml::Element::Range& args,
                     const std::string& base_path, Initializer* init) {
  return Extractor<T, num_args<T>()>()(args, base_path, init);
}

/// Specialization for Extractor of Histogram expressions.
template <>
std::unique_ptr<Expression>
Initializer::Extract<Histogram>(const xml::Element::Range& args,
                                const std::string& base_path,
                                Initializer* init) {
  auto it = args.begin();
  std::vector<Expression*> boundaries = {init->GetExpression(*it, base_path)};
  std::vector<Expression*> weights;
  for (++it; it != args.end(); ++it) {
    xml::Element::Range bin = it->children();
    assert(bin.size() == 2);
    auto it_bin = bin.begin();
    boundaries.push_back(init->GetExpression(*it_bin++, base_path));
    weights.push_back(init->GetExpression(*it_bin, base_path));
  }
  assert(!weights.empty() && "At least one bin must be present.");
  return std::make_unique<Histogram>(std::move(boundaries), std::move(weights));
}

/// Specialization due to overloaded constructors.
template <>
std::unique_ptr<Expression>
Initializer::Extract<LognormalDeviate>(const xml::Element::Range& args,
                                       const std::string& base_path,
                                       Initializer* init) {
  if (args.size() == 3)
    return Extractor<LognormalDeviate, 3>()(args, base_path, init);
  return Extractor<LognormalDeviate, 2>()(args, base_path, init);
}

/// Specialization due to overloaded constructors and un-fixed number of args.
template <>
std::unique_ptr<Expression>
Initializer::Extract<PeriodicTest>(const xml::Element::Range& args,
                                   const std::string& base_path,
                                   Initializer* init) {
  switch (args.size()) {
    case 4:
      return Extractor<PeriodicTest, 4>()(args, base_path, init);
    case 5:
      return Extractor<PeriodicTest, 5>()(args, base_path, init);
    case 11:
      return Extractor<PeriodicTest, 11>()(args, base_path, init);
    default:
      SCRAM_THROW(
          ValidityError("Invalid number of arguments for Periodic Test."));
  }
}

/// Specialization for Switch-Case operation extraction.
template <>
std::unique_ptr<Expression>
Initializer::Extract<Switch>(const xml::Element::Range& args,
                             const std::string& base_path, Initializer* init) {
  assert(!args.empty());
  Expression* default_value = nullptr;
  std::vector<Switch::Case> cases;
  for (auto it = args.begin(), it_end = args.end(); it != it_end;) {
    auto it_cur = it++;
    if (it == it_end) {
      default_value = init->GetExpression(*it_cur, base_path);
      break;
    }
    xml::Element::Range nodes = it_cur->children();
    assert(nodes.size() == 2);
    auto it_node = nodes.begin();
    cases.push_back({*init->GetExpression(*it_node++, base_path),
                     *init->GetExpression(*it_node, base_path)});
  }
  assert(default_value);
  return std::make_unique<Switch>(std::move(cases), default_value);
}

const Initializer::ExtractorMap Initializer::kExpressionExtractors_ = {
    {"exponential", &Extract<Exponential>},
    {"GLM", &Extract<Glm>},
    {"Weibull", &Extract<Weibull>},
    {"periodic-test", &Extract<PeriodicTest>},
    {"uniform-deviate", &Extract<UniformDeviate>},
    {"normal-deviate", &Extract<NormalDeviate>},
    {"lognormal-deviate", &Extract<LognormalDeviate>},
    {"gamma-deviate", &Extract<GammaDeviate>},
    {"beta-deviate", &Extract<BetaDeviate>},
    {"histogram", &Extract<Histogram>},
    {"neg", &Extract<Neg>},
    {"add", &Extract<Add>},
    {"sub", &Extract<Sub>},
    {"mul", &Extract<Mul>},
    {"div", &Extract<Div>},
    {"abs", &Extract<Abs>},
    {"acos", &Extract<Acos>},
    {"asin", &Extract<Asin>},
    {"atan", &Extract<Atan>},
    {"cos", &Extract<Cos>},
    {"sin", &Extract<Sin>},
    {"tan", &Extract<Tan>},
    {"cosh", &Extract<Cosh>},
    {"sinh", &Extract<Sinh>},
    {"tanh", &Extract<Tanh>},
    {"exp", &Extract<Exp>},
    {"log", &Extract<Log>},
    {"log10", &Extract<Log10>},
    {"mod", &Extract<Mod>},
    {"pow", &Extract<Pow>},
    {"sqrt", &Extract<Sqrt>},
    {"ceil", &Extract<Ceil>},
    {"floor", &Extract<Floor>},
    {"min", &Extract<Min>},
    {"max", &Extract<Max>},
    {"mean", &Extract<Mean>},
    {"not", &Extract<Not>},
    {"and", &Extract<And>},
    {"or", &Extract<Or>},
    {"eq", &Extract<Eq>},
    {"df", &Extract<Df>},
    {"lt", &Extract<Lt>},
    {"gt", &Extract<Gt>},
    {"leq", &Extract<Leq>},
    {"geq", &Extract<Geq>},
    {"ite", &Extract<Ite>},
    {"switch", &Extract<Switch>}};

Expression* Initializer::GetExpression(const xml::Element& expr_element,
                                       const std::string& base_path) {
  std::string_view expr_type = expr_element.name();
  auto register_expression = [this](std::unique_ptr<Expression> expression) {
    auto* ret_ptr = expression.get();
    model_->Add(std::move(expression));
    return ret_ptr;
  };
  if (expr_type == "int") {
    int val = *expr_element.attribute<int>("value");
    return register_expression(std::make_unique<ConstantExpression>(val));
  }
  if (expr_type == "float") {
    double val = *expr_element.attribute<double>("value");
    return register_expression(std::make_unique<ConstantExpression>(val));
  }
  if (expr_type == "bool") {
    bool val = *expr_element.attribute<bool>("value");
    return val ? &ConstantExpression::kOne : &ConstantExpression::kZero;
  }
  if (expr_type == "pi")
    return &ConstantExpression::kPi;

  if (expr_type == "test-initiating-event") {
    return register_expression(std::make_unique<TestInitiatingEvent>(
        std::string(expr_element.attribute("name")), model_->context()));
  }
  if (expr_type == "test-functional-event") {
    return register_expression(std::make_unique<TestFunctionalEvent>(
        std::string(expr_element.attribute("name")),
        std::string(expr_element.attribute("state")), model_->context()));
  }

  if (expr_type == "extern-function") {
    const ExternFunction<void>* extern_function = [this, &expr_element] {
      try {
        auto& ret =
            model_->Get<ExternFunction<void>>(expr_element.attribute("name"));
        ret.usage(true);
        return &ret;

      } catch (UndefinedElement& err) {
        err << boost::errinfo_at_line(expr_element.line());
        throw;
      }
    }();

    std::vector<Expression*> expr_args;
    for (const xml::Element& node : expr_element.children())
      expr_args.push_back(GetExpression(node, base_path));

    try {
      return register_expression(extern_function->apply(std::move(expr_args)));
    } catch (ValidityError& err) {
      err << boost::errinfo_at_line(expr_element.line());
      throw;
    }
  }

  if (auto* expression = GetParameter(expr_type, expr_element, base_path))
    return expression;

  try {
    Expression* expression = register_expression(kExpressionExtractors_.at(
        expr_type)(expr_element.children(), base_path, this));
    // Register for late validation after ensuring no cycles.
    expressions_.emplace_back(expression, expr_element);
    return expression;
  } catch (ValidityError& err) {
    err << boost::errinfo_at_line(expr_element.line());
    throw;
  }
}

Expression* Initializer::GetParameter(const std::string_view& expr_type,
                                      const xml::Element& expr_element,
                                      const std::string& base_path) {
  auto check_units = [&expr_element](const auto& parameter) {
    std::string_view unit = expr_element.attribute("unit");
    const char* param_unit = scram::mef::kUnitsToString[parameter.unit()];
    if (!unit.empty() && unit != param_unit) {
      std::stringstream msg;
      msg << "Parameter unit mismatch.\nExpected: " << param_unit
          << "\nGiven: " << unit;
      SCRAM_THROW(ValidityError(msg.str()))
          << boost::errinfo_at_line(expr_element.line());
    }
  };

  if (expr_type == "parameter") {
    try {
      Parameter* param =
          GetParameter(expr_element.attribute("name"), base_path);
      param->usage(true);
      check_units(*param);
      return param;
    } catch (ValidityError& err) {
      err << boost::errinfo_at_line(expr_element.line());
      throw;
    }
  } else if (expr_type == "system-mission-time") {
    check_units(model_->mission_time());
    return &model_->mission_time();
  }
  return nullptr;  // The expression is not a parameter.
}

void Initializer::ProcessCcfMembers(const xml::Element& members_node,
                                    CcfGroup* ccf_group) {
  for (const xml::Element& event_node : members_node.children()) {
    assert("basic-event" == event_node.name());
    auto basic_event =
        std::make_unique<BasicEvent>(std::string(event_node.attribute("name")),
                                     ccf_group->base_path(), ccf_group->role());
    try {
      ccf_group->AddMember(basic_event.get());
    } catch (DuplicateElementError& err) {
      err << boost::errinfo_at_line(event_node.line());
      throw;
    }
    Register(std::move(basic_event), event_node);
  }
}

void Initializer::DefineCcfFactor(const xml::Element& factor_node,
                                  CcfGroup* ccf_group) {
  Expression* expression =
      GetExpression(*factor_node.child(), ccf_group->base_path());

  try {
    ccf_group->AddFactor(expression, factor_node.attribute<int>("level"));
  } catch (ValidityError& err) {
    err << boost::errinfo_at_line(factor_node.line());
    throw;
  }
}

Parameter* Initializer::GetParameter(std::string_view entity_reference,
                                     const std::string& base_path) {
  return GetEntity(entity_reference, base_path, model_->table<Parameter>(),
                   TableRange(path_parameters_));
}

HouseEvent* Initializer::GetHouseEvent(std::string_view entity_reference,
                                       const std::string& base_path) {
  return GetEntity(entity_reference, base_path, model_->table<HouseEvent>(),
                   TableRange(path_house_events_));
}

BasicEvent* Initializer::GetBasicEvent(std::string_view entity_reference,
                                       const std::string& base_path) {
  return GetEntity(entity_reference, base_path, model_->table<BasicEvent>(),
                   TableRange(path_basic_events_));
}

Gate* Initializer::GetGate(std::string_view entity_reference,
                           const std::string& base_path) {
  return GetEntity(entity_reference, base_path, model_->table<Gate>(),
                   TableRange(path_gates_));
}

template <class P, class T>
T* Initializer::GetEntity(std::string_view entity_reference,
                          const std::string& base_path,
                          const TableRange<IdTable<P>>& container,
                          const TableRange<PathTable<T>>& path_container) {
  assert(!entity_reference.empty());
  if (!base_path.empty()) {  // Check the local scope.
    std::string full_path = base_path + ".";
    full_path.append(entity_reference.data(), entity_reference.size());
    if (auto it = ext::find(path_container, full_path))
      return &*it;
  }

  auto at = [&entity_reference, &base_path](const auto& reference_container) {
    if (auto it = ext::find(reference_container, entity_reference))
      return &*it;
    SCRAM_THROW(UndefinedElement())
        << errinfo_reference(std::string(entity_reference))
        << errinfo_base_path(base_path) << errinfo_element_type(T::kTypeString);
  };

  if (entity_reference.find('.') == std::string_view::npos)  // Public entity.
    return at(container);

  return at(path_container);  // Direct access.
}

/// Helper macro for Initializer::GetEvent event discovery.
#define GET_EVENT(gates, basic_events, house_events, path_reference) \
  do {                                                               \
    if (auto it = ext::find(gates, path_reference))                  \
      return &*it;                                                   \
    if (auto it = ext::find(basic_events, path_reference))           \
      return &*it;                                                   \
    if (auto it = ext::find(house_events, path_reference))           \
      return &*it;                                                   \
  } while (false)

Formula::ArgEvent Initializer::GetEvent(std::string_view entity_reference,
                                        const std::string& base_path) {
  // Do not implement this in terms of
  // GetGate, GetBasicEvent, or GetHouseEvent.
  // The semantics for local lookup with the base type is different.
  assert(!entity_reference.empty());
  if (!base_path.empty()) {  // Check the local scope.
    std::string full_path = base_path + ".";
    full_path.append(entity_reference.data(), entity_reference.size());
    GET_EVENT(TableRange(path_gates_), TableRange(path_basic_events_),
              TableRange(path_house_events_), full_path);
  }

  if (entity_reference.find('.') == std::string_view::npos) {  // Public entity.
    GET_EVENT(model_->table<Gate>(), model_->table<BasicEvent>(),
              model_->table<HouseEvent>(), entity_reference);
  } else {  // Direct access.
    GET_EVENT(TableRange(path_gates_), TableRange(path_basic_events_),
              TableRange(path_house_events_), entity_reference);
  }
  SCRAM_THROW(UndefinedElement())
      << errinfo_reference(std::string(entity_reference))
      << errinfo_base_path(base_path) << errinfo_element_type("event");
}

#undef GET_EVENT

void Initializer::DefineExternLibraries(const xml::Element& xml_node) {
  auto optional_bool = [&xml_node](const char* tag) {
    std::optional<bool> attribute = xml_node.attribute<bool>(tag);
    return attribute ? *attribute : false;
  };
  auto library = [&xml_node, &optional_bool] {
    try {
      return std::make_unique<ExternLibrary>(
          std::string(xml_node.attribute("name")),
          std::string(xml_node.attribute("path")),
          boost::filesystem::path(xml_node.filename()).parent_path(),
          optional_bool("system"), optional_bool("decorate"));
    } catch (DLError& err) {
      err << boost::errinfo_file_name(xml_node.filename())
          << boost::errinfo_at_line(xml_node.line());
      throw;
    } catch (ValidityError& err) {
      err << boost::errinfo_at_line(xml_node.line());
      throw;
    }
  }();
  AttachLabelAndAttributes(xml_node, library.get());
  Register(std::move(library), xml_node);
}

namespace {  // Extern function initialization helpers.

/// All the allowed extern function parameter types.
///
/// @note Template code may be optimized for these types only.
enum class ExternParamType { kInt = 1, kDouble };
const int kExternTypeBase = 3;  ///< The information base for encoding.
const int kMaxNumParam = 5;  ///< The max number of args (excludes the return).
const int kNumInterfaces = 126;  ///< All possible interfaces.

/// Encodes parameter types kExternTypeBase base number.
///
/// @tparam SinglePassRange  The forward range type
///
/// @param[in] args  The non-empty XML elements encoding parameter types.
///
/// @returns Unique integer encoding parameter types.
///
/// @pre The number of parameters is less than log_base(max int).
template <class SinglePassRange>
int Encode(const SinglePassRange& args) noexcept {
  assert(!args.empty());
  auto to_digit = [](const xml::Element& node) -> int {
    std::string_view name = node.name();
    return static_cast<int>([&name] {
      if (name == "int")
        return ExternParamType::kInt;
      assert(name == "double");
      return ExternParamType::kDouble;
    }());
  };

  int ret = 0;
  int base_power = 1;  // Base ^ (pos - 1).
  for (const xml::Element& node : args) {
    ret += base_power * to_digit(node);
    base_power *= kExternTypeBase;
  }

  return ret;
}

/// Encodes function parameter types at compile-time.
template <typename T, typename... Ts>
constexpr int Encode(int base_power = 1) noexcept {
  if constexpr (sizeof...(Ts)) {
    return Encode<T>(base_power) + Encode<Ts...>(base_power * kExternTypeBase);

  } else if constexpr (std::is_same_v<T, int>) {
    return base_power * static_cast<int>(ExternParamType::kInt);

  } else {
    static_assert(std::is_same_v<T, double>);
    return base_power * static_cast<int>(ExternParamType::kDouble);
  }
}

using ExternFunctionExtractor = ExternFunctionPtr (*)(std::string,
                                                      const std::string&,
                                                      const ExternLibrary&);
using ExternFunctionExtractorMap =
    std::unordered_map<int, ExternFunctionExtractor>;

/// Generates all extractors for extern functions.
///
/// @tparam N  The number of parameters.
/// @tparam Ts  The return and parameter types of the extern function.
///
/// @param[in,out] function_map  The destination container for extractor.
template <int N, typename... Ts>
void GenerateExternFunctionExtractor(ExternFunctionExtractorMap* function_map) {
  static_assert(N >= 0);
  static_assert(sizeof...(Ts));

  if constexpr (N == 0) {
    function_map->emplace(
        Encode<Ts...>(),
        [](std::string name, const std::string& symbol,
           const ExternLibrary& library) -> ExternFunctionPtr {
          return std::make_unique<ExternFunction<Ts...>>(std::move(name),
                                                         symbol, library);
        });
  } else {
    GenerateExternFunctionExtractor<0, Ts...>(function_map);
    GenerateExternFunctionExtractor<N - 1, Ts..., int>(function_map);
    GenerateExternFunctionExtractor<N - 1, Ts..., double>(function_map);
  }
}

}  // namespace

void Initializer::DefineExternFunction(const xml::Element& xml_element) {
  static const ExternFunctionExtractorMap function_extractors = [] {
    ExternFunctionExtractorMap function_map;
    function_map.reserve(kNumInterfaces);
    GenerateExternFunctionExtractor<kMaxNumParam, int>(&function_map);
    GenerateExternFunctionExtractor<kMaxNumParam, double>(&function_map);
    assert(function_map.size() == kNumInterfaces);
    return function_map;
  }();

  const ExternLibrary& library = [this, &xml_element]() -> decltype(auto) {
    try {
      auto& lib = model_->Get<ExternLibrary>(xml_element.attribute("library"));
      lib.usage(true);
      return lib;
    } catch (UndefinedElement& err) {
      err << boost::errinfo_at_line(xml_element.line());
      throw;
    }
  }();

  ExternFunctionPtr extern_function = [&xml_element, &library] {
    auto args = GetNonAttributeElements(xml_element);
    assert(!args.empty());
    /// @todo Optimize extern-function num args violation detection.
    int num_args = std::distance(args.begin(), args.end()) - /*return*/ 1;
    if (num_args > kMaxNumParam) {
      SCRAM_THROW(ValidityError("The number of function parameters '" +
                                std::to_string(num_args) +
                                "' exceeds the number of allowed parameters '" +
                                std::to_string(kMaxNumParam) + "'"))
          << boost::errinfo_at_line(xml_element.line());
    }
    int encoding = Encode(args);
    try {
      return function_extractors.at(encoding)(
          std::string(xml_element.attribute("name")),
          std::string(xml_element.attribute("symbol")), library);
    } catch (Error& err) {
      err << boost::errinfo_at_line(xml_element.line());
      throw;
    }
  }();

  Register(std::move(extern_function), xml_element);
}

void Initializer::ValidateInitialization() {
  // Check if *all* gates have no cycles.
  cycle::CheckCycle<Gate>(model_->table<Gate>(), "gate");

  // Check for cycles in event tree instruction rules.
  cycle::CheckCycle<Rule>(model_->table<Rule>(), "rule");

  // Check for cycles in event tree branches.
  for (EventTree& event_tree : model_->table<EventTree>()) {
    try {
      cycle::CheckCycle<NamedBranch>(event_tree.table<NamedBranch>(), "branch");
    } catch (CycleError& err) {
      err << errinfo_container(event_tree.name(), "event tree");
      throw;
    }
  }

  // All other event-tree checks available after ensuring no-cycles in branches.
  for (const EventTree& event_tree : model_->event_trees()) {
    try {
      for (const NamedBranch& branch : event_tree.branches()) {
        CheckFunctionalEventOrder(branch);  // The order of events in forks.
        EnsureLinksOnlyInSequences(branch);  // Link instructions in sequences.
      }
      CheckFunctionalEventOrder(event_tree.initial_state());
      EnsureLinksOnlyInSequences(event_tree.initial_state());
    } catch (ValidityError& err) {
      err << errinfo_container(event_tree.name(), "event tree");
      throw;
    }
  }

  // The cycles in links are checked only after ensuring their valid locations.
  cycle::CheckCycle<Link>(boost::adaptors::indirect(links_), "event-tree link");

  // Event-tree instruction homogeneity checks only after cycle checks.
  for (const EventTree& event_tree : model_->event_trees()) {
    try {
      for (const NamedBranch& branch : event_tree.branches()) {
        EnsureHomogeneousEventTree(branch);  // No mixed instructions.
      }
      EnsureHomogeneousEventTree(event_tree.initial_state());
    } catch (ValidityError& err) {
      err << errinfo_container(event_tree.name(), "event tree");
      throw;
    }
  }

  EnsureNoSubstitutionConflicts();

  ValidateExpressions();
}

void Initializer::CheckFunctionalEventOrder(const Branch& branch) {
  struct CheckOrder {
    void operator()(Sequence*) const {}

    void operator()(NamedBranch* named_branch) const {
      std::visit(*this, named_branch->target());
    }

    void operator()(Fork* fork) const {
      if (functional_event.order() == fork->functional_event().order()) {
        assert(&functional_event == &fork->functional_event());
        SCRAM_THROW(ValidityError("Functional event " +
                                  functional_event.name() +
                                  " is duplicated in event tree fork paths."));
      }

      if (functional_event.order() > fork->functional_event().order())
        SCRAM_THROW(ValidityError(
            "Functional event " + functional_event.name() +
            " must appear after functional event " +
            fork->functional_event().name() + " in event tree fork paths."));
    }

    const FunctionalEvent& functional_event;
  };

  struct OrderValidator {
    void operator()(Sequence*) const {}
    void operator()(NamedBranch*) const {}
    void operator()(Fork* fork) const {
      for (const Path& fork_path : fork->paths()) {
        initializer->CheckFunctionalEventOrder(fork_path);
        std::visit(CheckOrder{fork->functional_event()}, fork_path.target());
      }
    }
    Initializer* initializer;
  };

  std::visit(OrderValidator{this}, branch.target());
}

void Initializer::EnsureLinksOnlyInSequences(const Branch& branch) {
  struct Validator : public NullVisitor {
    void Visit(const Link* link) override {
      SCRAM_THROW(ValidityError("Link " + link->event_tree().name() +
                                " can only be used in end-state sequences."));
    }
  };

  struct {
    void operator()(Sequence*) {}
    void operator()(const NamedBranch*) {}

    void operator()(const Branch* arg_branch) {
      for (const Instruction* instruction : arg_branch->instructions())
        instruction->Accept(&validator);
      std::visit(*this, arg_branch->target());
    }

    void operator()(Fork* fork) {
      for (const Path& fork_path : fork->paths())
        (*this)(&fork_path);
    }
    Validator validator;
  } link_checker;

  link_checker(&branch);
}

void Initializer::EnsureHomogeneousEventTree(const Branch& branch) {
  enum Type { kUnknown, kExpression, kFormula };

  struct Visitor : public NullVisitor {
    void Visit(const CollectExpression*) override {
      switch (type) {
        case kFormula:
          SCRAM_THROW(
              ValidityError("Mixed collect-expression and collect-formula"));
        case kUnknown:
          type = kExpression;
        case kExpression:
          break;
      }
    }

    void Visit(const CollectFormula*) override {
      switch (type) {
        case kExpression:
          SCRAM_THROW(
              ValidityError("Mixed collect-expression and collect-formula"));
        case kUnknown:
          type = kFormula;
        case kFormula:
          break;
      }
    }

    void Visit(const Link* link) override {
      (*this)(&link->event_tree().initial_state());
    }

    void CheckInstructions(const std::vector<Instruction*>& instructions) {
      for (const Instruction* instruction : instructions)
        instruction->Accept(this);
    }

    void operator()(const Sequence* sequence) {
      CheckInstructions(sequence->instructions());
    }
    void operator()(const Branch* arg_branch) {
      CheckInstructions(arg_branch->instructions());
      std::visit(*this, arg_branch->target());
    }
    void operator()(const Fork* fork) {
      for (const Path& fork_path : fork->paths())
        (*this)(&fork_path);
    }

    Type type = kUnknown;
  } homogeneous_checker;

  homogeneous_checker(&branch);
}

void Initializer::EnsureNoSubstitutionConflicts() {
  auto substitutions = model_->substitutions() |
                       boost::adaptors::filtered([](const auto& substitution) {
                         return !substitution.declarative();
                       });
  for (const Substitution& origin : substitutions) {
    const auto* target_ptr = std::get_if<BasicEvent*>(&origin.target());
    for (const Substitution& substitution : substitutions) {
      if (target_ptr && boost::count(substitution.source(), *target_ptr))
        SCRAM_THROW(
            ValidityError("Non-declarative substitution target event should "
                          "not appear in any substitution source."))
            << errinfo_element(origin.name(), "substitution");
      if (&origin == &substitution)
        continue;
      auto in_hypothesis = [&substitution](const BasicEvent* source) {
        return ext::any_of(substitution.hypothesis().args(),
                           [source](const Formula::Arg& arg) {
                             return std::get<BasicEvent*>(arg.event) == source;
                           });
      };
      if (target_ptr && in_hypothesis(*target_ptr))
        SCRAM_THROW(
            ValidityError("Non-declarative substitution target event should "
                          "not appear in another substitution hypothesis."))
            << errinfo_element(origin.name(), "substitution");
      if (ext::any_of(origin.source(), in_hypothesis))
        SCRAM_THROW(
            ValidityError("Non-declarative substitution source event should "
                          "not appear in another substitution hypothesis."))
            << errinfo_element(origin.name(), "substitution");
    }
  }
}

void Initializer::EnsureNoCcfSubstitutions() {
  auto substitutions = model_->substitutions() |
                       boost::adaptors::filtered([](const auto& substitution) {
                         return !substitution.declarative();
                       });
  auto is_ccf = [](const Substitution& substitution) {
    if (ext::any_of(substitution.hypothesis().args(),
                    [](const Formula::Arg& arg) {
                      return std::get<BasicEvent*>(arg.event)->HasCcf();
                    }))
      return true;

    const auto* target_ptr = std::get_if<BasicEvent*>(&substitution.target());
    if (target_ptr && (*target_ptr)->HasCcf())
      return true;

    if (ext::any_of(substitution.source(), std::mem_fn(&BasicEvent::HasCcf)))
      return true;

    return false;
  };

  for (const Substitution& substitution : substitutions) {
    if (is_ccf(substitution))
      SCRAM_THROW(ValidityError("Non-declarative substitution '" +
                                substitution.name() +
                                "' events cannot be in a CCF group."));
  }
}

void Initializer::EnsureSubstitutionsWithApproximations() {
  if (settings_.approximation() != core::Approximation::kNone)
    return;

  if (ext::any_of(model_->substitutions(),
                  [](const Substitution& substitution) {
                    return !substitution.declarative();
                  }))
    SCRAM_THROW(ValidityError(
        "Non-declarative substitutions do not apply to exact analyses."));
}

void Initializer::ValidateExpressions() {
  // Check for cycles in parameters.
  // This must be done before expressions.
  cycle::CheckCycle<Parameter>(model_->table<Parameter>(), "parameter");

  // Validate expressions.
  for (const std::pair<Expression*, xml::Element>& expression : expressions_) {
    try {
      expression.first->Validate();
    } catch (ValidityError& err) {
      err << boost::errinfo_file_name(expression.second.filename())
          << boost::errinfo_at_line(expression.second.line());
      throw;
    }
  }

  // Validate CCF groups.
  for (const CcfGroup& group : model_->ccf_groups())
    group.Validate();

  // Check probability values for primary events.
  for (const BasicEvent& event : model_->basic_events()) {
    if (event.HasExpression())
      event.Validate();
  }
}

void Initializer::SetupForAnalysis() {
  {
    TIMER(DEBUG2, "Collecting top events of fault trees");
    for (Gate& gate : model_->table<Gate>())
      gate.mark(NodeMark::kClear);

    for (FaultTree& ft : model_->table<FaultTree>())
      ft.CollectTopEvents();
  }

  {
    TIMER(DEBUG2, "Applying CCF models");
    // CCF groups must apply models to basic event members.
    for (CcfGroup& group : model_->table<CcfGroup>())
      group.ApplyModel();
  }
}

}  // namespace scram::mef
