/// @file initializer.h
/// A facility that processes input files into analysis constructs.
#ifndef SCRAM_SRC_INITIALIZER_H_
#define SCRAM_SRC_INITIALIZER_H_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <libxml++/libxml++.h>

#include "event.h"
#include "model.h"
#include "settings.h"

namespace scram {

class Element;
class FaultTree;
class Component;
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
  explicit Initializer(const Settings& settings);

  /// Reads input files with the structure of analysis constructs.
  /// Initializes the analysis model from the given input files.
  /// Puts all events into their appropriate containers in the model.
  /// @param[in] xml_files The formatted XML input files.
  /// @throws ValidationError if input contains errors.
  /// @throws ValueError if input values are not valid.
  /// @throws IOError if an input file is not accessible.
  void ProcessInputFiles(const std::vector<std::string>& xml_files);

  /// @returns The model build from the input files.
  inline ModelPtr model() const { return model_; }

 private:
  typedef boost::shared_ptr<Element> ElementPtr;
  typedef boost::shared_ptr<Event> EventPtr;
  typedef boost::shared_ptr<Gate> GatePtr;
  typedef boost::shared_ptr<Formula> FormulaPtr;
  typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
  typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
  typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
  typedef boost::shared_ptr<CcfGroup> CcfGroupPtr;
  typedef boost::shared_ptr<FaultTree> FaultTreePtr;
  typedef boost::shared_ptr<Component> ComponentPtr;
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
  void ProcessInputFile(const std::string& xml_file);

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
  /// @throws ValidationError if there are issues with registering and defining
  ///                         the fault tree and its data like gates and events.
  void DefineFaultTree(const xmlpp::Element* ft_node);

  /// Defines a component container.
  /// @param[in] component_node XML element defining the component.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[in] public_container A flag for the parent container's role.
  /// @returns Component that is ready for registration.
  /// @throws ValidationError if there are issues with registering and defining
  ///                         the component and its data like gates and events.
  ComponentPtr DefineComponent(const xmlpp::Element* component_node,
                               const std::string& base_path,
                               bool public_container = true);

  /// Registers fault tree and component data like gates, events, parameters.
  /// @param[in] ft_node XML element defining the fault tree or component.
  /// @param[in/out] component The component or fault tree container that is
  ///                          the owner of the data.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @throws ValidationError if there are issues with registering and defining
  ///                         the component's data like gates and events.
  void RegisterFaultTreeData(const xmlpp::Element* ft_node,
                             const ComponentPtr& component,
                             const std::string& base_path);

  /// Processes model data with definitions of events and analysis.
  /// @param[in] model_data XML node with model data description.
  void ProcessModelData(const xmlpp::Element* model_data);

  /// Registers a gate for later definition.
  /// @param[in] gate_node XML element defining the gate.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[in] public_container A flag for the parent container's role.
  /// @returns Pointer to the registered gate.
  /// @throws ValidationError if an event with the same name is already defined.
  GatePtr RegisterGate(const xmlpp::Element* gate_node,
                       const std::string& base_path = "",
                       bool public_container = true);

  /// Defines a gate for this analysis.
  /// @param[in] gate_node XML element defining the gate.
  /// @param[in,out] gate Registered gate ready to be defined.
  void DefineGate(const xmlpp::Element* gate_node, const GatePtr& gate);

  /// Creates a Boolean formula from the XML elements describing the formula
  /// with events and other nested formulas.
  /// @param[in] gate_node XML element defining the formula.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @returns Boolean formula that is defined.
  /// @throws ValidationError if the defined formula is not valid.
  FormulaPtr GetFormula(const xmlpp::Element* formula_node,
                        const std::string& base_path);

  /// Processes the arguments of a formula with nodes and formulas.
  /// @param[in] formula_node The XML element with children as arguments.
  /// @param[in/out] formula The formula to be defined by the arguments.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @throws ValidationError if repeated arguments are identified.
  void ProcessFormula(const xmlpp::Element* formula_node,
                      const FormulaPtr& formula,
                      const std::string& base_path);

  /// Registers a basic event for later definition.
  /// @param[in] event_node XML element defining the event.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[in] public_container A flag for the parent container's role.
  /// @returns Pointer to the registered basic event.
  /// @throws ValidationError if an event with the same name is already defined.
  BasicEventPtr RegisterBasicEvent(const xmlpp::Element* event_node,
                                   const std::string& base_path = "",
                                   bool public_container = true);

  /// Defines a basic event for this analysis.
  /// @param[in] event_node XML element defining the event.
  /// @param[in,out] basic_event Registered basic event ready to be defined.
  void DefineBasicEvent(const xmlpp::Element* event_node,
                        const BasicEventPtr& basic_event);

  /// Defines and adds a house event for this analysis.
  /// @param[in] event_node XML element defining the event.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[in] public_container A flag for the parent container's role.
  /// @returns Pointer to the registered house event.
  /// @throws ValidationError if an event with the same name is already defined.
  HouseEventPtr DefineHouseEvent(const xmlpp::Element* event_node,
                                 const std::string& base_path = "",
                                 bool public_container = true);

  /// Registers a variable or parameter.
  /// @param[in] param_node XML element defining the parameter.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[in] public_container A flag for the parent container's role.
  /// @returns Pointer to the registered parameter.
  /// @throws ValidationError if the parameter is already registered.
  ParameterPtr RegisterParameter(const xmlpp::Element* param_node,
                                 const std::string& base_path = "",
                                 bool public_container = true);

  /// Defines a variable or parameter.
  /// @param[in] param_node XML element defining the parameter.
  /// @param[in,out] parameter Registered parameter to be defined.
  void DefineParameter(const xmlpp::Element* param_node,
                       const ParameterPtr& parameter);

  /// Processes Expression definitions in input file.
  /// @param[in] expr_element XML expression element containing the definition.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @throws ValidationError if there are problems with getting the expression.
  ExpressionPtr GetExpression(const xmlpp::Element* expr_element,
                              const std::string& base_path);

  /// Processes Constant Expression definitions in input file.
  /// @param[in] expr_element XML expression element containing the definition.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[out] expression Expression described in XML input expression node.
  /// @returns true if expression was found and processed.
  bool GetConstantExpression(const xmlpp::Element* expr_element,
                             const std::string& base_path,
                             ExpressionPtr& expression);

  /// Processes Parameter Expression definitions in input file.
  /// @param[in] expr_element XML expression element containing the definition.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[out] expression Expression described in XML input expression node.
  /// @returns true if expression was found and processed.
  /// @throws ValidationError if the parameter variable is not reachable.
  bool GetParameterExpression(const xmlpp::Element* expr_element,
                              const std::string& base_path,
                              ExpressionPtr& expression);

  /// Processes Distribution deviate expression definitions in input file.
  /// @param[in] expr_element XML expression element containing the definition.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[out] expression Expression described in XML input expression node.
  /// @returns true if expression was found and processed.
  bool GetDeviateExpression(const xmlpp::Element* expr_element,
                            const std::string& base_path,
                            ExpressionPtr& expression);

  /// Registers a common cause failure group for later definition.
  /// @param[in] ccf_node XML element defining CCF group.
  /// @param[in] base_path Series of ancestor containers in the path with dots.
  /// @param[in] public_container A flag for the parent container's role.
  /// @returns Pointer to the registered CCF group.
  /// @throws ValidationError for problems with registering the group and
  ///         its members, for example, duplication or missing information.
  CcfGroupPtr RegisterCcfGroup(const xmlpp::Element* ccf_node,
                               const std::string& base_path = "",
                               bool public_container = true);

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
  /// @throws ValidationError if there are problems with level numbers or
  ///         factors for specific CCF models.
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

  /// Map roots of documents to files. This is for error reporting.
  std::map<const xmlpp::Node*, std::string> doc_to_file_;

  /// Elements that are defined on the second pass.
  std::vector<std::pair<ElementPtr, const xmlpp::Element*> > tbd_elements_;

  /// Container for defined expressions for later validation.
  std::vector<ExpressionPtr> expressions_;
};

}  // namespace scram

#endif  // SCRAM_SRC_INITIALIZER_H_
