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

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/range/algorithm.hpp>

#include "cycle.h"
#include "env.h"
#include "error.h"
#include "logger.h"

namespace scram {
namespace mef {

const std::map<std::string, Units> Initializer::kUnits_ = {
    {"bool", kBool},
    {"int", kInt},
    {"float", kFloat},
    {"hours", kHours},
    {"hours-1", kInverseHours},
    {"years", kYears},
    {"years-1", kInverseYears},
    {"fit", kFit},
    {"demands", kDemands}};

const char* const Initializer::kUnitToString_[] = {"unitless", "bool", "int",
                                                   "float", "hours", "hours-1",
                                                   "years", "years-1", "fit",
                                                   "demands"};

std::stringstream Initializer::schema_;

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

Initializer::Initializer(const core::Settings& settings)
    : settings_(settings),
      mission_time_(std::make_shared<MissionTime>()) {
  mission_time_->mission_time(settings_.mission_time());
  if (schema_.str().empty()) {
    std::string schema_path = Env::input_schema();
    std::ifstream schema_stream(schema_path.c_str());
    schema_ << schema_stream.rdbuf();
    schema_stream.close();
  }
}

void Initializer::ProcessInputFiles(const std::vector<std::string>& xml_files) {
  CLOCK(input_time);
  LOG(DEBUG1) << "Processing input files";
  Initializer::CheckFileExistence(xml_files);
  Initializer::CheckDuplicateFiles(xml_files);
  std::vector<std::string>::const_iterator it;
  try {
    for (it = xml_files.begin(); it != xml_files.end(); ++it) {
      Initializer::ProcessInputFile(*it);
    }
  } catch (ValidationError& err) {
    err.msg("In file '" + *it + "', " + err.msg());
    throw;
  }
  CLOCK(def_time);
  Initializer::ProcessTbdElements();
  LOG(DEBUG2) << "Element definition time " << DUR(def_time);
  LOG(DEBUG1) << "Input files are processed in " << DUR(input_time);

  CLOCK(valid_time);
  LOG(DEBUG1) << "Validating the input files";
  // Check if the initialization is successful.
  Initializer::ValidateInitialization();
  LOG(DEBUG1) << "Validation is finished in " << DUR(valid_time);

  CLOCK(setup_time);
  LOG(DEBUG1) << "Setting up for the analysis";
  // Perform setup for analysis using configurations from the input files.
  Initializer::SetupForAnalysis();
  LOG(DEBUG1) << "Setup time " << DUR(setup_time);
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
  auto Comparator = [](const Path& lhs, const Path& rhs) {
    return lhs.first < rhs.first;
  };

  for (auto& xml_file : xml_files)
    files.emplace_back(fs::canonical(xml_file), xml_file);

  auto it = boost::adjacent_find(
      boost::sort(files, Comparator),  // NOLINT(build/include_what_you_use)
      [](const Path& lhs, const Path& rhs) { return lhs.first == rhs.first; });

  if (it != files.end()) {
    std::stringstream msg;
    msg << "Duplicate input files:\n";
    const Path& path = *it;
    auto it_end = std::upper_bound(it, files.end(), path, Comparator);
    for (; it != it_end; ++it) {
      msg << "    " << it->second << "\n";
    }
    msg << "  POSIX Path: " << path.first.c_str();
    throw DuplicateArgumentError(msg.str());
  }
}

void Initializer::ProcessInputFile(const std::string& xml_file) {
  std::ifstream file_stream(xml_file.c_str());
  if (!file_stream) throw IOError("'" + xml_file + "' could not be loaded.");

  std::stringstream stream;
  stream << file_stream.rdbuf();
  file_stream.close();

  XmlParser* parser = new XmlParser(stream);
  parsers_.emplace_back(parser);  // Gets managed by unique pointer.
  parser->Validate(schema_);

  const xmlpp::Document* doc = parser->Document();
  const xmlpp::Node* root = doc->get_root_node();
  assert(root->get_name() == "opsa-mef");
  doc_to_file_.emplace(root, xml_file);  // Save for later.

  if (!model_) {  // Create only one model for multiple files.
    const xmlpp::Element* root_element = XmlElement(root);
    model_ = std::make_shared<Model>(GetAttributeValue(root_element, "name"));
    Initializer::AttachLabelAndAttributes(root_element, model_.get());
  }

  for (const xmlpp::Node* node : root->find("./define-fault-tree")) {
    Initializer::DefineFaultTree(XmlElement(node));
  }

  for (const xmlpp::Node* node : root->find("./define-CCF-group")) {
    Initializer::RegisterCcfGroup(XmlElement(node), "", RoleSpecifier::kPublic);
  }

  for (const xmlpp::Node* node : root->find("./model-data")) {
    Initializer::ProcessModelData(XmlElement(node));
  }
}

void Initializer::ProcessTbdElements() {
  // This element helps report errors.
  const xmlpp::Element* el_def;  // XML element with the definition.
  try {
    for (const std::pair<Parameter*, const xmlpp::Element*>& param :
         tbd_.parameters) {
      el_def = param.second;
      Initializer::DefineParameter(el_def, param.first);
    }
    for (const std::pair<BasicEvent*, const xmlpp::Element*>& event :
         tbd_.basic_events) {
      el_def = event.second;
      Initializer::DefineBasicEvent(el_def, event.first);
    }
    for (const std::pair<Gate*, const xmlpp::Element*>& gate : tbd_.gates) {
      el_def = gate.second;
      Initializer::DefineGate(el_def, gate.first);
    }
    for (const std::pair<CcfGroup*, const xmlpp::Element*>& group :
         tbd_.ccf_groups) {
      el_def = group.second;
      Initializer::DefineCcfGroup(el_def, group.first);
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
    element->label(text->get_content());
  }

  xmlpp::NodeSet attributes = element_node->find("./attributes");
  if (!attributes.empty()) {
    assert(attributes.size() == 1);  // Only one big element 'attributes'.
    const xmlpp::Element* attributes_element = XmlElement(attributes.front());
    for (const xmlpp::Node* node : attributes_element->find("./attribute")) {
      const xmlpp::Element* attribute = XmlElement(node);
      Attribute attr = {GetAttributeValue(attribute, "name"),
                        GetAttributeValue(attribute, "value"),
                        GetAttributeValue(attribute, "type")};
      element->AddAttribute(attr);
    }
  }
}

void Initializer::DefineFaultTree(const xmlpp::Element* ft_node) {
  std::string name = GetAttributeValue(ft_node, "name");
  FaultTreePtr fault_tree(new FaultTree(name));
  Initializer::RegisterFaultTreeData(ft_node, name, fault_tree.get());
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
  Initializer::RegisterFaultTreeData(component_node, base_path + "." + name,
                                     component.get());
  return component;
}

void Initializer::RegisterFaultTreeData(const xmlpp::Element* ft_node,
                                        const std::string& base_path,
                                        Component* component) {
  Initializer::AttachLabelAndAttributes(ft_node, component);

  for (const xmlpp::Node* node : ft_node->find("./define-house-event")) {
    component->AddHouseEvent(
        Initializer::DefineHouseEvent(XmlElement(node), base_path,
                                      component->role()));
  }
  CLOCK(basic_time);
  for (const xmlpp::Node* node : ft_node->find("./define-basic-event")) {
    component->AddBasicEvent(
        Initializer::RegisterBasicEvent(XmlElement(node), base_path,
                                        component->role()));
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (const xmlpp::Node* node : ft_node->find("./define-parameter")) {
    component->AddParameter(
        Initializer::RegisterParameter(XmlElement(node), base_path,
                                       component->role()));
  }

  CLOCK(gate_time);
  for (const xmlpp::Node* node : ft_node->find("./define-gate")) {
    component->AddGate(Initializer::RegisterGate(XmlElement(node), base_path,
                                                 component->role()));
  }
  LOG(DEBUG2) << "Gate registration time " << DUR(gate_time);
  for (const xmlpp::Node* node : ft_node->find("./define-CCF-group")) {
    component->AddCcfGroup(
        Initializer::RegisterCcfGroup(XmlElement(node), base_path,
                                      component->role()));
  }
  for (const xmlpp::Node* node : ft_node->find("./define-component")) {
    ComponentPtr sub = Initializer::DefineComponent(XmlElement(node), base_path,
                                                    component->role());
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
    Initializer::DefineHouseEvent(XmlElement(node), "", RoleSpecifier::kPublic);
  }
  CLOCK(basic_time);
  for (const xmlpp::Node* node : model_data->find("./define-basic-event")) {
    Initializer::RegisterBasicEvent(XmlElement(node), "",
                                    RoleSpecifier::kPublic);
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (const xmlpp::Node* node : model_data->find("./define-parameter")) {
    Initializer::RegisterParameter(XmlElement(node), "",
                                   RoleSpecifier::kPublic);
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
  Initializer::AttachLabelAndAttributes(gate_node, gate.get());
  return gate;
}

void Initializer::DefineGate(const xmlpp::Element* gate_node, Gate* gate) {
  xmlpp::NodeSet formulas =
      gate_node->find("./*[name() != 'attributes' and name() != 'label']");
  // Assumes that there are no attributes and labels.
  assert(formulas.size() == 1);
  const xmlpp::Element* formula_node = XmlElement(formulas.front());
  gate->formula(Initializer::GetFormula(formula_node, gate->base_path()));
  try {
    gate->Validate();
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << gate_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
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
  FormulaPtr formula(new Formula(type));
  if (type == "atleast") {
    int vote_number = CastAttributeValue<int>(formula_node, "min");
    formula->vote_number(vote_number);
  }
  // Process arguments of this formula.
  if (type == "null") {  // Special case of pass-through.
    formula_node = formula_node->get_parent();
  }
  Initializer::ProcessFormula(formula_node, base_path, formula.get());

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
        try {  // Let's play some baseball.
          try {
            formula->AddArgument(model_->GetBasicEvent(name, base_path));
          } catch (std::out_of_range&) {
            formula->AddArgument(model_->GetGate(name, base_path));
          }
        } catch (std::out_of_range&) {
          formula->AddArgument(model_->GetHouseEvent(name, base_path));
        }
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
    formula->AddArgument(Initializer::GetFormula(nested_formula, base_path));
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
  Initializer::AttachLabelAndAttributes(event_node, basic_event.get());
  return basic_event;
}

void Initializer::DefineBasicEvent(const xmlpp::Element* event_node,
                                   BasicEvent* basic_event) {
  xmlpp::NodeSet expressions =
     event_node->find("./*[name() != 'attributes' and name() != 'label']");

  if (!expressions.empty()) {
    const xmlpp::Element* expr_node = XmlElement(expressions.back());
    ExpressionPtr expression =
        Initializer::GetExpression(expr_node, basic_event->base_path());
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
  Initializer::AttachLabelAndAttributes(event_node, house_event.get());
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
    assert(kUnits_.count(unit));
    parameter->unit(kUnits_.at(unit));
  }
  Initializer::AttachLabelAndAttributes(param_node, parameter.get());
  return parameter;
}

void Initializer::DefineParameter(const xmlpp::Element* param_node,
                                  Parameter* parameter) {
  // Assuming that expression is the last child of the parameter definition.
  xmlpp::NodeSet expressions =
      param_node->find("./*[name() != 'attributes' and name() != 'label']");
  assert(expressions.size() == 1);
  const xmlpp::Element* expr_node = XmlElement(expressions.back());
  ExpressionPtr expression =
      Initializer::GetExpression(expr_node, parameter->base_path());

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

/// Full specialization for Extractor of Histogram expressions.
template <>
struct Initializer::Extractor<Histogram, -1> {
  /// Constructs Histogram deviate expression
  /// expression arguments in XML elements.
  ///
  /// @param[in] args  A vector of XML elements containing the arguments.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] init  The host Initializer.
  ///
  /// @returns A shared pointer to the constructed Histogram expression.
  std::shared_ptr<Histogram> operator()(const xmlpp::NodeSet& args,
                                        const std::string& base_path,
                                        Initializer* init) {
    std::vector<ExpressionPtr> boundaries = {ConstantExpression::kZero};
    std::vector<ExpressionPtr> weights;
    for (const xmlpp::Node* node : args) {
      const xmlpp::Element* el = XmlElement(node);
      xmlpp::NodeSet pair = el->find("./*");
      assert(pair.size() == 2);
      boundaries.push_back(init->GetExpression(XmlElement(pair[0]), base_path));
      weights.push_back(init->GetExpression(XmlElement(pair[1]), base_path));
    }
    return std::make_shared<Histogram>(std::move(boundaries),
                                       std::move(weights));
  }
};

const Initializer::ExtractorMap Initializer::kExpressionExtractors_ = {
    {"exponential", Extractor<ExponentialExpression, 2>()},
    {"GLM", Extractor<GlmExpression, 4>()},
    {"Weibull", Extractor<WeibullExpression, 4>()},
    {"uniform-deviate", Extractor<UniformDeviate, 2>()},
    {"normal-deviate", Extractor<NormalDeviate, 2>()},
    {"lognormal-deviate", Extractor<LogNormalDeviate, 3>()},
    {"gamma-deviate", Extractor<GammaDeviate, 2>()},
    {"beta-deviate", Extractor<BetaDeviate, 2>()},
    {"histogram", Extractor<Histogram, -1>()},
    {"neg", Extractor<Neg, 1>()},
    {"add", Extractor<Add, -1>()},
    {"sub", Extractor<Sub, -1>()},
    {"mul", Extractor<Mul, -1>()},
    {"div", Extractor<Div, -1>()}};

ExpressionPtr Initializer::GetExpression(const xmlpp::Element* expr_element,
                                         const std::string& base_path) {
  std::string expr_name = expr_element->get_name();
  if (expr_name == "int" || expr_name == "float" || expr_name == "bool")
    return Initializer::GetConstantExpression(expr_element);

  if (expr_name == "parameter" || expr_name == "system-mission-time")
    return Initializer::GetParameterExpression(expr_element, base_path);

  if (expr_name == "pi") return ConstantExpression::kPi;

  try {
    ExpressionPtr expression = kExpressionExtractors_.at(expr_name)(
        expr_element->find("./*"), base_path, this);
    expressions_.push_back(expression.get());  // For late validation.
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
      param_unit = kUnitToString_[param->unit()];
      expression = param;
    } catch (std::out_of_range&) {
      std::stringstream msg;
      msg << "Line " << expr_element->get_line() << ":\n"
          << "Undefined parameter " << name << " with base path " << base_path;
      throw ValidationError(msg.str());
    }
  } else {
    assert(expr_name == "system-mission-time");
    param_unit = kUnitToString_[mission_time_->unit()];
    expression = mission_time_;
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
  Initializer::ProcessCcfMembers(XmlElement(members[0]), ccf_group.get());

  Initializer::AttachLabelAndAttributes(ccf_node, ccf_group.get());

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
          Initializer::GetExpression(expr_node, ccf_group->base_path());
      ccf_group->AddDistribution(expression);

    } else if (name == "factor") {
      Initializer::DefineCcfFactor(element, ccf_group);

    } else if (name == "factors") {
      for (const xmlpp::Node* factor_node : element->find("./*")) {
        Initializer::DefineCcfFactor(XmlElement(factor_node), ccf_group);
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
  ExpressionPtr expression =
      Initializer::GetExpression(expr_node, ccf_group->base_path());
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
  // Check if all gates have no cycles.
  for (const std::pair<const std::string, GatePtr>& gate : model_->gates()) {
    std::vector<std::string> cycle;
    if (cycle::DetectCycle(gate.second, &cycle)) {
      std::string msg =
          "Detected a cycle in " + gate.second->name() + " gate:\n";
      msg += cycle::PrintCycle(cycle);
      throw ValidationError(msg);
    }
  }
  // Check if all primary events have expressions for probability analysis.
  if (settings_.probability_analysis()) {
    std::string msg;
    for (const std::pair<const std::string, BasicEventPtr>& event :
         model_->basic_events()) {
      if (!event.second->has_expression()) msg += event.second->name() + "\n";
    }
    for (const std::pair<const std::string, HouseEventPtr>& event :
         model_->house_events()) {
      if (!event.second->has_expression()) msg += event.second->name() + "\n";
    }

    if (!msg.empty())
      throw ValidationError("These primary events do not have expressions:\n" +
                            msg);
  }

  Initializer::ValidateExpressions();

  for (const std::pair<const std::string, CcfGroupPtr>& group :
       model_->ccf_groups()) {
    group.second->Validate();
  }
}

void Initializer::ValidateExpressions() {
  // Check for cycles in parameters. This must be done before expressions.
  for (const std::pair<const std::string, ParameterPtr>& p :
       model_->parameters()) {
    std::vector<std::string> cycle;
    if (cycle::DetectCycle(p.second.get(), &cycle)) {
      throw ValidationError("Detected a cycle in " + p.second->name() +
                            " parameter:\n" + cycle::PrintCycle(cycle));
    }
  }

  // Validate expressions.
  try {
    for (Expression* expression : expressions_) expression->Validate();
  } catch (InvalidArgument& err) {
    throw ValidationError(err.msg());
  }

  // Check distribution values for CCF groups.
  std::stringstream msg;
  for (const std::pair<const std::string, CcfGroupPtr>& group :
        model_->ccf_groups()) {
    try {
      group.second->ValidateDistribution();
    } catch (ValidationError& err) {
      msg << group.second->name() << " : " << err.msg() << "\n";
    }
  }
  if (!msg.str().empty()) {
    std::string head = "Invalid distributions for CCF groups detected:\n";
    throw ValidationError(head + msg.str());
  }

  // Check probability values for primary events.
  for (const std::pair<const std::string, BasicEventPtr>& event :
        model_->basic_events()) {
    if (event.second->has_expression() == false) continue;
    try {
      event.second->Validate();
    } catch (ValidationError& err) {
      msg << event.second->name() << " : " << err.msg() << "\n";
    }
  }
  if (!msg.str().empty()) {
    std::string head = "Invalid basic event probabilities detected:\n";
    throw ValidationError(head + msg.str());
  }
}

void Initializer::SetupForAnalysis() {
  CLOCK(top_time);
  LOG(DEBUG2) << "Collecting top events of fault trees...";
  for (const std::pair<const std::string, FaultTreePtr>& ft :
       model_->fault_trees()) {
    ft.second->CollectTopEvents();
  }
  LOG(DEBUG2) << "Top event collection is finished in " << DUR(top_time);

  CLOCK(ccf_time);
  LOG(DEBUG2) << "Applying CCF models...";
  // CCF groups must apply models to basic event members.
  for (const std::pair<const std::string, CcfGroupPtr>& group :
       model_->ccf_groups()) {
    group.second->ApplyModel();
  }
  LOG(DEBUG2) << "Application of CCF models finished in " << DUR(ccf_time);
}

}  // namespace mef
}  // namespace scram
