/// @file risk_analysis.cc
/// Implementation of risk analysis handler.
#include "risk_analysis.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/pointer_cast.hpp>

#include "ccf_group.h"
#include "cycle.h"
#include "element.h"
#include "env.h"
#include "error.h"
#include "fault_tree.h"
#include "grapher.h"
#include "logger.h"
#include "random.h"
#include "reporter.h"
#include "xml_parser.h"

namespace fs = boost::filesystem;

namespace scram {

std::map<std::string, Units> RiskAnalysis::units_ =
    boost::assign::map_list_of ("bool", kBool) ("int", kInt) ("float", kFloat)
                               ("hours", kHours) ("hours-1", kInverseHours)
                               ("years", kYears) ("years-1", kInverseYears)
                               ("fit", kFit) ("demands", kDemands);

const char* RiskAnalysis::unit_to_string_[] = {"unitless", "bool", "int",
                                               "float", "hours", "hours-1",
                                               "years", "years-1", "fit",
                                               "demands"};

RiskAnalysis::RiskAnalysis() {
  // Initialize the mission time with any value.
  mission_time_ = boost::shared_ptr<MissionTime>(new MissionTime());
}

void RiskAnalysis::ProcessInput(std::string xml_file) {
  std::vector<std::string> single;
  single.push_back(xml_file);
  RiskAnalysis::ProcessInputFiles(single);
}

void RiskAnalysis::ProcessInputFiles(
    const std::vector<std::string>& xml_files) {
  // Assume that settings are configured.
  mission_time_->mission_time(settings_.mission_time_);

  CLOCK(input_time);
  LOG(DEBUG1) << "Processing input files";
  std::vector<std::string>::const_iterator it;
  for (it = xml_files.begin(); it != xml_files.end(); ++it) {
    try {
      RiskAnalysis::ProcessInputFile(*it);
    } catch (ValidationError& err) {
      err.msg("In file '" + *it + "', " + err.msg());
      throw err;
    }
  }
  CLOCK(def_time);
  RiskAnalysis::ProcessTbdElements();
  LOG(DEBUG2) << "Element definition time " << DUR(def_time);
  LOG(DEBUG1) << "Input files are processed in " << DUR(input_time);

  CLOCK(valid_time);
  LOG(DEBUG1) << "Validating the input files";
  // Check if the initialization is successful.
  RiskAnalysis::ValidateInitialization();
  LOG(DEBUG1) << "Validation is finished in " << DUR(valid_time);

  CLOCK(setup_time);
  LOG(DEBUG1) << "Setting up for the analysis";
  // Perform setup for analysis using configurations from the input files.
  RiskAnalysis::SetupForAnalysis();
  LOG(DEBUG1) << "Setup time " << DUR(setup_time);
}

void RiskAnalysis::GraphingInstructions() {
  CLOCK(graph_time);
  LOG(DEBUG1) << "Producing graphing instructions";
  std::map<std::string, FaultTreePtr>::iterator it;
  for (it = fault_trees_.begin(); it != fault_trees_.end(); ++it) {
    const std::vector<GatePtr>* top_events = &it->second->top_events();
    std::vector<GatePtr>::const_iterator it_top;
    for (it_top = top_events->begin(); it_top != top_events->end(); ++it_top) {
      std::string output =
          it->second->name() + "_" + (*it_top)->name() + ".dot";
      std::ofstream of(output.c_str());
      if (!of.good()) {
        throw IOError(output +  " : Cannot write the graphing file.");
      }
      Grapher gr = Grapher();
      gr.GraphFaultTree(*it_top, settings_.probability_analysis_, of);
      of.flush();
    }
  }
  LOG(DEBUG1) << "Graphing instructions are produced in " << DUR(graph_time);
}

void RiskAnalysis::Analyze() {
  // Set system mission time for all analysis.
  mission_time_->mission_time(settings_.mission_time_);

  // Set the seed for the pseudo-random number generator if given explicitly.
  // Otherwise it defaults to the current time.
  if (settings_.seed_ >= 0) Random::seed(settings_.seed_);

  std::map<std::string, FaultTreePtr>::iterator it;
  for (it = fault_trees_.begin(); it != fault_trees_.end(); ++it) {
    const std::vector<GatePtr>* top_events = &it->second->top_events();
    std::vector<GatePtr>::const_iterator it_top;
    for (it_top = top_events->begin(); it_top != top_events->end(); ++it_top) {
      std::string name = (*it_top)->name();  // Analysis identifier.

      FaultTreeAnalysisPtr fta(new FaultTreeAnalysis(*it_top,
                                                     settings_.limit_order_,
                                                     settings_.ccf_analysis_));
      fta->Analyze();
      ftas_.insert(std::make_pair(name, fta));

      if (settings_.probability_analysis_) {
        ProbabilityAnalysisPtr pa(
            new ProbabilityAnalysis(settings_.approx_, settings_.num_sums_,
                                    settings_.cut_off_,
                                    settings_.importance_analysis_));
        pa->UpdateDatabase(fta->mcs_basic_events());
        pa->Analyze(fta->min_cut_sets());
        prob_analyses_.insert(std::make_pair(name, pa));
      }

      if (settings_.uncertainty_analysis_) {
        UncertaintyAnalysisPtr ua(
            new UncertaintyAnalysis(settings_.num_sums_, settings_.cut_off_,
                                    settings_.num_trials_));
        ua->UpdateDatabase(fta->mcs_basic_events());
        ua->Analyze(fta->min_cut_sets());
        uncertainty_analyses_.insert(std::make_pair(name, ua));
      }
    }
  }
}

void RiskAnalysis::Report(std::ostream& out) {
  Reporter rp = Reporter();

  // Create XML or use already created document.
  xmlpp::Document* doc = new xmlpp::Document();
  rp.SetupReport(this, settings_, doc);

  /// Container for excess primary events not in the analysis.
  /// This container is for warning in case the input is formed not as intended.
  std::set<PrimaryEventPtr> orphan_primary_events;
  boost::unordered_map<std::string, BasicEventPtr>::iterator it_b;
  for (it_b = basic_events_.begin(); it_b != basic_events_.end(); ++it_b) {
    if (it_b->second->orphan()) orphan_primary_events.insert(it_b->second);
  }
  boost::unordered_map<std::string, HouseEventPtr>::iterator it_h;
  for (it_h = house_events_.begin(); it_h != house_events_.end(); ++it_h) {
    if (it_h->second->orphan()) orphan_primary_events.insert(it_h->second);
  }
  if (!orphan_primary_events.empty())
    rp.ReportOrphanPrimaryEvents(orphan_primary_events, doc);

  /// Container for unused parameters not in the analysis.
  /// This container is for warning in case the input is formed not as intended.
  std::set<ParameterPtr> unused_parameters;
  boost::unordered_map<std::string, ParameterPtr>::iterator it_v;
  for (it_v = parameters_.begin(); it_v != parameters_.end(); ++it_v) {
    if (it_v->second->unused()) unused_parameters.insert(it_v->second);
  }

  if (!unused_parameters.empty())
    rp.ReportUnusedParameters(unused_parameters, doc);

  std::map<std::string, FaultTreeAnalysisPtr>::iterator it;
  for (it = ftas_.begin(); it != ftas_.end(); ++it) {
    ProbabilityAnalysisPtr prob_analysis;  // Null pointer if no analysis.
    if (settings_.probability_analysis_) {
      prob_analysis = prob_analyses_.find(it->first)->second;
    }
    rp.ReportFta(it->first, ftas_.find(it->first)->second, prob_analysis, doc);

    if (settings_.importance_analysis_) {
      rp.ReportImportance(it->first,
                          prob_analyses_.find(it->first)->second, doc);
    }

    if (settings_.uncertainty_analysis_) {
        rp.ReportUncertainty(it->first,
                             uncertainty_analyses_.find(it->first)->second,
                             doc);
    }
  }

  doc->write_to_stream_formatted(out, "UTF-8");
  delete doc;
}

void RiskAnalysis::Report(std::string output) {
  std::ofstream of(output.c_str());
  if (!of.good()) {
    throw IOError(output +  " : Cannot write the output file.");
  }
  RiskAnalysis::Report(of);
}

void RiskAnalysis::ProcessInputFile(std::string xml_file) {
  std::ifstream file_stream(xml_file.c_str());
  if (!file_stream) {
    throw IOError("File '" + xml_file + "' could not be loaded.");
  }
  fs::path file_path = fs::canonical(xml_file);
  if (input_path_.count(file_path.native())) {
    throw ValidationError("Trying to pass the same file twice: " +
                          file_path.native());
  }
  input_path_.insert(file_path.native());

  std::stringstream stream;
  stream << file_stream.rdbuf();
  file_stream.close();

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  parser->Init(stream);

  std::stringstream schema;
  std::string schema_path = Env::input_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  parser->Validate(schema);
  parsers_.push_back(parser);

  const xmlpp::Document* doc = parser->Document();
  const xmlpp::Node* root = doc->get_root_node();
  assert(root->get_name() == "opsa-mef");
  xmlpp::NodeSet name_attr = root->find("./@name");
  std::string model_name = "";
  if (!name_attr.empty()) {
    assert(name_attr.size() == 1);
    const xmlpp::Attribute* attr =
        dynamic_cast<const xmlpp::Attribute*>(name_attr[0]);
    model_name = attr->get_value();
  }
  ModelPtr new_model(new Model(xml_file, model_name));
  RiskAnalysis::AttachLabelAndAttributes(
      dynamic_cast<const xmlpp::Element*>(root),
      new_model);
  models_.insert(std::make_pair(xml_file, new_model));

  xmlpp::NodeSet::iterator it_ch;  // Iterator for all children.

  xmlpp::NodeSet fault_trees = root->find("./define-fault-tree");
  for (it_ch = fault_trees.begin(); it_ch != fault_trees.end(); ++it_ch) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it_ch);
    assert(element);
    RiskAnalysis::DefineFaultTree(element);
  }

  xmlpp::NodeSet ccf_groups = root->find("./define-CCF-group");
  for (it_ch = ccf_groups.begin(); it_ch != ccf_groups.end(); ++it_ch) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it_ch);
    assert(element);
    RiskAnalysis::RegisterCcfGroup(element);
  }

  xmlpp::NodeSet model_data = root->find("./model-data");
  for (it_ch = model_data.begin(); it_ch != model_data.end(); ++it_ch) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it_ch);
    assert(element);
    RiskAnalysis::ProcessModelData(element);
  }
}

void RiskAnalysis::ProcessTbdElements() {
  std::vector< std::pair<ElementPtr, const xmlpp::Element*> >::iterator it;
  for (it = tbd_elements_.begin(); it != tbd_elements_.end(); ++it) {
    if (boost::dynamic_pointer_cast<BasicEvent>(it->first)) {
      BasicEventPtr basic_event =
          boost::dynamic_pointer_cast<BasicEvent>(it->first);
      DefineBasicEvent(it->second, basic_event);
    } else if (boost::dynamic_pointer_cast<Gate>(it->first)) {
      GatePtr gate =
          boost::dynamic_pointer_cast<Gate>(it->first);
      DefineGate(it->second, gate);
    } else if (boost::dynamic_pointer_cast<CcfGroup>(it->first)) {
      CcfGroupPtr ccf_group = boost::dynamic_pointer_cast<CcfGroup>(it->first);
      DefineCcfGroup(it->second, ccf_group);
    } else if (boost::dynamic_pointer_cast<Parameter>(it->first)) {
      ParameterPtr param = boost::dynamic_pointer_cast<Parameter>(it->first);
      DefineParameter(it->second, param);
    }
  }
}

void RiskAnalysis::AttachLabelAndAttributes(
    const xmlpp::Element* element_node,
    const ElementPtr& element) {
  xmlpp::NodeSet labels = element_node->find("./*[name() = 'label']");
  if (!labels.empty()) {
    assert(labels.size() == 1);
    const xmlpp::Element* label =
        dynamic_cast<const xmlpp::Element*>(labels.front());
    assert(label);
    const xmlpp::TextNode* text = label->get_child_text();
    assert(text);
    element->label(text->get_content());
  }

  xmlpp::NodeSet attributes = element_node->find("./*[name() = 'attributes']");
  if (!attributes.empty()) {
    assert(attributes.size() == 1);  // Only one big element 'attributes'.
    const xmlpp::Element* attributes_element =
        dynamic_cast<const xmlpp::Element*>(attributes.front());
    xmlpp::NodeSet attribute_list =
        attributes_element->find("./*[name() = 'attribute']");
    xmlpp::NodeSet::iterator it;
    for (it = attribute_list.begin(); it != attribute_list.end(); ++it) {
      const xmlpp::Element* attribute =
          dynamic_cast<const xmlpp::Element*>(*it);
      Attribute attr;
      attr.name = attribute->get_attribute_value("name");
      attr.value = attribute->get_attribute_value("value");
      attr.type = attribute->get_attribute_value("type");
      element->AddAttribute(attr);
    }
  }
}

void RiskAnalysis::DefineFaultTree(const xmlpp::Element* ft_node) {
  std::string name = ft_node->get_attribute_value("name");
  std::string id = name;
  boost::to_lower(id);

  if (fault_trees_.count(id)) {
    std::stringstream msg;
    msg << "Line " << ft_node->get_line() << ":\n";
    msg << "Fault tree " << name << " is already defined.";
    throw ValidationError(msg.str());
  }

  FaultTreePtr fault_tree = FaultTreePtr(new FaultTree(name));
  fault_trees_.insert(std::make_pair(id, fault_tree));

  RiskAnalysis::AttachLabelAndAttributes(ft_node, fault_tree);

  xmlpp::NodeSet gates = ft_node->find("./define-gate");
  xmlpp::NodeSet ccf_groups = ft_node->find("./define-CCF-group");

  xmlpp::NodeSet::iterator it;
  CLOCK(gate_time);
  for (it = gates.begin(); it != gates.end(); ++it) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it);
    assert(element);
    RiskAnalysis::RegisterGate(element, fault_tree);
  }
  LOG(DEBUG2) << "Gate registration time " << DUR(gate_time);
  for (it = ccf_groups.begin(); it != ccf_groups.end(); ++it) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it);
    assert(element);
    RiskAnalysis::RegisterCcfGroup(element);
  }
  // Handle house events, basic events, and parameters.
  RiskAnalysis::ProcessModelData(ft_node);
}

void RiskAnalysis::ProcessModelData(const xmlpp::Element* model_data) {
  xmlpp::NodeSet house_events = model_data->find("./define-house-event");
  xmlpp::NodeSet basic_events = model_data->find("./define-basic-event");
  xmlpp::NodeSet parameters = model_data->find("./define-parameter");

  xmlpp::NodeSet::iterator it;

  for (it = house_events.begin(); it != house_events.end(); ++it) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it);
    assert(element);
    RiskAnalysis::DefineHouseEvent(element);
  }
  CLOCK(basic_time);
  for (it = basic_events.begin(); it != basic_events.end(); ++it) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it);
    assert(element);
    RiskAnalysis::RegisterBasicEvent(element);
  }
  LOG(DEBUG2) << "Basic event registration time " << DUR(basic_time);
  for (it = parameters.begin(); it != parameters.end(); ++it) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it);
    assert(element);
    RiskAnalysis::RegisterParameter(element);
  }
}

void RiskAnalysis::RegisterGate(const xmlpp::Element* gate_node,
                                const FaultTreePtr& ft) {
  // Only one child element is expected, which is a formula.
  std::string name = gate_node->get_attribute_value("name");
  boost::trim(name);
  std::string id = name;
  boost::to_lower(id);

  if (gates_.count(id) || basic_events_.count(id) || house_events_.count(id)) {
    // Redefined event.
    std::stringstream msg;
    msg << "Line " << gate_node->get_line() << ":\n";
    msg << name << " event is being redefined.";
    throw ValidationError(msg.str());
  }

  GatePtr gate(new Gate(id));
  gate->name(name);

  tbd_elements_.push_back(std::make_pair(gate, gate_node));
  gates_.insert(std::make_pair(id, gate));

  RiskAnalysis::AttachLabelAndAttributes(gate_node, gate);

  ft->AddGate(gate);
  gate->container(ft->name());
}

void RiskAnalysis::DefineGate(const xmlpp::Element* gate_node,
                              const GatePtr& gate) {
  xmlpp::NodeSet formulas =
      gate_node->find("./*[name() != 'attributes' and name() != 'label']");
  // Assumes that there are no attributes and labels.
  assert(formulas.size() == 1);
  const xmlpp::Element* formula_node =
      dynamic_cast<const xmlpp::Element*>(formulas.front());
  gate->formula(RiskAnalysis::GetFormula(formula_node));
  try {
    gate->Validate();
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << gate_node->get_line() << ":\n";
    throw ValidationError(msg.str() + err.msg());
  }
}

boost::shared_ptr<Formula> RiskAnalysis::GetFormula(
    const xmlpp::Element* formula_node) {
  std::string type = formula_node->get_name();
  if (type == "event" || type == "basic-event" || type == "gate" ||
      type == "house-event") {
    type = "null";
  }
  FormulaPtr formula(new Formula(type));
  if (type == "atleast") {
    std::string min_num = formula_node->get_attribute_value("min");
    boost::trim(min_num);
    int vote_number = boost::lexical_cast<int>(min_num);
    formula->vote_number(vote_number);
  }
  // Process arguments of this formula.
  if (type == "null") {
    RiskAnalysis::ProcessFormula(formula_node->get_parent(), formula);
  } else {
    RiskAnalysis::ProcessFormula(formula_node, formula);
  }

  try {
    formula->Validate();
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << formula_node->get_line() << ":\n";
    throw ValidationError(msg.str() + err.msg());
  }
  return formula;
}

void RiskAnalysis::ProcessFormula(const xmlpp::Element* formula_node,
                                  const FormulaPtr& formula) {
  xmlpp::NodeSet events = formula_node->find("./*[name() = 'event' or "
                                             "name() = 'gate' or "
                                             "name() = 'basic-event' or "
                                             "name() = 'house-event']");
  std::set<std::string> children_id;  // To detect repeated children.
  xmlpp::NodeSet::const_iterator it;
  for (it = events.begin(); it != events.end(); ++it) {
    const xmlpp::Element* event = dynamic_cast<const xmlpp::Element*>(*it);
    assert(event);
    std::string name = event->get_attribute_value("name");
    boost::trim(name);
    std::string id = name;
    boost::to_lower(id);

    if (children_id.count(id)) {
      std::stringstream msg;
      msg << "Line " << event->get_line() << ":\n";
      msg << "Detected a repeated child " << name;
      throw ValidationError(msg.str());
    } else {
      children_id.insert(id);
    }

    std::string element_type = event->get_name();
    // This is for a case "<event name="id" type="type"/>".
    std::string type = event->get_attribute_value("type");
    boost::trim(type);
    if (type != "") {
      assert(type == "gate" || type == "basic-event" ||
             type == "house-event");
      element_type = type;  // Event type is defined.
    }

    EventPtr child(new Event(id));
    child->name(name);
    if (element_type == "event") {  // Undefined type yet.
      RiskAnalysis::ProcessFormulaEvent(event, child);

    } else if (element_type == "gate") {
      RiskAnalysis::ProcessFormulaGate(event, child);

    } else if (element_type == "basic-event") {
      RiskAnalysis::ProcessFormulaBasicEvent(event, child);

    } else if (element_type == "house-event") {
      RiskAnalysis::ProcessFormulaHouseEvent(event, child);
    }

    formula->AddArgument(child);
    child->orphan(false);
  }

  xmlpp::NodeSet formulas = formula_node->find("./*[name() != 'event' and "
                                               "name() != 'gate' and "
                                               "name() != 'basic-event' and "
                                               "name() != 'house-event']");
  for (it = formulas.begin(); it != formulas.end(); ++it) {
    const xmlpp::Element* nested_formula =
        dynamic_cast<const xmlpp::Element*>(*it);
    assert(nested_formula);
    formula->AddArgument(RiskAnalysis::GetFormula(nested_formula));
  }
}

void RiskAnalysis::ProcessFormulaEvent(const xmlpp::Element* event,
                                       EventPtr& child) {
  std::string id = child->id();
  if (basic_events_.count(id)) {
    child = basic_events_.find(id)->second;

  } else if (gates_.count(id)) {
    child = gates_.find(id)->second;

  } else if (house_events_.count(id)) {
    child = house_events_.find(id)->second;

  } else {
    std::stringstream msg;
    msg << "Line " << event->get_line() << ":\n";
    msg << "Undefined event: " << child->name();
    throw ValidationError(msg.str());
  }
}

void RiskAnalysis::ProcessFormulaBasicEvent(const xmlpp::Element* event,
                                            EventPtr& child) {
  std::string id = child->id();
  std::string name = child->name();
  if (!basic_events_.count(id)) {
    std::stringstream msg;
    msg << "Line " << event->get_line() << ":\n";
    msg << "Undefined basic event: " << name;
    throw ValidationError(msg.str());
  }
  child = basic_events_.find(id)->second;
}

void RiskAnalysis::ProcessFormulaHouseEvent(const xmlpp::Element* event,
                                            EventPtr& child) {
  std::string id = child->id();
  if (house_events_.count(id)) {
    child = house_events_.find(id)->second;

  } else {
    std::stringstream msg;
    msg << "Line " << event->get_line() << ":\n";
    msg << "Undefined house event: " << child->name();
    throw ValidationError(msg.str());
  }
}

void RiskAnalysis::ProcessFormulaGate(const xmlpp::Element* event,
                                      EventPtr& child) {
  std::string id = child->id();
  std::string name = child->name();
  if (!gates_.count(id)) {
    std::stringstream msg;
    msg << "Line " << event->get_line() << ":\n";
    msg << "Undefined gate: " << name;
    throw ValidationError(msg.str());
  }
  child = gates_.find(id)->second;
}

void RiskAnalysis::RegisterBasicEvent(const xmlpp::Element* event_node) {
  std::string name = event_node->get_attribute_value("name");
  boost::trim(name);
  std::string id = name;
  boost::to_lower(id);
  // Detect name clashes.
  if (gates_.count(id) || basic_events_.count(id) || house_events_.count(id)) {
    std::stringstream msg;
    msg << "Line " << event_node->get_line() << ":\n";
    msg << name << " event is being redefined.";
    throw ValidationError(msg.str());
  }
  BasicEventPtr basic_event = BasicEventPtr(new BasicEvent(id));
  basic_event->name(name);
  basic_events_.insert(std::make_pair(id, basic_event));
  tbd_elements_.push_back(std::make_pair(basic_event, event_node));
  RiskAnalysis::AttachLabelAndAttributes(event_node, basic_event);
}

void RiskAnalysis::DefineBasicEvent(const xmlpp::Element* event_node,
                                    const BasicEventPtr& basic_event) {
  xmlpp::NodeSet expressions =
     event_node->find("./*[name() != 'attributes' and name() != 'label']");

  if (!expressions.empty()) {
    const xmlpp::Element* expr_node =
        dynamic_cast<const xmlpp::Element*>(expressions.back());
    assert(expr_node);
    ExpressionPtr expression = RiskAnalysis::GetExpression(expr_node);
    basic_event->expression(expression);
  }
}

void RiskAnalysis::DefineHouseEvent(const xmlpp::Element* event_node) {
  std::string name = event_node->get_attribute_value("name");
  boost::trim(name);
  std::string id = name;
  boost::to_lower(id);
  // Detect name clashes.
  if (gates_.count(id) || basic_events_.count(id) || house_events_.count(id)) {
    std::stringstream msg;
    msg << "Line " << event_node->get_line() << ":\n";
    msg << name << " event is being redefined.";
    throw ValidationError(msg.str());
  }
  HouseEventPtr house_event = HouseEventPtr(new HouseEvent(id));
  house_event->name(name);
  // Only Boolean constant.
  xmlpp::NodeSet expression = event_node->find("./*[name() = 'constant']");
  if (!expression.empty()) {
    assert(expression.size() == 1);
    const xmlpp::Element* constant =
        dynamic_cast<const xmlpp::Element*>(expression.front());
    if (!constant) assert(false);

    std::string val = constant->get_attribute_value("value");
    boost::trim(val);
    assert(val == "true" || val == "false");
    bool state = (val == "true") ? true : false;
    house_event->state(state);
  }
  house_events_.insert(std::make_pair(id, house_event));
  RiskAnalysis::AttachLabelAndAttributes(event_node, house_event);
}

void RiskAnalysis::RegisterParameter(const xmlpp::Element* param_node) {
  std::string name = param_node->get_attribute_value("name");
  boost::trim(name);
  // Detect case sensitive name clashes.
  if (parameters_.count(name)) {
    std::stringstream msg;
    msg << "Line " << param_node->get_line() << ":\n";
    msg << name << " parameter is being redefined.";
    throw ValidationError(msg.str());
  }

  ParameterPtr parameter = ParameterPtr(new Parameter(name));
  parameters_.insert(std::make_pair(name, parameter));

  tbd_elements_.push_back(std::make_pair(parameter, param_node));

  // Attach units.
  std::string unit = param_node->get_attribute_value("unit");
  if (unit != "") {
    assert(units_.count(unit));
    parameter->unit(units_.find(unit)->second);
  }
  RiskAnalysis::AttachLabelAndAttributes(param_node, parameter);
}

void RiskAnalysis::DefineParameter(const xmlpp::Element* param_node,
                                   const ParameterPtr& parameter) {
  // Assuming that expression is the last child of the parameter definition.
  xmlpp::NodeSet expressions =
      param_node->find("./*[name() != 'attributes' and name() != 'label']");
  assert(expressions.size() == 1);
  const xmlpp::Element* expr_node =
      dynamic_cast<const xmlpp::Element*>(expressions.back());
  assert(expr_node);
  ExpressionPtr expression = RiskAnalysis::GetExpression(expr_node);

  parameter->expression(expression);
}

boost::shared_ptr<Expression> RiskAnalysis::GetExpression(
    const xmlpp::Element* expr_element) {
  using scram::RiskAnalysis;
  ExpressionPtr expression;
  bool not_parameter = true;  // Parameters are saved in a different container.
  if (GetConstantExpression(expr_element, expression)) {
  } else if (GetParameterExpression(expr_element, expression)) {
    not_parameter = false;
  } else {
    GetDeviateExpression(expr_element, expression);
  }
  assert(expression);
  if (not_parameter) expressions_.push_back(expression);
  return expression;
}

bool RiskAnalysis::GetConstantExpression(const xmlpp::Element* expr_element,
                                         ExpressionPtr& expression) {
  typedef boost::shared_ptr<ConstantExpression> ConstantExpressionPtr;
  assert(expr_element);
  std::string expr_name = expr_element->get_name();
  if (expr_name == "float" || expr_name == "int") {
    std::string val = expr_element->get_attribute_value("value");
    boost::trim(val);
    double num = boost::lexical_cast<double>(val);
    expression = ConstantExpressionPtr(new ConstantExpression(num));

  } else if (expr_name == "bool") {
    std::string val = expr_element->get_attribute_value("value");
    boost::trim(val);
    bool state = (val == "true") ? true : false;
    expression = ConstantExpressionPtr(new ConstantExpression(state));
  } else {
    return false;
  }
  return true;
}

bool RiskAnalysis::GetParameterExpression(const xmlpp::Element* expr_element,
                                          ExpressionPtr& expression) {
  assert(expr_element);
  std::string expr_name = expr_element->get_name();
  std::string param_unit = "";  // The expected unit.
  if (expr_name == "parameter") {
    std::string name = expr_element->get_attribute_value("name");
    if (parameters_.count(name)) {
      ParameterPtr param = parameters_.find(name)->second;
      param->unused(false);
      param_unit = unit_to_string_[param->unit()];
      expression = param;
    } else {
      std::stringstream msg;
      msg << "Line " << expr_element->get_line() << ":\n";
      msg << "Undefined parameter: " << name;
      throw ValidationError(msg.str());
    }
  } else if (expr_name == "system-mission-time") {
    param_unit = unit_to_string_[mission_time_->unit()];
    expression = mission_time_;

  } else {
    return false;
  }
  // Check units.
  std::string unit = expr_element->get_attribute_value("unit");
  boost::trim(unit);
  if (!unit.empty() && unit != param_unit) {
    std::stringstream msg;
    msg << "Line " << expr_element->get_line() << ":\n";
    msg << "Parameter unit mismatch.\nExpected: " << param_unit
        << "\nGiven: " << unit;
    throw ValidationError(msg.str());
  }
  return true;
}

bool RiskAnalysis::GetDeviateExpression(const xmlpp::Element* expr_element,
                                        ExpressionPtr& expression) {
  assert(expr_element);
  std::string expr_name = expr_element->get_name();
  xmlpp::NodeSet args = expr_element->find("./*");
  if (expr_name == "uniform-deviate") {
    assert(args.size() == 2);
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(args[0]);
    assert(element);
    ExpressionPtr min = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[1]);
    assert(element);
    ExpressionPtr max = GetExpression(element);

    expression = boost::shared_ptr<UniformDeviate>(
        new UniformDeviate(min, max));

  } else if (expr_name == "normal-deviate") {
    assert(args.size() == 2);
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(args[0]);
    assert(element);
    ExpressionPtr mean = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[1]);
    assert(element);
    ExpressionPtr sigma = GetExpression(element);

    expression = boost::shared_ptr<NormalDeviate>(
        new NormalDeviate(mean, sigma));

  } else if (expr_name == "lognormal-deviate") {
    assert(args.size() == 3);
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(args[0]);
    assert(element);
    ExpressionPtr mean = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[1]);
    assert(element);
    ExpressionPtr ef = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[2]);
    assert(element);
    ExpressionPtr level = GetExpression(element);

    expression = boost::shared_ptr<LogNormalDeviate>(
        new LogNormalDeviate(mean, ef, level));

  } else if (expr_name == "gamma-deviate") {
    assert(args.size() == 2);
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(args[0]);
    assert(element);
    ExpressionPtr k = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[1]);
    assert(element);
    ExpressionPtr theta = GetExpression(element);

    expression = boost::shared_ptr<GammaDeviate>(new GammaDeviate(k, theta));

  } else if (expr_name == "beta-deviate") {
    assert(args.size() == 2);
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(args[0]);
    assert(element);
    ExpressionPtr alpha = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[1]);
    assert(element);
    ExpressionPtr beta = GetExpression(element);

    expression = boost::shared_ptr<BetaDeviate>(new BetaDeviate(alpha, beta));

  } else if (expr_name == "histogram") {
    std::vector<ExpressionPtr> boundaries;
    std::vector<ExpressionPtr> weights;
    xmlpp::NodeSet::iterator it;
    for (it = args.begin(); it != args.end(); ++it) {
      const xmlpp::Element* el = dynamic_cast<const xmlpp::Element*>(*it);
      xmlpp::NodeSet pair = el->find("./*");
      assert(pair.size() == 2);
      const xmlpp::Element* element =
          dynamic_cast<const xmlpp::Element*>(pair[0]);
      assert(element);
      ExpressionPtr bound = GetExpression(element);
      boundaries.push_back(bound);

      element = dynamic_cast<const xmlpp::Element*>(pair[1]);
      assert(element);
      ExpressionPtr weight = GetExpression(element);
      weights.push_back(weight);
    }
    expression = boost::shared_ptr<Histogram>(
        new Histogram(boundaries, weights));

  } else if (expr_name == "exponential") {
    assert(args.size() == 2);
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(args[0]);
    assert(element);
    ExpressionPtr lambda = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[1]);
    assert(element);
    ExpressionPtr time = GetExpression(element);

    expression = boost::shared_ptr<ExponentialExpression>(
        new ExponentialExpression(lambda, time));

  } else if (expr_name == "GLM") {
    assert(args.size() == 4);
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(args[0]);
    assert(element);
    ExpressionPtr gamma = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[1]);
    assert(element);
    ExpressionPtr lambda = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[2]);
    assert(element);
    ExpressionPtr mu = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[3]);
    assert(element);
    ExpressionPtr time = GetExpression(element);

    expression = boost::shared_ptr<GlmExpression>(
        new GlmExpression(gamma, lambda, mu, time));

  } else if (expr_name == "Weibull") {
    assert(args.size() == 4);
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(args[0]);
    assert(element);
    ExpressionPtr alpha = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[1]);
    assert(element);
    ExpressionPtr beta = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[2]);
    assert(element);
    ExpressionPtr t0 = GetExpression(element);

    element = dynamic_cast<const xmlpp::Element*>(args[3]);
    assert(element);
    ExpressionPtr time = GetExpression(element);

    expression = boost::shared_ptr<WeibullExpression>(
        new WeibullExpression(alpha, beta, t0, time));
  } else {
    return false;
  }
  return true;
}

void RiskAnalysis::RegisterCcfGroup(const xmlpp::Element* ccf_node) {
  std::string name = ccf_node->get_attribute_value("name");
  std::string id = name;
  boost::to_lower(id);

  if (ccf_groups_.count(id)) {
    std::stringstream msg;
    msg << "Line " << ccf_node->get_line() << ":\n";
    msg << "CCF group " << name << " is already defined.";
    throw ValidationError(msg.str());
  }
  std::string model = ccf_node->get_attribute_value("model");
  assert(model == "beta-factor" || model == "alpha-factor" ||
         model == "MGL" || model == "phi-factor");

  CcfGroupPtr ccf_group;
  if (model == "beta-factor") {
    ccf_group = CcfGroupPtr(new BetaFactorModel(name));

  } else if (model == "MGL") {
    ccf_group = CcfGroupPtr(new MglModel(name));

  } else if (model == "alpha-factor") {
    ccf_group = CcfGroupPtr(new AlphaFactorModel(name));

  } else if (model == "phi-factor") {
    ccf_group = CcfGroupPtr(new PhiFactorModel(name));
  }

  xmlpp::NodeSet members = ccf_node->find("./members");
  assert(members.size() == 1);
  const xmlpp::Element* element =
      dynamic_cast<const xmlpp::Element*>(members[0]);

  RiskAnalysis::ProcessCcfMembers(element, ccf_group);

  ccf_groups_.insert(std::make_pair(id, ccf_group));

  RiskAnalysis::AttachLabelAndAttributes(ccf_node, ccf_group);

  tbd_elements_.push_back(std::make_pair(ccf_group, ccf_node));
}

void RiskAnalysis::DefineCcfGroup(const xmlpp::Element* ccf_node,
                                  const CcfGroupPtr& ccf_group) {
  xmlpp::NodeSet children = ccf_node->find("./*");
  xmlpp::NodeSet::iterator it;
  for (it = children.begin(); it != children.end(); ++it) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*it);

    assert(element);
    std::string name = element->get_name();

    if (name == "distribution") {
      assert(element->find("./*").size() == 1);
      const xmlpp::Element* expr_node =
          dynamic_cast<const xmlpp::Element*>(element->find("./*")[0]);
      ExpressionPtr expression = RiskAnalysis::GetExpression(expr_node);
      ccf_group->AddDistribution(expression);

    } else if (name == "factor") {
      RiskAnalysis::DefineCcfFactor(element, ccf_group);

    } else if (name == "factors") {
      RiskAnalysis::ProcessCcfFactors(element, ccf_group);
    }
  }
}

void RiskAnalysis::ProcessCcfMembers(const xmlpp::Element* members_node,
                                     const CcfGroupPtr& ccf_group) {
  xmlpp::NodeSet children = members_node->find("./*");
  assert(!children.empty());
  std::set<std::string> member_ids;
  xmlpp::NodeSet::iterator it;
  for (it = children.begin(); it != children.end(); ++it) {
    const xmlpp::Element* event_node =
        dynamic_cast<const xmlpp::Element*>(*it);
    assert(event_node);
    assert("basic-event" == event_node->get_name());

    std::string name = event_node->get_attribute_value("name");
    boost::trim(name);
    std::string id = name;
    boost::to_lower(id);
    if (member_ids.count(id)) {
      std::stringstream msg;
      msg << "Line " << event_node->get_line() << ":\n";
      msg << name << " is already in CCF group " << ccf_group->name() << ".";
      throw ValidationError(msg.str());
    }
    if (gates_.count(id) || basic_events_.count(id) ||
        house_events_.count(id)) {
      std::stringstream msg;
      msg << "Line " << event_node->get_line() << ":\n";
      msg << name << " event is being redefined.";
      throw ValidationError(msg.str());
    }
    member_ids.insert(id);
    BasicEventPtr basic_event = BasicEventPtr(new BasicEvent(id));
    basic_event->name(name);
    basic_events_.insert(std::make_pair(id, basic_event));

    ccf_group->AddMember(basic_event);
  }
}

void RiskAnalysis::ProcessCcfFactors(const xmlpp::Element* factors_node,
                                     const CcfGroupPtr& ccf_group) {
  xmlpp::NodeSet children = factors_node->find("./*");
  assert(!children.empty());
  // To keep track of CCF group factor levels.
  xmlpp::NodeSet::iterator it;
  for (it = children.begin(); it != children.end(); ++it) {
    const xmlpp::Element* factor_node =
        dynamic_cast<const xmlpp::Element*>(*it);
    assert(factor_node);
    RiskAnalysis::DefineCcfFactor(factor_node, ccf_group);
  }
}

void RiskAnalysis::DefineCcfFactor(const xmlpp::Element* factor_node,
                                   const CcfGroupPtr& ccf_group) {
  // Checking the level for one factor input.
  std::string level = factor_node->get_attribute_value("level");
  boost::trim(level);
  if (level.empty()) {
    std::stringstream msg;
    msg << "Line " << factor_node->get_line() << ":\n";
    msg << "CCF group factor level number is not provided.";
    throw ValidationError(msg.str());
  }
  int level_num = boost::lexical_cast<int>(level);
  assert(factor_node->find("./*").size() == 1);
  const xmlpp::Element* expr_node =
      dynamic_cast<const xmlpp::Element*>(factor_node->find("./*")[0]);
  ExpressionPtr expression = RiskAnalysis::GetExpression(expr_node);
  try {
    ccf_group->AddFactor(expression, level_num);
  } catch (ValidationError& err) {
    std::stringstream msg;
    msg << "Line " << factor_node->get_line() << ":\n";
    msg << err.msg();
    throw ValidationError(msg.str());
  }
}

void RiskAnalysis::ValidateInitialization() {
  // Validation of essential members of analysis in the first layer.
  RiskAnalysis::CheckFirstLayer();

  // Validation of fault trees.
  RiskAnalysis::CheckSecondLayer();
}

void RiskAnalysis::CheckFirstLayer() {
  // Check if all gates have no cycles.
  boost::unordered_map<std::string, GatePtr>::iterator it;
  for (it = gates_.begin(); it != gates_.end(); ++it) {
    std::vector<std::string> cycle;
    if (cycle::DetectCycle<Gate, Formula>(&*it->second, &cycle)) {
      std::string msg = "Detected a cycle in " + it->second->name() +
                        " gate:\n";
      msg += cycle::PrintCycle(cycle);
      throw ValidationError(msg);
    }
  }
  std::stringstream error_messages;
  // Check if all primary events have expressions for probability analysis.
  if (settings_.probability_analysis_) {
    std::string msg = "";
    boost::unordered_map<std::string, BasicEventPtr>::iterator it_b;
    for (it_b = basic_events_.begin(); it_b != basic_events_.end(); ++it_b) {
      if (!it_b->second->has_expression()) msg += it_b->second->name() + "\n";
    }
    boost::unordered_map<std::string, HouseEventPtr>::iterator it_h;
    for (it_h = house_events_.begin(); it_h != house_events_.end(); ++it_h) {
      if (!it_h->second->has_expression()) msg += it_h->second->name() + "\n";
    }
    if (!msg.empty()) {
      error_messages << "\nThese primary events do not have expressions:\n"
                     << msg;
    }
  }

  if (!error_messages.str().empty()) {
    throw ValidationError(error_messages.str()); }

  RiskAnalysis::ValidateExpressions();
}

void RiskAnalysis::CheckSecondLayer() {
  if (!fault_trees_.empty()) {
    std::map<std::string, FaultTreePtr>::iterator it;
    for (it = fault_trees_.begin(); it != fault_trees_.end(); ++it) {
      it->second->Validate();
    }
  }

  if (!ccf_groups_.empty()) {
    std::map<std::string, CcfGroupPtr>::iterator it;
    for (it = ccf_groups_.begin(); it != ccf_groups_.end(); ++it) {
      it->second->Validate();
    }
  }
}

void RiskAnalysis::ValidateExpressions() {
  // Check for cycles in parameters. This must be done before expressions.
  if (!parameters_.empty()) {
    boost::unordered_map<std::string, ParameterPtr>::iterator it;
    for (it = parameters_.begin(); it != parameters_.end(); ++it) {
      std::vector<std::string> cycle;
      if (cycle::DetectCycle<Parameter, Expression>(&*it->second, &cycle)) {
        std::string msg = "Detected a cycle in " + it->second->name() +
                          " parameter:\n";
        msg += cycle::PrintCycle(cycle);
        throw ValidationError(msg);
      }
    }
  }

  // Validate expressions.
  if (!expressions_.empty()) {
    try {
      std::vector<ExpressionPtr>::iterator it;
      for (it = expressions_.begin(); it != expressions_.end(); ++it) {
        (*it)->Validate();
      }
    } catch (InvalidArgument& err) {
      throw ValidationError(err.msg());
    }
  }

  // Check probability values for primary events.
  if (settings_.probability_analysis_) {
    std::stringstream msg;
    msg << "";
    if (!ccf_groups_.empty()) {
      std::map<std::string, CcfGroupPtr>::iterator it;
      for (it = ccf_groups_.begin(); it != ccf_groups_.end(); ++it) {
        try {
          it->second->ValidateDistribution();
        } catch (ValidationError& err) {
          msg << it->second->name() << " : " << err.msg() << "\n";
        }
      }
    }
    boost::unordered_map<std::string, BasicEventPtr>::iterator it;
    for (it = basic_events_.begin(); it != basic_events_.end(); ++it) {
      try {
        it->second->Validate();
      } catch (ValidationError& err) {
        msg << it->second->name() << " : " << err.msg() << "\n";
      }
    }
    if (!msg.str().empty()) {
      std::string head = "Invalid probabilities detected:\n";
      throw ValidationError(head + msg.str());
    }
  }
}

void RiskAnalysis::SetupForAnalysis() {
  // CCF groups.
  if (!ccf_groups_.empty()) {
    std::map<std::string, CcfGroupPtr>::iterator it;
    for (it = ccf_groups_.begin(); it != ccf_groups_.end(); ++it) {
      it->second->ApplyModel();
    }
  }
}

}  // namespace scram
