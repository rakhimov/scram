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

namespace {

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

}  // namespace

Initializer::Initializer(const std::vector<std::string>& xml_files,
                         core::Settings settings)
    : settings_(std::move(settings)) {
  try {
    ProcessInputFiles(xml_files);
  } catch (const CycleError&) {
    BreakCycles();
    throw;
  }
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
    const xmlpp::Element* root_element = XmlElement(root);
    model_ = std::make_shared<Model>(GetAttributeValue(root_element, "name"));
    model_->mission_time()->value(settings_.mission_time());
    AttachLabelAndAttributes(root_element, model_.get());
  }

  for (const xmlpp::Node* node : root->find("./define-fault-tree")) {
    DefineFaultTree(XmlElement(node));
  }

  for (const xmlpp::Node* node : root->find("./define-CCF-group")) {
    RegisterCcfGroup(XmlElement(node), "", RoleSpecifier::kPublic);
  }

  for (const xmlpp::Node* node : root->find("./model-data")) {
    ProcessModelData(XmlElement(node));
  }
  parsers_.emplace_back(std::move(parser));
}

void Initializer::ProcessTbdElements() {
  // This element helps report errors.
  const xmlpp::Element* el_def;  // XML element with the definition.
  try {
    for (const std::pair<Parameter*, const xmlpp::Element*>& param :
         tbd_.parameters) {
      el_def = param.second;
      DefineParameter(el_def, param.first);
    }
    for (const std::pair<BasicEvent*, const xmlpp::Element*>& event :
         tbd_.basic_events) {
      el_def = event.second;
      DefineBasicEvent(el_def, event.first);
    }
    for (const std::pair<Gate*, const xmlpp::Element*>& gate : tbd_.gates) {
      el_def = gate.second;
      DefineGate(el_def, gate.first);
    }
    for (const std::pair<CcfGroup*, const xmlpp::Element*>& group :
         tbd_.ccf_groups) {
      el_def = group.second;
      DefineCcfGroup(el_def, group.first);
    }
  } catch (ValidationError& err) {
    const xmlpp::Node* root = el_def->find("/opsa-mef")[0];
    err.msg("In file '" + doc_to_file_.at(root) + "', " + err.msg());
    throw;
  }
}

void Initializer::AttachLabelAndAttributes(const xmlpp::Element* element_node,
                                           Element* element) {
  xmlpp::NodeSet labels = element_node->find("./label");
  if (!labels.empty()) {
    assert(labels.size() == 1);
    const xmlpp::Element* label = XmlElement(labels.front());
    const xmlpp::TextNode* text = label->get_child_text();
    assert(text);
    element->label(GetContent(text));
  }

  xmlpp::NodeSet attributes = element_node->find("./attributes");
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
    err.msg("Line " + std::to_string(attribute->get_line()) + ":\n" +
            err.msg());
    throw;
  }
}

void Initializer::DefineFaultTree(const xmlpp::Element* ft_node) {
  std::string name = GetAttributeValue(ft_node, "name");
  FaultTreePtr fault_tree(new FaultTree(name));
  RegisterFaultTreeData(ft_node, name, fault_tree.get());
  try {
    model_->AddFaultTree(std::move(fault_tree));
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << ft_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
}

ComponentPtr Initializer::DefineComponent(const xmlpp::Element* component_node,
                                          const std::string& base_path,
                                          RoleSpecifier container_role) {
  std::string name = GetAttributeValue(component_node, "name");
  std::string role = GetAttributeValue(component_node, "role");
  ComponentPtr component(new Component(name, base_path,
                                       GetRole(role, container_role)));
  RegisterFaultTreeData(component_node, base_path + "." + name,
                        component.get());
  return component;
}

void Initializer::RegisterFaultTreeData(const xmlpp::Element* ft_node,
                                        const std::string& base_path,
                                        Component* component) {
  AttachLabelAndAttributes(ft_node, component);

  for (const xmlpp::Node* node : ft_node->find("./define-house-event")) {
    component->AddHouseEvent(
        DefineHouseEvent(XmlElement(node), base_path, component->role()));
  }
  CLOCK(basic_time);
  for (const xmlpp::Node* node : ft_node->find("./define-basic-event")) {
    component->AddBasicEvent(
        RegisterBasicEvent(XmlElement(node), base_path, component->role()));
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (const xmlpp::Node* node : ft_node->find("./define-parameter")) {
    component->AddParameter(
        RegisterParameter(XmlElement(node), base_path, component->role()));
  }

  CLOCK(gate_time);
  for (const xmlpp::Node* node : ft_node->find("./define-gate")) {
    component->AddGate(
        RegisterGate(XmlElement(node), base_path, component->role()));
  }
  LOG(DEBUG2) << "Gate registration time " << DUR(gate_time);
  for (const xmlpp::Node* node : ft_node->find("./define-CCF-group")) {
    component->AddCcfGroup(
        RegisterCcfGroup(XmlElement(node), base_path, component->role()));
  }
  for (const xmlpp::Node* node : ft_node->find("./define-component")) {
    ComponentPtr sub =
        DefineComponent(XmlElement(node), base_path, component->role());
    try {
      component->AddComponent(std::move(sub));
    } catch (ValidationError& err) {
      std::stringstream msg;
      msg << "Line " << node->get_line() << ":\n";
      err.msg(msg.str() + err.msg());
      throw;
    }
  }
}

void Initializer::ProcessModelData(const xmlpp::Element* model_data) {
  for (const xmlpp::Node* node : model_data->find("./define-house-event")) {
    DefineHouseEvent(XmlElement(node), "", RoleSpecifier::kPublic);
  }
  CLOCK(basic_time);
  for (const xmlpp::Node* node : model_data->find("./define-basic-event")) {
    RegisterBasicEvent(XmlElement(node), "", RoleSpecifier::kPublic);
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (const xmlpp::Node* node : model_data->find("./define-parameter")) {
    RegisterParameter(XmlElement(node), "", RoleSpecifier::kPublic);
  }
}

GatePtr Initializer::RegisterGate(const xmlpp::Element* gate_node,
                                  const std::string& base_path,
                                  RoleSpecifier container_role) {
  std::string name = GetAttributeValue(gate_node, "name");
  std::string role = GetAttributeValue(gate_node, "role");
  auto gate = std::make_shared<Gate>(name, base_path,
                                     GetRole(role, container_role));
  try {
    model_->AddGate(gate);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << gate_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
  tbd_.gates.emplace_back(gate.get(), gate_node);
  AttachLabelAndAttributes(gate_node, gate.get());
  return gate;
}

void Initializer::DefineGate(const xmlpp::Element* gate_node, Gate* gate) {
  xmlpp::NodeSet formulas =
      gate_node->find("./*[name() != 'attributes' and name() != 'label']");
  // Assumes that there are no attributes and labels.
  assert(formulas.size() == 1);
  const xmlpp::Element* formula_node = XmlElement(formulas.front());
  gate->formula(GetFormula(formula_node, gate->base_path()));
  try {
    gate->Validate();
  } catch (ValidationError& err) {
    err.msg("Line " + std::to_string(gate_node->get_line()) + ":\n" +
            err.msg());
    throw;
  }
}

FormulaPtr Initializer::GetFormula(const xmlpp::Element* formula_node,
                                   const std::string& base_path) {
  std::string type = formula_node->get_name();
  if (type == "event" || type == "basic-event" || type == "gate" ||
      type == "house-event") {
    type = "null";
  }

  int pos =
      boost::find(kOperatorToString, type) - std::begin(kOperatorToString);
  assert(pos < kNumOperators && "Unexpected operator type.");

  FormulaPtr formula(new Formula(static_cast<Operator>(pos)));
  if (type == "atleast") {
    int vote_number = CastAttributeValue<int>(formula_node, "min");
    formula->vote_number(vote_number);
  }
  // Process arguments of this formula.
  if (type == "null") {  // Special case of pass-through.
    formula_node = formula_node->get_parent();
  }
  ProcessFormula(formula_node, base_path, formula.get());

  try {
    formula->Validate();
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << formula_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
  return formula;
}

void Initializer::ProcessFormula(const xmlpp::Element* formula_node,
                                 const std::string& base_path,
                                 Formula* formula) {
  xmlpp::NodeSet events = formula_node->find("./*[name() = 'event' or "
                                             "name() = 'gate' or "
                                             "name() = 'basic-event' or "
                                             "name() = 'house-event']");
  for (const xmlpp::Node* node : events) {
    const xmlpp::Element* event = XmlElement(node);
    std::string name = GetAttributeValue(event, "name");

    std::string element_type = event->get_name();
    // This is for the case "<event name="id" type="type"/>".
    std::string type = GetAttributeValue(event, "type");
    if (!type.empty()) {
      assert(type == "gate" || type == "basic-event" || type == "house-event");
      element_type = type;  // Event type is defined.
    }

    try {
      if (element_type == "event") {  // Undefined type yet.
        model_->BindEvent(name, base_path, formula);

      } else if (element_type == "gate") {
        formula->AddArgument(model_->GetGate(name, base_path));

      } else if (element_type == "basic-event") {
        formula->AddArgument(model_->GetBasicEvent(name, base_path));

      } else {
        assert(element_type == "house-event");
        formula->AddArgument(model_->GetHouseEvent(name, base_path));
      }
    } catch (std::out_of_range&) {
      std::stringstream msg;
      msg << "Line " << event->get_line() << ":\n"
          << "Undefined " << element_type << " " << name << " with base path "
          << base_path;
      throw ValidationError(msg.str());
    }
  }

  xmlpp::NodeSet formulas = formula_node->find("./*[name() != 'event' and "
                                               "name() != 'gate' and "
                                               "name() != 'basic-event' and "
                                               "name() != 'house-event']");
  for (const xmlpp::Node* node : formulas) {
    const xmlpp::Element* nested_formula = XmlElement(node);
    formula->AddArgument(GetFormula(nested_formula, base_path));
  }
}

BasicEventPtr Initializer::RegisterBasicEvent(const xmlpp::Element* event_node,
                                              const std::string& base_path,
                                              RoleSpecifier container_role) {
  std::string name = GetAttributeValue(event_node, "name");
  std::string role = GetAttributeValue(event_node, "role");
  auto basic_event = std::make_shared<BasicEvent>(
      name,
      base_path,
      GetRole(role, container_role));
  try {
    model_->AddBasicEvent(basic_event);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << event_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
  tbd_.basic_events.emplace_back(basic_event.get(), event_node);
  AttachLabelAndAttributes(event_node, basic_event.get());
  return basic_event;
}

void Initializer::DefineBasicEvent(const xmlpp::Element* event_node,
                                   BasicEvent* basic_event) {
  xmlpp::NodeSet expressions =
     event_node->find("./*[name() != 'attributes' and name() != 'label']");

  if (!expressions.empty()) {
    const xmlpp::Element* expr_node = XmlElement(expressions.back());
    ExpressionPtr expression =
        GetExpression(expr_node, basic_event->base_path());
    basic_event->expression(expression);
  }
}

HouseEventPtr Initializer::DefineHouseEvent(const xmlpp::Element* event_node,
                                            const std::string& base_path,
                                            RoleSpecifier container_role) {
  std::string name = GetAttributeValue(event_node, "name");
  std::string role = GetAttributeValue(event_node, "role");
  auto house_event = std::make_shared<HouseEvent>(
      name,
      base_path,
      GetRole(role, container_role));
  try {
    model_->AddHouseEvent(house_event);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << event_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }

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
  AttachLabelAndAttributes(event_node, house_event.get());
  return house_event;
}

ParameterPtr Initializer::RegisterParameter(const xmlpp::Element* param_node,
                                            const std::string& base_path,
                                            RoleSpecifier container_role) {
  std::string name = GetAttributeValue(param_node, "name");
  std::string role = GetAttributeValue(param_node, "role");
  auto parameter = std::make_shared<Parameter>(name, base_path,
                                               GetRole(role, container_role));
  try {
    model_->AddParameter(parameter);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << param_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
  tbd_.parameters.emplace_back(parameter.get(), param_node);

  // Attach units.
  std::string unit = GetAttributeValue(param_node, "unit");
  if (!unit.empty()) {
    int pos = boost::find(kUnitsToString, unit) - std::begin(kUnitsToString);
    assert(pos < kNumUnits && "Unexpected unit kind.");
    parameter->unit(static_cast<Units>(pos));
  }
  AttachLabelAndAttributes(param_node, parameter.get());
  return parameter;
}

void Initializer::DefineParameter(const xmlpp::Element* param_node,
                                  Parameter* parameter) {
  // Assuming that expression is the last child of the parameter definition.
  xmlpp::NodeSet expressions =
      param_node->find("./*[name() != 'attributes' and name() != 'label']");
  assert(expressions.size() == 1);
  const xmlpp::Element* expr_node = XmlElement(expressions.back());
  ExpressionPtr expression = GetExpression(expr_node, parameter->base_path());

  parameter->expression(expression);
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
  std::shared_ptr<T> operator()(const xmlpp::NodeSet& args,
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
  std::shared_ptr<T> operator()(const xmlpp::NodeSet& /*args*/,
                                const std::string& /*base_path*/,
                                Initializer* /*init*/,
                                Ts&&... expressions) {
    static_assert(sizeof...(Ts), "Unintended use case.");
    return std::make_shared<T>(std::forward<Ts>(expressions)...);
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
  std::shared_ptr<T> operator()(const xmlpp::NodeSet& args,
                                const std::string& base_path,
                                Initializer* init) {
    std::vector<ExpressionPtr> expr_args;
    for (const xmlpp::Node* node : args) {
      expr_args.push_back(init->GetExpression(XmlElement(node), base_path));
    }
    return std::make_shared<T>(std::move(expr_args));
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
  return count_args<T, ExpressionPtr>();
}

template <class T>
constexpr int num_args(std::true_type) { return -1; }

template <class T>
constexpr std::enable_if_t<std::is_base_of<Expression, T>::value, int>
num_args() {
  static_assert(!std::is_default_constructible<T>::value, "No zero args.");
  return num_args<T>(std::is_constructible<T, std::vector<ExpressionPtr>>());
}
/// @}

}  // namespace

template <class T>
ExpressionPtr Initializer::Extract(const xmlpp::NodeSet& args,
                                   const std::string& base_path,
                                   Initializer* init) {
  return Extractor<T, num_args<T>()>()(args, base_path, init);
}

/// Specialization for Extractor of Histogram expressions.
template <>
ExpressionPtr Initializer::Extract<Histogram>(const xmlpp::NodeSet& args,
                                              const std::string& base_path,
                                              Initializer* init) {
  assert(args.size() > 1 && "At least one bin must be present.");
  std::vector<ExpressionPtr> boundaries = {
      init->GetExpression(XmlElement(args.front()), base_path)};
  std::vector<ExpressionPtr> weights;
  for (auto it = std::next(args.begin()); it != args.end(); ++it) {
    const xmlpp::Element* el = XmlElement(*it);
    xmlpp::NodeSet bin = el->find("./*");
    assert(bin.size() == 2);
    boundaries.push_back(init->GetExpression(XmlElement(bin[0]), base_path));
    weights.push_back(init->GetExpression(XmlElement(bin[1]), base_path));
  }
  return std::make_shared<Histogram>(std::move(boundaries), std::move(weights));
}

/// Specialization due to overloaded constructors.
template <>
ExpressionPtr Initializer::Extract<LogNormalDeviate>(
    const xmlpp::NodeSet& args,
    const std::string& base_path,
    Initializer* init) {
  if (args.size() == 3)
    return Extractor<LogNormalDeviate, 3>()(args, base_path, init);
  return Extractor<LogNormalDeviate, 2>()(args, base_path, init);
}

/// Specialization due to overloaded constructors and un-fixed number of args.
template <>
ExpressionPtr Initializer::Extract<PeriodicTest>(
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

ExpressionPtr Initializer::GetExpression(const xmlpp::Element* expr_element,
                                         const std::string& base_path) {
  std::string expr_name = expr_element->get_name();
  if (expr_name == "int" || expr_name == "float" || expr_name == "bool")
    return GetConstantExpression(expr_element);

  if (expr_name == "parameter" || expr_name == "system-mission-time")
    return GetParameterExpression(expr_element, base_path);

  if (expr_name == "pi")
    return ConstantExpression::kPi;

  try {
    ExpressionPtr expression = kExpressionExtractors_.at(expr_name)(
        expr_element->find("./*"), base_path, this);
    // Register for late validation after ensuring no cycles.
    expressions_.emplace_back(expression.get(), expr_element);
    return expression;
  } catch (InvalidArgument& err) {
    std::stringstream msg;
    msg << "Line " << expr_element->get_line() << ":\n";
    throw ValidationError(msg.str() + err.msg());
  }
}

ExpressionPtr Initializer::GetConstantExpression(
    const xmlpp::Element* expr_element) {
  assert(expr_element);
  std::string expr_name = expr_element->get_name();
  if (expr_name == "int") {
    int val = CastAttributeValue<int>(expr_element, "value");
    return std::make_shared<ConstantExpression>(val);

  } else if (expr_name == "float") {
    double val = CastAttributeValue<double>(expr_element, "value");
    return std::make_shared<ConstantExpression>(val);

  } else {
    assert(expr_name == "bool");
    std::string val = GetAttributeValue(expr_element, "value");
    return val == "true" ? ConstantExpression::kOne : ConstantExpression::kZero;
  }
}

ExpressionPtr Initializer::GetParameterExpression(
    const xmlpp::Element* expr_element,
    const std::string& base_path) {
  assert(expr_element);
  std::string expr_name = expr_element->get_name();
  std::string param_unit;  // The expected unit.
  ExpressionPtr expression;
  if (expr_name == "parameter") {
    std::string name = GetAttributeValue(expr_element, "name");
    try {
      ParameterPtr param = model_->GetParameter(name, base_path);
      param->unused(false);
      param_unit = kUnitsToString[param->unit()];
      expression = param;
    } catch (std::out_of_range&) {
      std::stringstream msg;
      msg << "Line " << expr_element->get_line() << ":\n"
          << "Undefined parameter " << name << " with base path " << base_path;
      throw ValidationError(msg.str());
    }
  } else {
    assert(expr_name == "system-mission-time");
    param_unit = kUnitsToString[model_->mission_time()->unit()];
    expression = model_->mission_time();
  }
  // Check units.
  std::string unit = GetAttributeValue(expr_element, "unit");
  if (!unit.empty() && unit != param_unit) {
    std::stringstream msg;
    msg << "Line " << expr_element->get_line() << ":\n";
    msg << "Parameter unit mismatch.\nExpected: " << param_unit
        << "\nGiven: " << unit;
    throw ValidationError(msg.str());
  }
  return expression;
}

CcfGroupPtr Initializer::RegisterCcfGroup(const xmlpp::Element* ccf_node,
                                          const std::string& base_path,
                                          RoleSpecifier container_role) {
  std::string name = GetAttributeValue(ccf_node, "name");
  std::string model = GetAttributeValue(ccf_node, "model");
  assert(model == "beta-factor" || model == "alpha-factor" || model == "MGL" ||
         model == "phi-factor");

  CcfGroupPtr ccf_group;
  if (model == "beta-factor") {
    ccf_group =
        std::make_shared<BetaFactorModel>(name, base_path, container_role);

  } else if (model == "MGL") {
    ccf_group = std::make_shared<MglModel>(name, base_path, container_role);

  } else if (model == "alpha-factor") {
    ccf_group =
        std::make_shared<AlphaFactorModel>(name, base_path, container_role);

  } else if (model == "phi-factor") {
    ccf_group =
        std::make_shared<PhiFactorModel>(name, base_path, container_role);
  }

  try {
    model_->AddCcfGroup(ccf_group);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << ccf_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }

  xmlpp::NodeSet members = ccf_node->find("./members");
  assert(members.size() == 1);
  ProcessCcfMembers(XmlElement(members[0]), ccf_group.get());

  AttachLabelAndAttributes(ccf_node, ccf_group.get());

  tbd_.ccf_groups.emplace_back(ccf_group.get(), ccf_node);
  return ccf_group;
}

void Initializer::DefineCcfGroup(const xmlpp::Element* ccf_node,
                                 CcfGroup* ccf_group) {
  for (const xmlpp::Node* node : ccf_node->find("./*")) {
    const xmlpp::Element* element = XmlElement(node);
    std::string name = element->get_name();
    if (name == "distribution") {
      assert(element->find("./*").size() == 1);
      const xmlpp::Element* expr_node = XmlElement(element->find("./*")[0]);
      ExpressionPtr expression =
          GetExpression(expr_node, ccf_group->base_path());
      ccf_group->AddDistribution(expression);

    } else if (name == "factor") {
      DefineCcfFactor(element, ccf_group);

    } else if (name == "factors") {
      for (const xmlpp::Node* factor_node : element->find("./*")) {
        DefineCcfFactor(XmlElement(factor_node), ccf_group);
      }
    }
  }
}

void Initializer::ProcessCcfMembers(const xmlpp::Element* members_node,
                                    CcfGroup* ccf_group) {
  for (const xmlpp::Node* node : members_node->find("./*")) {
    const xmlpp::Element* event_node = XmlElement(node);
    assert("basic-event" == event_node->get_name());

    std::string name = GetAttributeValue(event_node, "name");
    auto basic_event = std::make_shared<BasicEvent>(name,
                                                    ccf_group->base_path(),
                                                    ccf_group->role());
    try {
      ccf_group->AddMember(basic_event);
      model_->AddBasicEvent(basic_event);
    } catch (DuplicateArgumentError& err) {
      std::stringstream msg;
      msg << "Line " << event_node->get_line() << ":\n";
      err.msg(msg.str() + err.msg());
      throw;
    }
  }
}

void Initializer::DefineCcfFactor(const xmlpp::Element* factor_node,
                                  CcfGroup* ccf_group) {
  // Checking the level for one factor input.
  std::string level = GetAttributeValue(factor_node, "level");
  if (level.empty()) {
    std::stringstream msg;
    msg << "Line " << factor_node->get_line() << ":\n";
    msg << "CCF group factor level number is not provided.";
    throw ValidationError(msg.str());
  }
  int level_num = CastAttributeValue<int>(factor_node, "level");
  assert(factor_node->find("./*").size() == 1);
  const xmlpp::Element* expr_node = XmlElement(factor_node->find("./*")[0]);
  ExpressionPtr expression = GetExpression(expr_node, ccf_group->base_path());
  try {
    ccf_group->AddFactor(expression, level_num);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << factor_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
}

void Initializer::ValidateInitialization() {
  // Check if *all* gates have no cycles.
  for (const GatePtr& gate : model_->gates()) {
    std::vector<std::string> cycle;
    if (cycle::DetectCycle(gate, &cycle)) {
      throw CycleError("Detected a cycle in " + gate->name() +
                       " gate:\n" + cycle::PrintCycle(cycle));
    }
  }

  // Keep node marks clean after use.
  for (const GatePtr& gate : model_->gates())
    gate->mark(NodeMark::kClear);

  // Check if all primary events have expressions for probability analysis.
  if (settings_.probability_analysis()) {
    std::string msg;
    for (const BasicEventPtr& event : model_->basic_events()) {
      if (!event->has_expression())
        msg += event->name() + "\n";
    }
    for (const HouseEventPtr& event : model_->house_events()) {
      if (!event->has_expression())
        msg += event->name() + "\n";
    }

    if (!msg.empty())
      throw ValidationError("These primary events do not have expressions:\n" +
                            msg);
  }

  ValidateExpressions();

  for (const CcfGroupPtr& group : model_->ccf_groups())
    group->Validate();
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
    throw ValidationError("In file '" + doc_to_file_.at(root) + "', Line " +
                          std::to_string(expr_element->get_line()) + ":\n" +
                          err.msg());
  }

  // Check distribution values for CCF groups.
  std::stringstream msg;
  for (const CcfGroupPtr& group : model_->ccf_groups()) {
    try {
      group->ValidateDistribution();
    } catch (ValidationError& err) {
      msg << group->name() << " : " << err.msg() << "\n";
    }
  }
  if (!msg.str().empty()) {
    std::string head = "Invalid distributions for CCF groups detected:\n";
    throw ValidationError(head + msg.str());
  }

  // Check probability values for primary events.
  for (const BasicEventPtr& event : model_->basic_events()) {
    if (event->has_expression() == false)
      continue;
    try {
      event->Validate();
    } catch (ValidationError& err) {
      msg << event->name() << " : " << err.msg() << "\n";
    }
  }
  if (!msg.str().empty()) {
    std::string head = "Invalid basic event probabilities detected:\n";
    throw ValidationError(head + msg.str());
  }
}

void Initializer::BreakCycles() {
  std::vector<std::weak_ptr<Gate>> cyclic_gates;
  for (const GatePtr& gate : model_->gates())
    cyclic_gates.emplace_back(gate);

  std::vector<std::weak_ptr<Parameter>> cyclic_parameters;
  for (const ParameterPtr& parameter : model_->parameters())
    cyclic_parameters.emplace_back(parameter);

  model_.reset();

  for (const auto& gate : cyclic_gates) {
    if (gate.expired())
      continue;
    Gate::Cycle::BreakConnections(gate.lock().get());
  }

  for (const auto& parameter : cyclic_parameters) {
    if (parameter.expired())
      continue;
    Parameter::Cycle::BreakConnections(parameter.lock().get());
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
