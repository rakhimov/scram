/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file initializer.cc
/// Implementation of input file processing into analysis constructs.

#include "initializer.h"

#include <sstream>
#include <type_traits>

#include <boost/filesystem.hpp>
#include <boost/range/algorithm.hpp>

#include "cycle.h"
#include "env.h"
#include "error.h"
#include "expression/arithmetic.h"
#include "expression/exponential.h"
#include "expression/random_deviate.h"
#include "logger.h"
#include "xml.h"

namespace scram {
namespace mef {

namespace {  // Helper function and wrappers for MEF initializations.

/// Maps string to the role specifier.
///
/// @param[in] s  Non-empty, valid role specifier string.
///
/// @returns Role specifier attribute for elements.
RoleSpecifier GetRole(const std::string& s) {
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
RoleSpecifier GetRole(const std::string& s, RoleSpecifier parent_role) {
  return s.empty() ? parent_role : GetRole(s);
}

/// Attaches attributes and a label to the elements of the analysis.
/// These attributes are not XML attributes
/// but the Open-PSA format defined arbitrary attributes
/// and a label that can be attached to many analysis elements.
///
/// @param[in] xml_element  XML element.
/// @param[out] element  The object that needs attributes and label.
void AttachLabelAndAttributes(const xmlpp::Element* xml_element,
                              Element* element) {
  xmlpp::NodeSet labels = xml_element->find("./label");
  if (!labels.empty()) {
    assert(labels.size() == 1);
    const xmlpp::Element* label = XmlElement(labels.front());
    const xmlpp::TextNode* text = label->get_child_text();
    assert(text);
    element->label(GetContent(text));
  }

  xmlpp::NodeSet attributes = xml_element->find("./attributes");
  if (attributes.empty())
    return;
  assert(attributes.size() == 1);  // Only one big element 'attributes'.
  const xmlpp::Element* attribute = nullptr;  // To report position.
  const xmlpp::Element* attributes_element = XmlElement(attributes.front());

  try {
    for (const xmlpp::Node* node : attributes_element->find("./attribute")) {
      attribute = XmlElement(node);
      Attribute attribute_struct = {GetAttributeValue(attribute, "name"),
                                    GetAttributeValue(attribute, "value"),
                                    GetAttributeValue(attribute, "type")};
      element->AddAttribute(std::move(attribute_struct));
    }
  } catch(ValidationError& err) {
    err.msg(GetLine(attribute) + err.msg());
    throw;
  }
}

/// Constructs Element of type T from an XML element.
template <class T>
std::enable_if_t<std::is_base_of<Element, T>::value, std::unique_ptr<T>>
ConstructElement(const xmlpp::Element* xml_element) {
  std::string name = GetAttributeValue(xml_element, "name");
  auto element = std::make_unique<T>(std::move(name));
  AttachLabelAndAttributes(xml_element, element.get());
  return element;
}

/// Constructs Element of type T with a role from an XML element.
template <class T>
std::enable_if_t<std::is_base_of<Role, T>::value, std::unique_ptr<T>>
ConstructElement(const xmlpp::Element* xml_element,
                 const std::string& base_path, RoleSpecifier base_role) {
  std::string name = GetAttributeValue(xml_element, "name");
  std::string role = GetAttributeValue(xml_element, "role");
  auto element =
      std::make_unique<T>(std::move(name), base_path, GetRole(role, base_role));
  AttachLabelAndAttributes(xml_element, element.get());
  return element;
}

/// Filters the data for MEF Element definitions.
///
/// @param[in] xml_element  The XML element with the construct definition.
///
/// @returns A set of XML child elements of MEF Element constructs.
xmlpp::NodeSet GetNonAttributeElements(const xmlpp::Element* xml_element) {
  return xml_element->find("./*[name() != 'attributes' and name() != 'label']");
}

}  // namespace

Initializer::Initializer(const std::vector<std::string>& xml_files,
                         core::Settings settings)
    : settings_(std::move(settings)) {
  ProcessInputFiles(xml_files);
}

void Initializer::CheckFileExistence(
    const std::vector<std::string>& xml_files) {
  for (auto& xml_file : xml_files) {
    if (boost::filesystem::exists(xml_file) == false)
      throw IOError("File doesn't exist: " + xml_file);
  }
}

void Initializer::CheckDuplicateFiles(
    const std::vector<std::string>& xml_files) {
  namespace fs = boost::filesystem;
  using Path = std::pair<fs::path, std::string>;  // Path mapping.
  // Collection of input file locations in canonical path.
  std::vector<Path> files;
  auto comparator = [](const Path& lhs, const Path& rhs) {
    return lhs.first < rhs.first;
  };

  for (auto& xml_file : xml_files)
    files.emplace_back(fs::canonical(xml_file), xml_file);

  auto it = boost::adjacent_find(
      boost::sort(files, comparator),  // NOLINT(build/include_what_you_use)
      [](const Path& lhs, const Path& rhs) { return lhs.first == rhs.first; });

  if (it != files.end()) {
    std::stringstream msg;
    msg << "Duplicate input files:\n";
    const Path& path = *it;
    auto it_end = std::upper_bound(it, files.end(), path, comparator);
    for (; it != it_end; ++it) {
      msg << "    " << it->second << "\n";
    }
    msg << "  POSIX Path: " << path.first.c_str();
    throw DuplicateArgumentError(msg.str());
  }
}

void Initializer::ProcessInputFiles(const std::vector<std::string>& xml_files) {
  CLOCK(input_time);
  LOG(DEBUG1) << "Processing input files";
  CheckFileExistence(xml_files);
  CheckDuplicateFiles(xml_files);
  std::vector<std::string>::const_iterator it;
  try {
    for (it = xml_files.begin(); it != xml_files.end(); ++it) {
      ProcessInputFile(*it);
    }
  } catch (ValidationError& err) {
    err.msg("In file '" + *it + "', " + err.msg());
    throw;
  }
  CLOCK(def_time);
  ProcessTbdElements();
  LOG(DEBUG2) << "Element definition time " << DUR(def_time);
  LOG(DEBUG1) << "Input files are processed in " << DUR(input_time);

  CLOCK(valid_time);
  LOG(DEBUG1) << "Validating the input files";
  // Check if the initialization is successful.
  ValidateInitialization();
  LOG(DEBUG1) << "Validation is finished in " << DUR(valid_time);

  CLOCK(setup_time);
  LOG(DEBUG1) << "Setting up for the analysis";
  // Perform setup for analysis using configurations from the input files.
  SetupForAnalysis();
  LOG(DEBUG1) << "Setup time " << DUR(setup_time);
}

template <class T>
void Initializer::Register(T&& element, const xmlpp::Element* xml_element) {
  try {
    model_->Add(std::forward<T>(element));
  } catch (ValidationError& err) {
    err.msg(GetLine(xml_element) + err.msg());
    throw;
  }
}

/// Specializations for element registrations.
/// @{
template <>
GatePtr Initializer::Register(const xmlpp::Element* gate_node,
                              const std::string& base_path,
                              RoleSpecifier container_role) {
  GatePtr gate = ConstructElement<Gate>(gate_node, base_path, container_role);
  Register(gate, gate_node);
  tbd_.emplace_back(gate.get(), gate_node);
  return gate;
}

template <>
BasicEventPtr Initializer::Register(const xmlpp::Element* event_node,
                                    const std::string& base_path,
                                    RoleSpecifier container_role) {
  BasicEventPtr basic_event =
      ConstructElement<BasicEvent>(event_node, base_path, container_role);
  Register(basic_event, event_node);
  tbd_.emplace_back(basic_event.get(), event_node);
  return basic_event;
}

template <>
HouseEventPtr Initializer::Register(const xmlpp::Element* event_node,
                                    const std::string& base_path,
                                    RoleSpecifier container_role) {
  HouseEventPtr house_event =
      ConstructElement<HouseEvent>(event_node, base_path, container_role);
  Register(house_event, event_node);

  // Only Boolean constant.
  xmlpp::NodeSet expression = event_node->find("./constant");
  if (!expression.empty()) {
    assert(expression.size() == 1);
    const xmlpp::Element* constant = XmlElement(expression.front());

    std::string val = GetAttributeValue(constant, "value");
    assert(val == "true" || val == "false");
    bool state = val == "true";
    house_event->state(state);
  }
  return house_event;
}

template <>
ParameterPtr Initializer::Register(const xmlpp::Element* param_node,
                                   const std::string& base_path,
                                   RoleSpecifier container_role) {
  ParameterPtr parameter =
      ConstructElement<Parameter>(param_node, base_path, container_role);
  Register(parameter, param_node);
  tbd_.emplace_back(parameter.get(), param_node);

  // Attach units.
  std::string unit = GetAttributeValue(param_node, "unit");
  if (!unit.empty()) {
    int pos = boost::find(kUnitsToString, unit) - std::begin(kUnitsToString);
    assert(pos < kNumUnits && "Unexpected unit kind.");
    parameter->unit(static_cast<Units>(pos));
  }
  return parameter;
}

template <>
CcfGroupPtr Initializer::Register(const xmlpp::Element* ccf_node,
                                  const std::string& base_path,
                                  RoleSpecifier container_role) {
  auto ccf_group = [&]() -> CcfGroupPtr {
    std::string model = GetAttributeValue(ccf_node, "model");
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

  Register(ccf_group, ccf_node);

  xmlpp::NodeSet members = ccf_node->find("./members");
  assert(members.size() == 1);
  ProcessCcfMembers(XmlElement(members[0]), ccf_group.get());

  tbd_.emplace_back(ccf_group.get(), ccf_node);
  return ccf_group;
}

template <>
SequencePtr Initializer::Register(const xmlpp::Element* xml_node,
                                  const std::string& /*base_path*/,
                                  RoleSpecifier /*container_role*/) {
  SequencePtr sequence = ConstructElement<Sequence>(xml_node);
  Register(sequence, xml_node);
  tbd_.emplace_back(sequence.get(), xml_node);
  return sequence;
}
/// @}

void Initializer::ProcessInputFile(const std::string& xml_file) {
  static xmlpp::RelaxNGValidator validator(Env::input_schema());

  std::unique_ptr<xmlpp::DomParser> parser = ConstructDomParser(xml_file);
  try {
    validator.validate(parser->get_document());
  } catch (const xmlpp::validity_error& err) {
    throw ValidationError("Document failed schema validation: " +
                          std::string(err.what()));
  }

  const xmlpp::Node* root = parser->get_document()->get_root_node();
  assert(root->get_name() == "opsa-mef");
  doc_to_file_.emplace(root, xml_file);  // Save for later.

  if (!model_) {  // Create only one model for multiple files.
    model_ = ConstructElement<Model>(XmlElement(root));
    model_->mission_time()->value(settings_.mission_time());
  }

  for (const xmlpp::Node* node : root->find("./define-event-tree")) {
    DefineEventTree(XmlElement(node));
  }

  for (const xmlpp::Node* node : root->find("./define-fault-tree")) {
    DefineFaultTree(XmlElement(node));
  }

  for (const xmlpp::Node* node : root->find("./define-CCF-group")) {
    Register<CcfGroup>(XmlElement(node), "", RoleSpecifier::kPublic);
  }

  for (const xmlpp::Node* node : root->find("./model-data")) {
    ProcessModelData(XmlElement(node));
  }
  parsers_.emplace_back(std::move(parser));
}

/// Specializations for elements defined after registration.
/// @{
template <>
void Initializer::Define(const xmlpp::Element* gate_node, Gate* gate) {
  xmlpp::NodeSet formulas = GetNonAttributeElements(gate_node);
  // Assumes that there are no attributes and labels.
  assert(formulas.size() == 1);
  const xmlpp::Element* formula_node = XmlElement(formulas.front());
  gate->formula(GetFormula(formula_node, gate->base_path()));
  try {
    gate->Validate();
  } catch (ValidationError& err) {
    err.msg(GetLine(gate_node) + err.msg());
    throw;
  }
}

template <>
void Initializer::Define(const xmlpp::Element* event_node,
                         BasicEvent* basic_event) {
  xmlpp::NodeSet expressions = GetNonAttributeElements(event_node);

  if (!expressions.empty()) {
    const xmlpp::Element* expr_node = XmlElement(expressions.back());
    basic_event->expression(GetExpression(expr_node, basic_event->base_path()));
  }
}

template <>
void Initializer::Define(const xmlpp::Element* param_node,
                         Parameter* parameter) {
  xmlpp::NodeSet expressions = GetNonAttributeElements(param_node);
  assert(expressions.size() == 1);
  const xmlpp::Element* expr_node = XmlElement(expressions.back());
  parameter->expression(GetExpression(expr_node, parameter->base_path()));
}

template <>
void Initializer::Define(const xmlpp::Element* ccf_node, CcfGroup* ccf_group) {
  for (const xmlpp::Node* node : ccf_node->find("./*")) {
    const xmlpp::Element* element = XmlElement(node);
    std::string name = element->get_name();
    if (name == "distribution") {
      assert(element->find("./*").size() == 1);
      const xmlpp::Element* expr_node = XmlElement(element->find("./*")[0]);
      ccf_group->AddDistribution(
          GetExpression(expr_node, ccf_group->base_path()));

    } else if (name == "factor") {
      DefineCcfFactor(element, ccf_group);

    } else if (name == "factors") {
      for (const xmlpp::Node* factor_node : element->find("./*")) {
        DefineCcfFactor(XmlElement(factor_node), ccf_group);
      }
    }
  }
}

template <>
void Initializer::Define(const xmlpp::Element* xml_node, Sequence* sequence) {
  xmlpp::NodeSet xml_instructions = GetNonAttributeElements(xml_node);
  InstructionContainer instructions;
  for (const xmlpp::Node* xml_instruction : xml_instructions) {
    instructions.emplace_back(GetInstruction(XmlElement(xml_instruction)));
  }
  sequence->instructions(std::move(instructions));
}
/// @}

void Initializer::ProcessTbdElements() {
  const xmlpp::Element* el_def;  // XML element with the definition for reports.
  try {
    for (const auto& tbd_element : tbd_) {
      el_def = tbd_element.second;
      boost::apply_visitor(
          [this, &el_def](auto* tbd_construct) {
            this->Define(el_def, tbd_construct);
          },
          tbd_element.first);
    }
  } catch (ValidationError& err) {
    const xmlpp::Node* root = el_def->find("/opsa-mef")[0];
    err.msg("In file '" + doc_to_file_.at(root) + "', " + err.msg());
    throw;
  }
}

void Initializer::DefineEventTree(const xmlpp::Element* et_node) {
  EventTreePtr event_tree = ConstructElement<EventTree>(et_node);
  for (const xmlpp::Node* node : et_node->find("./define-sequence")) {
    event_tree->Add(Register<Sequence>(XmlElement(node), event_tree->name(),
                                       RoleSpecifier::kPublic));
  }
  Register(std::move(event_tree), et_node);
}

void Initializer::DefineFaultTree(const xmlpp::Element* ft_node) {
  FaultTreePtr fault_tree = ConstructElement<FaultTree>(ft_node);
  RegisterFaultTreeData(ft_node, fault_tree->name(), fault_tree.get());
  Register(std::move(fault_tree), ft_node);
}

ComponentPtr Initializer::DefineComponent(const xmlpp::Element* component_node,
                                          const std::string& base_path,
                                          RoleSpecifier container_role) {
  ComponentPtr component =
      ConstructElement<Component>(component_node, base_path, container_role);
  RegisterFaultTreeData(component_node, base_path + "." + component->name(),
                        component.get());
  return component;
}

void Initializer::RegisterFaultTreeData(const xmlpp::Element* ft_node,
                                        const std::string& base_path,
                                        Component* component) {
  for (const xmlpp::Node* node : ft_node->find("./define-house-event")) {
    component->Add(
        Register<HouseEvent>(XmlElement(node), base_path, component->role()));
  }
  CLOCK(basic_time);
  for (const xmlpp::Node* node : ft_node->find("./define-basic-event")) {
    component->Add(
        Register<BasicEvent>(XmlElement(node), base_path, component->role()));
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (const xmlpp::Node* node : ft_node->find("./define-parameter")) {
    component->Add(
        Register<Parameter>(XmlElement(node), base_path, component->role()));
  }

  CLOCK(gate_time);
  for (const xmlpp::Node* node : ft_node->find("./define-gate")) {
    component->Add(
        Register<Gate>(XmlElement(node), base_path, component->role()));
  }
  LOG(DEBUG2) << "Gate registration time " << DUR(gate_time);
  for (const xmlpp::Node* node : ft_node->find("./define-CCF-group")) {
    component->Add(
        Register<CcfGroup>(XmlElement(node), base_path, component->role()));
  }
  for (const xmlpp::Node* node : ft_node->find("./define-component")) {
    ComponentPtr sub =
        DefineComponent(XmlElement(node), base_path, component->role());
    try {
      component->Add(std::move(sub));
    } catch (ValidationError& err) {
      err.msg(GetLine(node) + err.msg());
      throw;
    }
  }
}

void Initializer::ProcessModelData(const xmlpp::Element* model_data) {
  for (const xmlpp::Node* node : model_data->find("./define-house-event")) {
    Register<HouseEvent>(XmlElement(node), "", RoleSpecifier::kPublic);
  }
  CLOCK(basic_time);
  for (const xmlpp::Node* node : model_data->find("./define-basic-event")) {
    Register<BasicEvent>(XmlElement(node), "", RoleSpecifier::kPublic);
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (const xmlpp::Node* node : model_data->find("./define-parameter")) {
    Register<Parameter>(XmlElement(node), "", RoleSpecifier::kPublic);
  }
}

FormulaPtr Initializer::GetFormula(const xmlpp::Element* formula_node,
                                   const std::string& base_path) {
  Operator type = [&formula_node]() {
    if (formula_node->get_attribute("name"))
      return kNull;
    int pos = boost::find(kOperatorToString, formula_node->get_name()) -
              std::begin(kOperatorToString);
    assert(pos < kNumOperators && "Unexpected operator type.");
    return static_cast<Operator>(pos);
  }();
  FormulaPtr formula(new Formula(type));
  if (type == kVote) {
    formula->vote_number(CastAttributeValue<int>(formula_node, "min"));
  } else if (type == kNull) {  // Special case of pass-through.
    formula_node = formula_node->get_parent();
  }
  // Process arguments of this formula.
  ProcessFormula(formula_node, base_path, formula.get());

  try {
    formula->Validate();
  } catch (ValidationError& err) {
    err.msg(GetLine(formula_node) + err.msg());
    throw;
  }
  return formula;
}

void Initializer::ProcessFormula(const xmlpp::Element* formula_node,
                                 const std::string& base_path,
                                 Formula* formula) {
  for (const xmlpp::Node* node : formula_node->find("./*")) {
    const xmlpp::Element* element = XmlElement(node);
    std::string name = GetAttributeValue(element, "name");

    if (name.empty()) {
      formula->AddArgument(GetFormula(element, base_path));
      continue;
    }

    std::string element_type = element->get_name();
    // This is for the case "<event name="id" type="type"/>".
    std::string type = GetAttributeValue(element, "type");
    if (!type.empty()) {
      assert(type == "gate" || type == "basic-event" || type == "house-event");
      element_type = type;  // Event type is defined.
    }

    try {
      if (element_type == "event") {  // Undefined type yet.
        formula->AddArgument(model_->GetEvent(name, base_path));

      } else if (element_type == "gate") {
        formula->AddArgument(model_->GetGate(name, base_path));

      } else if (element_type == "basic-event") {
        formula->AddArgument(model_->GetBasicEvent(name, base_path));

      } else {
        assert(element_type == "house-event");
        formula->AddArgument(model_->GetHouseEvent(name, base_path));
      }
    } catch (std::out_of_range&) {
      throw ValidationError(GetLine(node) + "Undefined " + element_type + " " +
                            name + " with base path " + base_path);
    }
  }
}

InstructionPtr Initializer::GetInstruction(const xmlpp::Element* xml_element) {
  assert(xml_element->get_name() == "collect-expression");
  const xmlpp::Element* arg_element =
      XmlElement(xml_element->find("./*").front());
  return std::make_unique<CollectExpression>(GetExpression(arg_element, ""));
}

template <class T, int N>
struct Initializer::Extractor {
  /// Extracts and accumulates expressions
  /// to be passed to the constructor of expression T.
  ///
  /// @tparam Ts  Expression types.
  ///
  /// @param[in] args  A vector of XML elements containing the arguments.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] init  The host Initializer.
  /// @param[in] expressions  Accumulated argument expressions.
  ///
  /// @returns A shared pointer to the extracted expression.
  ///
  /// @throws std::out_of_range  Not enough arguments in the args container.
  template <class... Ts>
  std::unique_ptr<T> operator()(const xmlpp::NodeSet& args,
                                const std::string& base_path,
                                Initializer* init,
                                Ts&&... expressions) {
    static_assert(N > 0, "The number of arguments can't be fewer than 1.");
    return Extractor<T, N - 1>()(args, base_path, init,
                                 init->GetExpression(XmlElement(args.at(N - 1)),
                                                     base_path),
                                 std::forward<Ts>(expressions)...);
  }
};

/// Partial specialization for terminal Extractor.
template <class T>
struct Initializer::Extractor<T, 0> {
  /// Constructs the requested expression T
  /// with all accumulated argument expressions.
  ///
  /// @tparam Ts  Expression types.
  ///
  /// @param[in] expressions  All argument expressions for constructing T.
  ///
  /// @returns A shared pointer to the constructed expression.
  template <class... Ts>
  std::unique_ptr<T> operator()(const xmlpp::NodeSet& /*args*/,
                                const std::string& /*base_path*/,
                                Initializer* /*init*/,
                                Ts&&... expressions) {
    static_assert(sizeof...(Ts), "Unintended use case.");
    return std::make_unique<T>(std::forward<Ts>(expressions)...);
  }
};

/// Specialization of Extractor to extract all expressions into arg vector.
template <class T>
struct Initializer::Extractor<T, -1> {
  /// Constructs an expression with a variable number of arguments.
  ///
  /// @param[in] args  A vector of XML elements containing the arguments.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] init  The host Initializer.
  ///
  /// @returns A shared pointer to the constructed expression.
  std::unique_ptr<T> operator()(const xmlpp::NodeSet& args,
                                const std::string& base_path,
                                Initializer* init) {
    std::vector<Expression*> expr_args;
    for (const xmlpp::Node* node : args) {
      expr_args.push_back(init->GetExpression(XmlElement(node), base_path));
    }
    return std::make_unique<T>(std::move(expr_args));
  }
};

namespace {  // Expression extraction helper functions.

/// @returns The number of constructor arguments for Expression types.
/// @{
template <class T, class... As>
constexpr int count_args(std::true_type) {
  return sizeof...(As);
}

template <class T, class... As>
constexpr int count_args();

template <class T, class A, class... As>
constexpr int count_args(std::false_type) {
  return count_args<T, A, A, As...>();
}

template <class T, class... As>
constexpr int count_args() {
  return count_args<T, As...>(std::is_constructible<T, As...>());
}

template <class T>
constexpr int num_args(std::false_type) {
  return count_args<T, Expression*>();
}

template <class T>
constexpr int num_args(std::true_type) { return -1; }

template <class T>
constexpr std::enable_if_t<std::is_base_of<Expression, T>::value, int>
num_args() {
  static_assert(!std::is_default_constructible<T>::value, "No zero args.");
  return num_args<T>(std::is_constructible<T, std::vector<Expression*>>());
}
/// @}

}  // namespace

template <class T>
std::unique_ptr<Expression> Initializer::Extract(const xmlpp::NodeSet& args,
                                                 const std::string& base_path,
                                                 Initializer* init) {
  return Extractor<T, num_args<T>()>()(args, base_path, init);
}

/// Specialization for Extractor of Histogram expressions.
template <>
std::unique_ptr<Expression> Initializer::Extract<Histogram>(
    const xmlpp::NodeSet& args,
    const std::string& base_path,
    Initializer* init) {
  assert(args.size() > 1 && "At least one bin must be present.");
  std::vector<Expression*> boundaries = {
      init->GetExpression(XmlElement(args.front()), base_path)};
  std::vector<Expression*> weights;
  for (auto it = std::next(args.begin()); it != args.end(); ++it) {
    const xmlpp::Element* el = XmlElement(*it);
    xmlpp::NodeSet bin = el->find("./*");
    assert(bin.size() == 2);
    boundaries.push_back(init->GetExpression(XmlElement(bin[0]), base_path));
    weights.push_back(init->GetExpression(XmlElement(bin[1]), base_path));
  }
  return std::make_unique<Histogram>(std::move(boundaries), std::move(weights));
}

/// Specialization due to overloaded constructors.
template <>
std::unique_ptr<Expression> Initializer::Extract<LogNormalDeviate>(
    const xmlpp::NodeSet& args,
    const std::string& base_path,
    Initializer* init) {
  if (args.size() == 3)
    return Extractor<LogNormalDeviate, 3>()(args, base_path, init);
  return Extractor<LogNormalDeviate, 2>()(args, base_path, init);
}

/// Specialization due to overloaded constructors and un-fixed number of args.
template <>
std::unique_ptr<Expression> Initializer::Extract<PeriodicTest>(
    const xmlpp::NodeSet& args,
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
      throw InvalidArgument("Invalid number of arguments for Periodic Test.");
  }
}

const Initializer::ExtractorMap Initializer::kExpressionExtractors_ = {
    {"exponential", &Extract<ExponentialExpression>},
    {"GLM", &Extract<GlmExpression>},
    {"Weibull", &Extract<WeibullExpression>},
    {"periodic-test", &Extract<PeriodicTest>},
    {"uniform-deviate", &Extract<UniformDeviate>},
    {"normal-deviate", &Extract<NormalDeviate>},
    {"lognormal-deviate", &Extract<LogNormalDeviate>},
    {"gamma-deviate", &Extract<GammaDeviate>},
    {"beta-deviate", &Extract<BetaDeviate>},
    {"histogram", &Extract<Histogram>},
    {"neg", &Extract<Neg>},
    {"add", &Extract<Add>},
    {"sub", &Extract<Sub>},
    {"mul", &Extract<Mul>},
    {"div", &Extract<Div>}};

Expression* Initializer::GetExpression(const xmlpp::Element* expr_element,
                                       const std::string& base_path) {
  std::string expr_type = expr_element->get_name();
  auto register_expression = [this](std::unique_ptr<Expression> expression) {
    auto* ret_ptr = expression.get();
    model_->Add(std::move(expression));
    return ret_ptr;
  };
  if (expr_type == "int") {
    int val = CastAttributeValue<int>(expr_element, "value");
    return register_expression(std::make_unique<ConstantExpression>(val));
  }
  if (expr_type == "float") {
    double val = CastAttributeValue<double>(expr_element, "value");
    return register_expression(std::make_unique<ConstantExpression>(val));
  }
  if (expr_type == "bool") {
    std::string val = GetAttributeValue(expr_element, "value");
    return val == "true" ? &ConstantExpression::kOne
                         : &ConstantExpression::kZero;
  }
  if (expr_type == "pi")
    return &ConstantExpression::kPi;

  if (auto* expression = GetParameter(expr_type, expr_element, base_path))
    return expression;

  try {
    Expression* expression = register_expression(kExpressionExtractors_.at(
        expr_type)(expr_element->find("./*"), base_path, this));
    // Register for late validation after ensuring no cycles.
    expressions_.emplace_back(expression, expr_element);
    return expression;
  } catch (InvalidArgument& err) {
    throw ValidationError(GetLine(expr_element) + err.msg());
  }
}

Expression* Initializer::GetParameter(const std::string& expr_type,
                                      const xmlpp::Element* expr_element,
                                      const std::string& base_path) {
  auto check_units = [&expr_element](const auto& parameter) {
    std::string unit = GetAttributeValue(expr_element, "unit");
    const char* param_unit = scram::mef::kUnitsToString[parameter.unit()];
    if (!unit.empty() && unit != param_unit) {
      std::stringstream msg;
      msg << GetLine(expr_element)
          << "Parameter unit mismatch.\nExpected: " << param_unit
          << "\nGiven: " << unit;
      throw scram::ValidationError(msg.str());
    }
  };

  if (expr_type == "parameter") {
    std::string name = GetAttributeValue(expr_element, "name");
    try {
      Parameter* param = model_->GetParameter(name, base_path);
      param->unused(false);
      check_units(*param);
      return param;
    } catch (std::out_of_range&) {
      throw ValidationError(GetLine(expr_element) + "Undefined parameter " +
                            name + " with base path " + base_path);
    }
  } else if (expr_type == "system-mission-time") {
    check_units(*model_->mission_time());
    return model_->mission_time().get();
  }
  return nullptr;  // The expression is not a parameter.
}

void Initializer::ProcessCcfMembers(const xmlpp::Element* members_node,
                                    CcfGroup* ccf_group) {
  for (const xmlpp::Node* node : members_node->find("./*")) {
    const xmlpp::Element* event_node = XmlElement(node);
    assert("basic-event" == event_node->get_name());

    std::string name = GetAttributeValue(event_node, "name");
    auto basic_event = std::make_shared<BasicEvent>(std::move(name),
                                                    ccf_group->base_path(),
                                                    ccf_group->role());
    try {
      ccf_group->AddMember(basic_event);
    } catch (DuplicateArgumentError& err) {
      err.msg(GetLine(event_node) + err.msg());
      throw;
    }
    Register(basic_event, event_node);
  }
}

void Initializer::DefineCcfFactor(const xmlpp::Element* factor_node,
                                  CcfGroup* ccf_group) {
  assert(factor_node->find("./*").size() == 1);
  const xmlpp::Element* expr_node = XmlElement(factor_node->find("./*")[0]);
  Expression* expression = GetExpression(expr_node, ccf_group->base_path());

  try {
    if (GetAttributeValue(factor_node, "level").empty()) {
      ccf_group->AddFactor(expression);
    } else {
      ccf_group->AddFactor(expression,
                           CastAttributeValue<int>(factor_node, "level"));
    }
  } catch (ValidationError& err) {
    err.msg(GetLine(factor_node) + err.msg());
    throw;
  }
}

void Initializer::ValidateInitialization() {
  // Check if *all* gates have no cycles.
  for (const GatePtr& gate : model_->gates()) {
    std::vector<std::string> cycle;
    if (cycle::DetectCycle(gate.get(), &cycle)) {
      throw CycleError("Detected a cycle in " + gate->name() +
                       " gate:\n" + cycle::PrintCycle(cycle));
    }
  }

  // Keep node marks clean after use.
  for (const GatePtr& gate : model_->gates())
    gate->mark(NodeMark::kClear);

  // Check if all basic events have expressions for probability analysis.
  if (settings_.probability_analysis()) {
    std::string msg;
    for (const BasicEventPtr& event : model_->basic_events()) {
      if (event->HasExpression() == false)
        msg += event->name() + "\n";
    }

    if (!msg.empty())
      throw ValidationError("These basic events do not have expressions:\n" +
                            msg);
  }

  ValidateExpressions();
}

void Initializer::ValidateExpressions() {
  // Check for cycles in parameters.
  // This must be done before expressions.
  for (const ParameterPtr& param : model_->parameters()) {
    std::vector<std::string> cycle;
    if (cycle::DetectCycle(param.get(), &cycle)) {
      throw CycleError("Detected a cycle in " + param->name() +
                       " parameter:\n" + cycle::PrintCycle(cycle));
    }
  }

  for (const ParameterPtr& param : model_->parameters())
    param->mark(NodeMark::kClear);

  // Validate expressions.
  const xmlpp::Element* expr_element = nullptr;  // To report the position.
  try {
    for (const std::pair<Expression*, const xmlpp::Element*>& expression :
         expressions_) {
      expr_element = expression.second;
      expression.first->Validate();
    }
  } catch (InvalidArgument& err) {
    const xmlpp::Node* root = expr_element->find("/opsa-mef")[0];
    throw ValidationError("In file '" + doc_to_file_.at(root) + "', " +
                          GetLine(expr_element) + err.msg());
  }

  // Validate CCF groups.
  std::stringstream msg;
  for (const CcfGroupPtr& group : model_->ccf_groups()) {
    try {
      group->Validate();
    } catch (ValidationError& err) {
      msg << group->name() << " : " << err.msg() << "\n";
    }
  }
  if (!msg.str().empty()) {
    throw ValidationError("Invalid distributions for CCF groups detected:\n" +
                          msg.str());
  }

  // Check probability values for primary events.
  for (const BasicEventPtr& event : model_->basic_events()) {
    if (event->HasExpression() == false)
      continue;
    try {
      event->Validate();
    } catch (ValidationError& err) {
      msg << event->name() << " : " << err.msg() << "\n";
    }
  }
  if (!msg.str().empty()) {
    throw ValidationError("Invalid basic event probabilities detected:\n" +
                          msg.str());
  }
}

void Initializer::SetupForAnalysis() {
  CLOCK(top_time);
  LOG(DEBUG2) << "Collecting top events of fault trees...";
  for (const FaultTreePtr& ft : model_->fault_trees()) {
    ft->CollectTopEvents();
  }
  LOG(DEBUG2) << "Top event collection is finished in " << DUR(top_time);

  CLOCK(ccf_time);
  LOG(DEBUG2) << "Applying CCF models...";
  // CCF groups must apply models to basic event members.
  for (const CcfGroupPtr& group : model_->ccf_groups())
    group->ApplyModel();
  LOG(DEBUG2) << "Application of CCF models finished in " << DUR(ccf_time);
}

}  // namespace mef
}  // namespace scram
