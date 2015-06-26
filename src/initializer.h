/// @file initializer.h
/// A facility that processes input files into analysis constructs.
#ifndef SCRAM_SRC_INITIALIZER_H_
#define SCRAM_SRC_INITIALIZER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <libxml++/libxml++.h>

#include "event.h"
#include "model.h"
#include "settings.h"

namespace scram {

class Element;
class FaultTree;
class CcfGroup;
class Expression;
class Formula;
class XMLParser;
enum Units;

/// @class Initializer
/// This class operates on input files to initialize analysis constructs like
/// models, fault trees, and events. The initialization phase includes
/// validation and proper setup of the constructs for future use or analysis.
class Initializer {
 public:
  typedef boost::shared_ptr<Model> ModelPtr;

  /// Prepares common information to be used by the future input file
  /// constructs, for example, mission time.
  /// @param[in] settings Analysis settings.
  Initializer(const Settings& settings);

  /// Reads input files with the structure of analysis constructs.
  /// Initializes the analysis model from the given input files.
  /// Puts all events into their appropriate containers in the model.
  /// @param[in] xml_files The formatted XML input files.
  /// @throws ValidationError if input contains errors.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if an input file is not accessible.
  void ProcessInputFiles(const std::vector<std::string>& xml_files);

  /// @returns The model build from the input files.
  inline ModelPtr model() { return model_; }

 private:
  typedef boost::shared_ptr<Element> ElementPtr;
  typedef boost::shared_ptr<Event> EventPtr;
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<Formula> FormulaPtr;
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<FaultTree> FaultTreePtr;
  typedef boost::shared_ptr<CcfGroup> CcfGroupPtr;
  typedef boost::shared_ptr<Expression> ExpressionPtr;
  typedef boost::shared_ptr<Parameter> ParameterPtr;

  /// Map of valid units for parameters.
  static const std::map<std::string, Units> units_;
  /// String representation of units.
  static const char* const unit_to_string_[];

  /// Reads one input file with the structure of analysis entities.
  /// Initializes the analysis from the given input file.
  /// Puts all events into their appropriate containers.
  /// This function mostly registers element definitions, but it may leave
  /// them to be defined later because of possible undefined dependencies of
  /// those elements.
  /// @param[in] xml_file The formatted XML input file.
  /// @throws ValidationError if input contains errors.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if an input file is not accessible.
  void ProcessInputFile(std::string xml_file);

  /// Processes definitions of elements that are left to be determined later.
  /// @throws ValidationError if elements contain undefined dependencies.
  void ProcessTbdElements();

  /// Attaches attributes and label to the elements of the analysis.
  /// These attributes are not XML attributes but OpenPSA format defined
  /// arbitrary attributes and label that can be attached to many analysis
  /// elements.
  /// @param[in] element_node XML element.
  /// @param[out] element The object that needs attributes and label.
  void AttachLabelAndAttributes(const xmlpp::Element* element_node,
                                const ElementPtr& element);

  /// Defines a fault tree for the analysis.
  /// @param[in] ft_node XML element defining the fault tree.
  void DefineFaultTree(const xmlpp::Element* ft_node);

  /// Processes model data with definitions of events and analysis.
  /// @param[in] model_data XML node with model data description.
  void ProcessModelData(const xmlpp::Element* model_data);

  /// Registers a gate for later definition.
  /// @param[in] gate_node XML element defining the gate.
  /// @param[in,out] ft FaultTree under which this gate belongs to.
  /// @throws ValidationError if an event with the same name is already defined.
  void RegisterGate(const xmlpp::Element* gate_node, const FaultTreePtr& ft);

  /// Defines a gate for this analysis.
  /// @param[in] gate_node XML element defining the gate.
  /// @param[in,out] gate Registered gate ready to be defined.
  void DefineGate(const xmlpp::Element* gate_node, const GatePtr& gate);

  /// Creates a Boolean formula from the XML elements describing the formula
  /// with events and other nested formulas.
  /// @param[in] gate_node XML element defining the formula.
  /// @returns Boolean formula that is registered.
  /// @throws ValidationError if the defined formula is not valid.
  FormulaPtr GetFormula(const xmlpp::Element* formula_node);

  /// Processes the arguments of a formula with nodes and formulas.
  /// @param[in] formula_node The XML element with children as arguments.
  /// @param[in/out] formula The formula to be defined by the arguments.
  /// @throws ValidationError if repeated arguments are identified.
  void ProcessFormula(const xmlpp::Element* formula_node,
                      const FormulaPtr& formula);

  /// Process [event name=id] cases inside of a one layer formula description.
  /// @param[in] event XML element defining this event.
  /// @param[out] child The child the currently processed formula.
  void ProcessFormulaEvent(const xmlpp::Element* event, EventPtr& child);

  /// Process [basic-event name=id] cases inside of a one layer
  /// formula description.
  /// @param[in] event XML element defining this event.
  /// @param[out] child The child the currently processed formula.
  void ProcessFormulaBasicEvent(const xmlpp::Element* event, EventPtr& child);

  /// Process [house-event name=id] cases inside of a one layer
  /// formula description.
  /// @param[in] event XML element defining this event.
  /// @param[out] child The child the currently processed formula.
  void ProcessFormulaHouseEvent(const xmlpp::Element* event, EventPtr& child);

  /// Process [gate name=id]cases inside of a one layer
  /// formula description.
  /// @param[in] event XML element defining this event.
  /// @param[out] child The child the currently processed formula.
  void ProcessFormulaGate(const xmlpp::Element* event, EventPtr& child);

  /// Registers a basic event for later definition.
  /// @param[in] event_node XML element defining the event.
  /// @throws ValidationError if an event with the same name is already defined.
  void RegisterBasicEvent(const xmlpp::Element* event_node);

  /// Defines a basic event for this analysis.
  /// @param[in] event_node XML element defining the event.
  /// @param[in,out] basic_event Registered basic event ready to be defined.
  void DefineBasicEvent(const xmlpp::Element* event_node,
                        const BasicEventPtr& basic_event);

  /// Defines and adds a house event for this analysis.
  /// @param[in] event_node XML element defining the event.
  /// @throws ValidationError if an event with the same name is already defined.
  void DefineHouseEvent(const xmlpp::Element* event_node);

  /// Registers a variable or parameter.
  /// @param[in] param_node XML element defining the parameter.
  /// @throws ValidationError if the parameter is already registered.
  void RegisterParameter(const xmlpp::Element* param_node);

  /// Defines a variable or parameter.
  /// @param[in] param_node XML element defining the parameter.
  /// @param[in,out] parameter Registered parameter to be defined.
  void DefineParameter(const xmlpp::Element* param_node,
                       const ParameterPtr& parameter);

  /// Processes Expression definitions in input file.
  /// @param[in] expr_element XML expression element containing the definition.
  ExpressionPtr GetExpression(const xmlpp::Element* expr_element);

  /// Processes Constant Expression definitions in input file.
  /// @param[in] expr_element XML expression element containing the definition.
  /// @param[out] expression Expression described in XML input expression node.
  /// @returns true if expression was found and processed.
  bool GetConstantExpression(const xmlpp::Element* expr_element,
                             ExpressionPtr& expression);

  /// Processes Parameter Expression definitions in input file.
  /// @param[in] expr_element XML expression element containing the definition.
  /// @param[out] expression Expression described in XML input expression node.
  /// @returns true if expression was found and processed.
  bool GetParameterExpression(const xmlpp::Element* expr_element,
                              ExpressionPtr& expression);

  /// Processes Distribution deviate expression definitions in input file.
  /// @param[in] expr_element XML expression element containing the definition.
  /// @param[out] expression Expression described in XML input expression node.
  /// @returns true if expression was found and processed.
  bool GetDeviateExpression(const xmlpp::Element* expr_element,
                            ExpressionPtr& expression);

  /// Registers a common cause failure group for later definition.
  /// @param[in] ccf_node XML element defining CCF group.
  void RegisterCcfGroup(const xmlpp::Element* ccf_node);

  /// Defines a common cause failure group for the analysis.
  /// @param[in] ccf_node XML element defining CCF group.
  /// @param[in,out] ccf_group Registered CCF group to be defined.
  void DefineCcfGroup(const xmlpp::Element* ccf_node,
                      const CcfGroupPtr& ccf_group);

  /// Processes common cause failure group members as defined basic events.
  /// @param[in] members_node XML element containing all members.
  /// @param[in,out] ccf_group CCF group of the given members.
  /// @throws ValidationError if members are redefined, or there are other
  ///                         setup issues with the CCF group.
  void ProcessCcfMembers(const xmlpp::Element* members_node,
                         const CcfGroupPtr& ccf_group);

  /// Attaches factors to a given common cause failure group.
  /// @param[in] factors_node XML element containing all factors.
  /// @param[in,out] ccf_group CCF group to be defined by the given factors.
  void ProcessCcfFactors(const xmlpp::Element* factors_node,
                         const CcfGroupPtr& ccf_group);

  /// Defines factor and adds it to CCF group.
  /// @param[in] factor_node XML element containing one factor.
  /// @param[in,out] ccf_group CCF group to be defined by the given factors.
  void DefineCcfFactor(const xmlpp::Element* factor_node,
                       const CcfGroupPtr& ccf_group);

  /// Validates if the initialization of the analysis is successful.
  /// This validation process also generates optional warnings.
  /// @throws ValidationError if the initialization contains mistakes.
  void ValidateInitialization();

  /// Checks for problems with gates, events, expressions, and parameters.
  /// @throws ValidationError if the first layer members contain mistakes.
  void CheckFirstLayer();

  /// Checks for problems with analysis containers, such as fault trees,
  /// event trees, common cause groups, and others that use the first layer
  /// members.
  /// @throws ValidationError if the second layer members contain mistakes.
  void CheckSecondLayer();

  /// Validates expressions and anything that is dependent on them, such
  /// as parameters and basic events.
  /// @throws ValidationError if any problems detected with expressions.
  void ValidateExpressions();

  /// Applies the input information to set up for future analysis.
  /// This step is crucial to get correct fault tree structures and
  /// basic events with correct expressions.
  /// Meta-logical layer of analysis, such as CCF groups and substitutions,
  /// is applied to analysis.
  void SetupForAnalysis();

  ModelPtr model_;  ///< Analysis model with constructs.
  Settings settings_;  ///< Settings for analysis.
  boost::shared_ptr<MissionTime> mission_time_;  ///< Mission time expression.

  /// Parsers with all documents saved for later access.
  std::vector<boost::shared_ptr<XMLParser> > parsers_;

  /// Elements that are defined on the second pass.
  std::vector<std::pair<ElementPtr, const xmlpp::Element*> > tbd_elements_;

  /// Container for defined expressions for later validation.
  std::vector<ExpressionPtr> expressions_;
};

}  // namespace scram

#endif  // SCRAM_SRC_INITIALIZER_H_
