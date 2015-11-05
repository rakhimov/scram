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

/// @file initializer.cc
/// Implementation of input file processing into analysis constructs.

#include "initializer.h"

#include <fstream>
#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "ccf_group.h"
#include "cycle.h"
#include "element.h"
#include "env.h"
#include "error.h"
#include "expression.h"
#include "fault_tree.h"
#include "logger.h"

namespace fs = boost::filesystem;

namespace scram {

namespace {

/// Helper function to staticly cast to XML element.
///
/// @param[in] node  XML node known to be XML element.
///
/// @returns XML element cast from the XML node.
///
/// @warning The node must be an XML element.
inline const xmlpp::Element* XmlElement(const xmlpp::Node* node) {
  return static_cast<const xmlpp::Element*>(node);
}

/// Normalizes the string in the XML attribute.
///
/// @param[in] element  XML element with the attribute.
///
/// @returns Normalized (trimmed) string from the attribute.
inline std::string GetAttributeValue(const xmlpp::Element* element,
                                     const std::string& attribute) {
  std::string value = element->get_attribute_value(attribute);
  boost::trim(value);
  return value;
}

}  // namespace

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

Initializer::Initializer(const Settings& settings)
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

void Initializer::ProcessInputFile(const std::string& xml_file) {
  std::ifstream file_stream(xml_file.c_str());
  if (!file_stream) {
    throw IOError("File '" + xml_file + "' could not be loaded.");
  }

  // Collection of input file locations in canonical path.
  std::set<std::string> input_paths;
  fs::path file_path = fs::canonical(xml_file);
  if (input_paths.count(file_path.native())) {
    throw ValidationError("Trying to pass the same file twice: " +
                          file_path.native());
  }
  input_paths.insert(file_path.native());

  std::stringstream stream;
  stream << file_stream.rdbuf();
  file_stream.close();

  XmlParser* parser = new XmlParser(stream);
  parsers_.emplace_back(parser);
  parser->Validate(schema_);

  const xmlpp::Document* doc = parser->Document();
  const xmlpp::Node* root = doc->get_root_node();
  assert(root->get_name() == "opsa-mef");
  doc_to_file_.emplace(root, xml_file);  // Save for later.

  if (!model_) {  // Create only one model for multiple files.
    const xmlpp::Element* root_element = XmlElement(root);
    std::string model_name = GetAttributeValue(root_element, "name");
    model_ = std::make_shared<Model>(model_name);
    Initializer::AttachLabelAndAttributes(root_element, model_.get());
  }

  for (const xmlpp::Node* node : root->find("./define-fault-tree")) {
    Initializer::DefineFaultTree(XmlElement(node));
  }

  for (const xmlpp::Node* node : root->find("./define-CCF-group")) {
    Initializer::RegisterCcfGroup(XmlElement(node));
  }

  for (const xmlpp::Node* node : root->find("./model-data")) {
    Initializer::ProcessModelData(XmlElement(node));
  }
}

void Initializer::ProcessTbdElements() {
  // This element helps report errors.
  const xmlpp::Element* el_def;  // XML element with the definition.
  try {
    for (const std::pair<ParameterPtr, const xmlpp::Element*>& param :
         tbd_.parameters) {
      el_def = param.second;
      Initializer::DefineParameter(el_def, param.first);
    }
    for (const std::pair<BasicEventPtr, const xmlpp::Element*>& event :
         tbd_.basic_events) {
      el_def = event.second;
      Initializer::DefineBasicEvent(el_def, event.first);
    }
    for (const std::pair<GatePtr, const xmlpp::Element*>& gate : tbd_.gates) {
      el_def = gate.second;
      Initializer::DefineGate(el_def, gate.first);
    }
    for (const std::pair<CcfGroupPtr, const xmlpp::Element*>& group :
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
  assert(!name.empty());
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

std::unique_ptr<Component> Initializer::DefineComponent(
    const xmlpp::Element* component_node,
    const std::string& base_path,
    bool public_container) {
  std::string name = GetAttributeValue(component_node, "name");
  assert(!name.empty());
  std::string role = GetAttributeValue(component_node, "role");
  bool component_role = public_container;  // Inherited role by default.
  // Overwrite the role explicitly.
  if (role != "") component_role = role == "public" ? true : false;
  ComponentPtr component(new Component(name, base_path, component_role));
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
                                      component->is_public()));
  }
  CLOCK(basic_time);
  for (const xmlpp::Node* node : ft_node->find("./define-basic-event")) {
    component->AddBasicEvent(
        Initializer::RegisterBasicEvent(XmlElement(node), base_path,
                                        component->is_public()));
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (const xmlpp::Node* node : ft_node->find("./define-parameter")) {
    component->AddParameter(
        Initializer::RegisterParameter(XmlElement(node), base_path,
                                       component->is_public()));
  }

  CLOCK(gate_time);
  for (const xmlpp::Node* node : ft_node->find("./define-gate")) {
    component->AddGate(Initializer::RegisterGate(XmlElement(node), base_path,
                                                 component->is_public()));
  }
  LOG(DEBUG2) << "Gate registration time " << DUR(gate_time);
  for (const xmlpp::Node* node : ft_node->find("./define-CCF-group")) {
    component->AddCcfGroup(
        Initializer::RegisterCcfGroup(XmlElement(node), base_path,
                                      component->is_public()));
  }
  for (const xmlpp::Node* node : ft_node->find("./define-component")) {
    ComponentPtr sub = Initializer::DefineComponent(XmlElement(node), base_path,
                                                    component->is_public());
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
    Initializer::DefineHouseEvent(XmlElement(node));
  }
  CLOCK(basic_time);
  for (const xmlpp::Node* node : model_data->find("./define-basic-event")) {
    Initializer::RegisterBasicEvent(XmlElement(node));
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (const xmlpp::Node* node : model_data->find("./define-parameter")) {
    Initializer::RegisterParameter(XmlElement(node));
  }
}

std::shared_ptr<Gate> Initializer::RegisterGate(const xmlpp::Element* gate_node,
                                                const std::string& base_path,
                                                bool public_container) {
  std::string name = GetAttributeValue(gate_node, "name");
  std::string role = GetAttributeValue(gate_node, "role");
  bool gate_role = public_container;  // Inherited role by default.
  if (role != "") gate_role = role == "public" ? true : false;
  GatePtr gate(new Gate(name, base_path, gate_role));
  try {
    model_->AddGate(gate);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << gate_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
  tbd_.gates.emplace_back(gate, gate_node);
  Initializer::AttachLabelAndAttributes(gate_node, gate.get());
  return gate;
}

void Initializer::DefineGate(const xmlpp::Element* gate_node,
                             const GatePtr& gate) {
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

std::unique_ptr<Formula> Initializer::GetFormula(
    const xmlpp::Element* formula_node,
    const std::string& base_path) {
  std::string type = formula_node->get_name();
  if (type == "event" || type == "basic-event" || type == "gate" ||
      type == "house-event") {
    type = "null";
  }
  FormulaPtr formula(new Formula(type));
  if (type == "atleast") {
    std::string min_num = GetAttributeValue(formula_node, "min");
    int vote_number = boost::lexical_cast<int>(min_num);
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
    if (type != "") {
      assert(type == "gate" || type == "basic-event" || type == "house-event");
      element_type = type;  // Event type is defined.
    }

    try {
      if (element_type == "event") {  // Undefined type yet.
        std::pair<EventPtr, std::string> target =
            model_->GetEvent(name, base_path);
        EventPtr undefined = target.first;
        undefined->orphan(false);
        std::string type_inference = target.second;
        if (type_inference == "gate") {
          formula->AddArgument(std::static_pointer_cast<Gate>(undefined));
        } else if (type_inference == "basic-event") {
          formula->AddArgument(std::static_pointer_cast<BasicEvent>(undefined));
        } else {
          assert(type_inference == "house-event");
          formula->AddArgument(std::static_pointer_cast<HouseEvent>(undefined));
        }
      } else if (element_type == "gate") {
        GatePtr gate = model_->GetGate(name, base_path);
        formula->AddArgument(gate);
        gate->orphan(false);

      } else if (element_type == "basic-event") {
        BasicEventPtr basic_event = model_->GetBasicEvent(name, base_path);
        formula->AddArgument(basic_event);
        basic_event->orphan(false);

      } else {
        assert(element_type == "house-event");
        HouseEventPtr house_event = model_->GetHouseEvent(name, base_path);
        formula->AddArgument(house_event);
        house_event->orphan(false);
      }
    } catch (ValidationError& err) {
      std::stringstream msg;
      msg << "Line " << event->get_line() << ":\n";
      err.msg(msg.str() + err.msg());
      throw;
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

std::shared_ptr<BasicEvent> Initializer::RegisterBasicEvent(
    const xmlpp::Element* event_node,
    const std::string& base_path,
    bool public_container) {
  std::string name = GetAttributeValue(event_node, "name");
  std::string role = GetAttributeValue(event_node, "role");
  bool event_role = public_container;  // Inherited role by default.
  if (role != "") event_role = role == "public" ? true : false;
  BasicEventPtr basic_event(new BasicEvent(name, base_path, event_role));
  try {
    model_->AddBasicEvent(basic_event);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << event_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
  tbd_.basic_events.emplace_back(basic_event, event_node);
  Initializer::AttachLabelAndAttributes(event_node, basic_event.get());
  return basic_event;
}

void Initializer::DefineBasicEvent(const xmlpp::Element* event_node,
                                   const BasicEventPtr& basic_event) {
  xmlpp::NodeSet expressions =
     event_node->find("./*[name() != 'attributes' and name() != 'label']");

  if (!expressions.empty()) {
    const xmlpp::Element* expr_node = XmlElement(expressions.back());
    ExpressionPtr expression =
        Initializer::GetExpression(expr_node, basic_event->base_path());
    basic_event->expression(expression);
  }
}

std::shared_ptr<HouseEvent> Initializer::DefineHouseEvent(
    const xmlpp::Element* event_node,
    const std::string& base_path,
    bool public_container) {
  std::string name = GetAttributeValue(event_node, "name");
  std::string role = GetAttributeValue(event_node, "role");
  bool event_role = public_container;  // Inherited role by default.
  if (role != "") event_role = role == "public" ? true : false;
  HouseEventPtr house_event(new HouseEvent(name, base_path, event_role));
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
    bool state = (val == "true") ? true : false;
    house_event->state(state);
  }
  Initializer::AttachLabelAndAttributes(event_node, house_event.get());
  return house_event;
}

std::shared_ptr<Parameter> Initializer::RegisterParameter(
    const xmlpp::Element* param_node,
    const std::string& base_path,
    bool public_container) {
  std::string name = GetAttributeValue(param_node, "name");
  std::string role = GetAttributeValue(param_node, "role");
  bool param_role = public_container;  // Inherited role by default.
  if (role != "") param_role = role == "public" ? true : false;
  ParameterPtr parameter(new Parameter(name, base_path, param_role));
  try {
    model_->AddParameter(parameter);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << param_node->get_line() << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
  tbd_.parameters.emplace_back(parameter, param_node);

  // Attach units.
  std::string unit = GetAttributeValue(param_node, "unit");
  if (unit != "") {
    assert(kUnits_.count(unit));
    parameter->unit(kUnits_.at(unit));
  }
  Initializer::AttachLabelAndAttributes(param_node, parameter.get());
  return parameter;
}

void Initializer::DefineParameter(const xmlpp::Element* param_node,
                                  const ParameterPtr& parameter) {
  // Assuming that expression is the last child of the parameter definition.
  xmlpp::NodeSet expressions =
      param_node->find("./*[name() != 'attributes' and name() != 'label']");
  assert(expressions.size() == 1);
  const xmlpp::Element* expr_node = XmlElement(expressions.back());
  ExpressionPtr expression =
      Initializer::GetExpression(expr_node, parameter->base_path());

  parameter->expression(expression);
}

std::shared_ptr<Expression> Initializer::GetExpression(
    const xmlpp::Element* expr_element,
    const std::string& base_path) {
  ExpressionPtr expression;
  bool not_parameter = true;  // Parameters are saved in a different container.
  if (GetConstantExpression(expr_element, expression)) {
  } else if (GetParameterExpression(expr_element, base_path, expression)) {
    not_parameter = false;
  } else {
    GetDeviateExpression(expr_element, base_path, expression);
  }
  assert(expression);
  if (not_parameter) expressions_.push_back(expression);
  return expression;
}

bool Initializer::GetConstantExpression(const xmlpp::Element* expr_element,
                                        ExpressionPtr& expression) {
  assert(expr_element);
  std::string expr_name = expr_element->get_name();
  if (expr_name == "float" || expr_name == "int") {
    std::string val = GetAttributeValue(expr_element, "value");
    double num = boost::lexical_cast<double>(val);
    expression = std::make_shared<ConstantExpression>(num);

  } else if (expr_name == "bool") {
    std::string val = GetAttributeValue(expr_element, "value");
    bool state = (val == "true") ? true : false;
    expression = std::make_shared<ConstantExpression>(state);
  } else {
    return false;
  }
  return true;
}

bool Initializer::GetParameterExpression(const xmlpp::Element* expr_element,
                                         const std::string& base_path,
                                         ExpressionPtr& expression) {
  assert(expr_element);
  std::string expr_name = expr_element->get_name();
  std::string param_unit = "";  // The expected unit.
  if (expr_name == "parameter") {
    std::string name = GetAttributeValue(expr_element, "name");
    try {
      ParameterPtr param = model_->GetParameter(name, base_path);
      param->unused(false);
      param_unit = kUnitToString_[param->unit()];
      expression = param;
    } catch (ValidationError& err) {
      std::stringstream msg;
      msg << "Line " << expr_element->get_line() << ":\n";
      err.msg(msg.str() + err.msg());
      throw;
    }
  } else if (expr_name == "system-mission-time") {
    param_unit = kUnitToString_[mission_time_->unit()];
    expression = mission_time_;

  } else {
    return false;
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
  return true;
}

bool Initializer::GetDeviateExpression(const xmlpp::Element* expr_element,
                                       const std::string& base_path,
                                       ExpressionPtr& expression) {
  assert(expr_element);
  std::string expr_name = expr_element->get_name();
  xmlpp::NodeSet args = expr_element->find("./*");
  if (expr_name == "uniform-deviate") {
    assert(args.size() == 2);
    ExpressionPtr min = GetExpression(XmlElement(args[0]), base_path);
    ExpressionPtr max = GetExpression(XmlElement(args[1]), base_path);
    expression = std::make_shared<UniformDeviate>(min, max);

  } else if (expr_name == "normal-deviate") {
    assert(args.size() == 2);
    ExpressionPtr mean = GetExpression(XmlElement(args[0]), base_path);
    ExpressionPtr sigma = GetExpression(XmlElement(args[1]), base_path);
    expression = std::make_shared<NormalDeviate>(mean, sigma);

  } else if (expr_name == "lognormal-deviate") {
    assert(args.size() == 3);
    ExpressionPtr mean = GetExpression(XmlElement(args[0]), base_path);
    ExpressionPtr ef = GetExpression(XmlElement(args[1]), base_path);
    ExpressionPtr level = GetExpression(XmlElement(args[2]), base_path);
    expression = std::make_shared<LogNormalDeviate>(mean, ef, level);

  } else if (expr_name == "gamma-deviate") {
    assert(args.size() == 2);
    ExpressionPtr k = GetExpression(XmlElement(args[0]), base_path);
    ExpressionPtr theta = GetExpression(XmlElement(args[1]), base_path);
    expression = std::make_shared<GammaDeviate>(k, theta);

  } else if (expr_name == "beta-deviate") {
    assert(args.size() == 2);
    ExpressionPtr alpha = GetExpression(XmlElement(args[0]), base_path);
    ExpressionPtr beta = GetExpression(XmlElement(args[1]), base_path);
    expression = std::make_shared<BetaDeviate>(alpha, beta);

  } else if (expr_name == "histogram") {
    std::vector<ExpressionPtr> boundaries;
    std::vector<ExpressionPtr> weights;
    for (const xmlpp::Node* node : args) {
      const xmlpp::Element* el = XmlElement(node);
      xmlpp::NodeSet pair = el->find("./*");
      assert(pair.size() == 2);
      ExpressionPtr bound = GetExpression(XmlElement(pair[0]), base_path);
      boundaries.push_back(bound);

      ExpressionPtr weight = GetExpression(XmlElement(pair[1]), base_path);
      weights.push_back(weight);
    }
    expression = std::make_shared<Histogram>(boundaries, weights);

  } else if (expr_name == "exponential") {
    assert(args.size() == 2);
    ExpressionPtr lambda = GetExpression(XmlElement(args[0]), base_path);
    ExpressionPtr time = GetExpression(XmlElement(args[1]), base_path);
    expression = std::make_shared<ExponentialExpression>(lambda, time);

  } else if (expr_name == "GLM") {
    assert(args.size() == 4);
    ExpressionPtr gamma = GetExpression(XmlElement(args[0]), base_path);
    ExpressionPtr lambda = GetExpression(XmlElement(args[1]), base_path);
    ExpressionPtr mu = GetExpression(XmlElement(args[2]), base_path);
    ExpressionPtr time = GetExpression(XmlElement(args[3]), base_path);
    expression = std::make_shared<GlmExpression>(gamma, lambda, mu, time);

  } else if (expr_name == "Weibull") {
    assert(args.size() == 4);
    ExpressionPtr alpha = GetExpression(XmlElement(args[0]), base_path);
    ExpressionPtr beta = GetExpression(XmlElement(args[1]), base_path);
    ExpressionPtr t0 = GetExpression(XmlElement(args[2]), base_path);
    ExpressionPtr time = GetExpression(XmlElement(args[3]), base_path);
    expression = std::make_shared<WeibullExpression>(alpha, beta, t0, time);
  } else {
    return false;
  }
  return true;
}

std::shared_ptr<CcfGroup> Initializer::RegisterCcfGroup(
    const xmlpp::Element* ccf_node,
    const std::string& base_path,
    bool public_container) {
  std::string name = GetAttributeValue(ccf_node, "name");
  std::string model = GetAttributeValue(ccf_node, "model");
  assert(model == "beta-factor" || model == "alpha-factor" || model == "MGL" ||
         model == "phi-factor");

  CcfGroupPtr ccf_group;
  if (model == "beta-factor") {
    ccf_group = CcfGroupPtr(new BetaFactorModel(name, base_path,
                                                public_container));

  } else if (model == "MGL") {
    ccf_group = CcfGroupPtr(new MglModel(name, base_path, public_container));

  } else if (model == "alpha-factor") {
    ccf_group = CcfGroupPtr(new AlphaFactorModel(name, base_path,
                                                 public_container));

  } else if (model == "phi-factor") {
    ccf_group = CcfGroupPtr(new PhiFactorModel(name, base_path,
                                               public_container));
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
  Initializer::ProcessCcfMembers(XmlElement(members[0]), ccf_group);

  Initializer::AttachLabelAndAttributes(ccf_node, ccf_group.get());

  tbd_.ccf_groups.emplace_back(ccf_group, ccf_node);
  return ccf_group;
}

void Initializer::DefineCcfGroup(const xmlpp::Element* ccf_node,
                                 const CcfGroupPtr& ccf_group) {
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
                                    const CcfGroupPtr& ccf_group) {
  for (const xmlpp::Node* node : members_node->find("./*")) {
    const xmlpp::Element* event_node = XmlElement(node);
    assert("basic-event" == event_node->get_name());

    std::string name = GetAttributeValue(event_node, "name");
    BasicEventPtr basic_event(new BasicEvent(name, ccf_group->base_path(),
                                             ccf_group->is_public()));
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
                                  const CcfGroupPtr& ccf_group) {
  // Checking the level for one factor input.
  std::string level = GetAttributeValue(factor_node, "level");
  if (level.empty()) {
    std::stringstream msg;
    msg << "Line " << factor_node->get_line() << ":\n";
    msg << "CCF group factor level number is not provided.";
    throw ValidationError(msg.str());
  }
  int level_num = boost::lexical_cast<int>(level);
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
    if (cycle::DetectCycle<Gate, Formula>(gate.second.get(), &cycle)) {
      std::string msg =
          "Detected a cycle in " + gate.second->name() + " gate:\n";
      msg += cycle::PrintCycle(cycle);
      throw ValidationError(msg);
    }
  }
  std::stringstream error_messages;
  // Check if all primary events have expressions for probability analysis.
  if (settings_.probability_analysis()) {
    std::string msg = "";
    for (const std::pair<const std::string, BasicEventPtr>& event :
         model_->basic_events()) {
      if (!event.second->has_expression()) msg += event.second->name() + "\n";
    }
    for (const std::pair<const std::string, HouseEventPtr>& event :
         model_->house_events()) {
      if (!event.second->has_expression()) msg += event.second->name() + "\n";
    }
    if (!msg.empty()) {
      error_messages << "\nThese primary events do not have expressions:\n"
                     << msg;
    }
  }

  if (!error_messages.str().empty())
    throw ValidationError(error_messages.str());

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
    if (cycle::DetectCycle<Parameter, Expression>(p.second.get(), &cycle)) {
      std::string msg = "Detected a cycle in " + p.second->name() +
          " parameter:\n";
      msg += cycle::PrintCycle(cycle);
      throw ValidationError(msg);
    }
  }

  // Validate expressions.
  try {
    for (const ExpressionPtr& expression : expressions_) expression->Validate();
  } catch (InvalidArgument& err) {
    throw ValidationError(err.msg());
  }

  // Check probability values for primary events.
  if (settings_.probability_analysis()) {
    std::stringstream msg;
    msg << "";
    for (const std::pair<const std::string, CcfGroupPtr>& group :
         model_->ccf_groups()) {
      try {
        group.second->ValidateDistribution();
      } catch (ValidationError& err) {
        msg << group.second->name() << " : " << err.msg() << "\n";
      }
    }
    for (const std::pair<const std::string, BasicEventPtr>& event :
         model_->basic_events()) {
      try {
        event.second->Validate();
      } catch (ValidationError& err) {
        msg << event.second->name() << " : " << err.msg() << "\n";
      }
    }
    if (!msg.str().empty()) {
      std::string head = "Invalid probabilities detected:\n";
      throw ValidationError(head + msg.str());
    }
  }
}

void Initializer::SetupForAnalysis() {
  // Collecting top events of fault trees.
  for (const std::pair<const std::string, FaultTreePtr>& ft :
       model_->fault_trees()) {
    ft.second->CollectTopEvents();
  }

  // CCF groups must apply models to basic event members.
  for (const std::pair<const std::string, CcfGroupPtr>& group :
       model_->ccf_groups()) {
    group.second->ApplyModel();
  }
}

}  // namespace scram
