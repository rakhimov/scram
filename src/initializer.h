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

/// @file initializer.h
/// A facility that processes input files into analysis constructs.

#ifndef SCRAM_SRC_INITIALIZER_H_
#define SCRAM_SRC_INITIALIZER_H_

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <libxml++/libxml++.h>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "expression.h"
#include "fault_tree.h"
#include "model.h"
#include "settings.h"
#include "xml_parser.h"

namespace scram {
namespace mef {

/// This class operates on input files
/// to initialize analysis constructs
/// like models, fault trees, and events.
/// The initialization phase includes
/// validation and proper setup of the constructs
/// for future use or analysis.
class Initializer {
 public:
  /// Prepares common information to be used by
  /// the future input file constructs,
  /// for example, mission time and validation schema.
  ///
  /// @param[in] settings  Analysis settings.
  explicit Initializer(const core::Settings& settings);

  Initializer(const Initializer&) = delete;
  Initializer& operator=(const Initializer&) = delete;

  /// Reads input files with the structure of analysis constructs.
  /// Initializes the analysis model from the given input files.
  /// Puts all events into their appropriate containers in the model.
  ///
  /// @param[in] xml_files  The formatted XML input files.
  ///
  /// @throws DuplicateArgumentError  Input contains duplicate files.
  /// @throws ValidationError  The input contains errors.
  /// @throws IOError  One of the input files is not accessible.
  void ProcessInputFiles(const std::vector<std::string>& xml_files);

  /// @returns The model built from the input files.
  std::shared_ptr<Model> model() const { return model_; }

 private:
  /// Convenience alias for expression extractor function types.
  using ExtractorFunction = std::function<ExpressionPtr(const xmlpp::NodeSet&,
                                                        const std::string&,
                                                        Initializer*)>;
  /// Map of expression names and their extractor functions.
  using ExtractorMap = std::unordered_map<std::string, ExtractorFunction>;

  /// Map of valid units for parameters.
  static const std::map<std::string, Units> kUnits_;
  /// String representation of units.
  static const char* const kUnitToString_[];
  /// Expressions mapped to their extraction functions.
  static const ExtractorMap kExpressionExtractors_;

  /// @tparam T  Type of an expression.
  /// @tparam N  The number of arguments for the expression.
  ///
  /// Extracts argument expressions from XML elements
  /// and constructs the requested expression T.
  template <class T, int N>
  struct Extractor;

  /// Checks if all input files exist on the system.
  ///
  /// @param[in] xml_files  The XML input files.
  ///
  /// @throws IOError  Some files are missing.
  void CheckFileExistence(const std::vector<std::string>& xml_files);

  /// Checks if there are duplicate input files.
  ///
  /// @param[in] xml_files  The XML input files.
  ///
  /// @pre All input files exist on the system.
  ///
  /// @throws DuplicateArgumentError  There are duplicate input files.
  void CheckDuplicateFiles(const std::vector<std::string>& xml_files);

  /// Reads one input file with the structure of analysis entities.
  /// Initializes the analysis from the given input file.
  /// Puts all events into their appropriate containers.
  /// This function mostly registers element definitions,
  /// but it may leave them to be defined later
  /// because of possible undefined dependencies of those elements.
  ///
  /// @param[in] xml_file  The formatted XML input file.
  ///
  /// @pre The input file has not been passed before.
  ///
  /// @throws ValidationError  The input contains errors.
  /// @throws IOError  The input file is not accessible.
  void ProcessInputFile(const std::string& xml_file);

  /// Processes definitions of elements
  /// that are left to be determined later.
  /// This late definition happens primarily due to unregistered dependencies.
  ///
  /// @throws ValidationError  The elements contain undefined dependencies.
  void ProcessTbdElements();

  /// Attaches attributes and a label to the elements of the analysis.
  /// These attributes are not XML attributes
  /// but OpenPSA format defined arbitrary attributes
  /// and a label that can be attached to many analysis elements.
  ///
  /// @param[in] element_node  XML element.
  /// @param[out] element  The object that needs attributes and label.
  void AttachLabelAndAttributes(const xmlpp::Element* element_node,
                                Element* element);

  /// Defines a fault tree for the analysis.
  ///
  /// @param[in] ft_node  XML element defining the fault tree.
  ///
  /// @throws ValidationError  There are issues with registering and defining
  ///                          the fault tree and its data
  ///                          like gates and events.
  void DefineFaultTree(const xmlpp::Element* ft_node);

  /// Defines a component container.
  ///
  /// @param[in] component_node  XML element defining the component.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in] container_role  The parent container's role.
  ///
  /// @returns Component that is ready for registration.
  ///
  /// @throws ValidationError  There are issues with registering and defining
  ///                          the component and its data
  ///                          like gates and events.
  ComponentPtr DefineComponent(const xmlpp::Element* component_node,
                               const std::string& base_path,
                               RoleSpecifier container_role);

  /// Registers fault tree and component data
  /// like gates, events, parameters.
  ///
  /// @param[in] ft_node  XML element defining the fault tree or component.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] component  The component or fault tree container
  ///                          that is the owner of the data.
  ///
  /// @throws ValidationError  There are issues with registering and defining
  ///                          the component's data like gates and events.
  void RegisterFaultTreeData(const xmlpp::Element* ft_node,
                             const std::string& base_path,
                             Component* component);

  /// Processes model data with definitions of events and analysis.
  ///
  /// @param[in] model_data  XML node with model data description.
  void ProcessModelData(const xmlpp::Element* model_data);

  /// Registers a gate for later definition.
  ///
  /// @param[in] gate_node  XML element defining the gate.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in] container_role  The parent container's role.
  ///
  /// @returns Pointer to the registered gate.
  ///
  /// @throws ValidationError  An event with the same name is already defined.
  GatePtr RegisterGate(const xmlpp::Element* gate_node,
                       const std::string& base_path,
                       RoleSpecifier container_role);

  /// Defines a gate for this analysis.
  ///
  /// @param[in] gate_node  XML element defining the gate.
  /// @param[in,out] gate  Registered gate ready to be defined.
  void DefineGate(const xmlpp::Element* gate_node, Gate* gate);

  /// Creates a Boolean formula from the XML elements
  /// describing the formula with events and other nested formulas.
  ///
  /// @param[in] formula_node  XML element defining the formula.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  ///
  /// @returns Boolean formula that is defined.
  ///
  /// @throws ValidationError  The defined formula is not valid.
  FormulaPtr GetFormula(const xmlpp::Element* formula_node,
                        const std::string& base_path);

  /// Processes the arguments of a formula with nodes and formulas.
  ///
  /// @param[in] formula_node  The XML element with children as arguments.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] formula  The formula to be defined by the arguments.
  ///
  /// @throws ValidationError  Repeated arguments are identified.
  void ProcessFormula(const xmlpp::Element* formula_node,
                      const std::string& base_path,
                      Formula* formula);

  /// Registers a basic event for later definition.
  ///
  /// @param[in] event_node  XML element defining the event.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in] container_role  The parent container's role.
  ///
  /// @returns Pointer to the registered basic event.
  ///
  /// @throws ValidationError  An event with the same name is already defined.
  BasicEventPtr RegisterBasicEvent(const xmlpp::Element* event_node,
                                   const std::string& base_path,
                                   RoleSpecifier container_role);

  /// Defines a basic event for this analysis.
  ///
  /// @param[in] event_node  XML element defining the event.
  /// @param[in,out] basic_event  Registered basic event ready to be defined.
  void DefineBasicEvent(const xmlpp::Element* event_node,
                        BasicEvent* basic_event);

  /// Defines and adds a house event for this analysis.
  ///
  /// @param[in] event_node  XML element defining the event.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in] container_role  The parent container's role.
  ///
  /// @returns Pointer to the registered house event.
  ///
  /// @throws ValidationError  An event with the same name is already defined.
  HouseEventPtr DefineHouseEvent(const xmlpp::Element* event_node,
                                 const std::string& base_path,
                                 RoleSpecifier container_role);

  /// Registers a variable or parameter.
  ///
  /// @param[in] param_node  XML element defining the parameter.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in] container_role  The parent container's role.
  ///
  /// @returns Pointer to the registered parameter.
  ///
  /// @throws ValidationError  The parameter is already registered.
  ParameterPtr RegisterParameter(const xmlpp::Element* param_node,
                                 const std::string& base_path,
                                 RoleSpecifier container_role);

  /// Defines a variable or parameter.
  ///
  /// @param[in] param_node  XML element defining the parameter.
  /// @param[in,out] parameter  Registered parameter to be defined.
  void DefineParameter(const xmlpp::Element* param_node, Parameter* parameter);

  /// Processes Expression definitions in input file.
  ///
  /// @param[in] expr_element  XML expression element containing the definition.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  ///
  /// @returns Pointer to the newly defined or registered expression.
  ///
  /// @throws ValidationError  There are problems with getting the expression.
  ExpressionPtr GetExpression(const xmlpp::Element* expr_element,
                              const std::string& base_path);

  /// Processes Constant Expression definitions in input file.
  ///
  /// @param[in] expr_element  XML expression element containing the definition.
  ///
  /// @returns Expression described in XML input expression node.
  ExpressionPtr GetConstantExpression(const xmlpp::Element* expr_element);

  /// Processes Parameter Expression definitions in input file.
  ///
  /// @param[in] expr_element  XML expression element containing the definition.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  ///
  /// @returns Parameter expression described in XML input expression node.
  ///
  /// @throws ValidationError  The parameter variable is not reachable.
  ExpressionPtr GetParameterExpression(const xmlpp::Element* expr_element,
                                       const std::string& base_path);

  /// Registers a common cause failure group for later definition.
  ///
  /// @param[in] ccf_node  XML element defining CCF group.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in] container_role  The parent container's role.
  ///
  /// @returns Pointer to the registered CCF group.
  ///
  /// @throws ValidationError  There are problems with registering
  ///                          the group and its members,
  ///                          for example, duplications or missing information.
  CcfGroupPtr RegisterCcfGroup(const xmlpp::Element* ccf_node,
                               const std::string& base_path,
                               RoleSpecifier container_role);

  /// Defines a common cause failure group for the analysis.
  ///
  /// @param[in] ccf_node  XML element defining CCF group.
  /// @param[in,out] ccf_group  Registered CCF group to be defined.
  void DefineCcfGroup(const xmlpp::Element* ccf_node, CcfGroup* ccf_group);

  /// Processes common cause failure group members as defined basic events.
  ///
  /// @param[in] members_node  XML element containing all members.
  /// @param[in,out] ccf_group  CCF group of the given members.
  ///
  /// @throws ValidationError  Members are redefined,
  ///                          or there are other setup issues
  ///                          with the CCF group.
  void ProcessCcfMembers(const xmlpp::Element* members_node,
                         CcfGroup* ccf_group);

  /// Defines factor and adds it to CCF group.
  ///
  /// @param[in] factor_node  XML element containing one factor.
  /// @param[in,out] ccf_group  CCF group to be defined by the given factors.
  ///
  /// @throws ValidationError  There are problems with level numbers
  ///                          or factors for specific CCF models.
  void DefineCcfFactor(const xmlpp::Element* factor_node, CcfGroup* ccf_group);

  /// Validates if the initialization of the analysis is successful.
  /// This validation process also generates optional warnings.
  ///
  /// @throws ValidationError  The initialization contains mistakes.
  void ValidateInitialization();

  /// Validates expressions and anything
  /// that is dependent on them,
  /// such as parameters and basic events.
  ///
  /// @throws ValidationError  There are problems detected with expressions.
  void ValidateExpressions();

  /// Applies the input information to set up for future analysis.
  /// This step is crucial to get
  /// correct fault tree structures
  /// and basic events with correct expressions.
  /// Meta-logical layer of analysis,
  /// such as CCF groups and substitutions,
  /// is applied to analysis.
  void SetupForAnalysis();

  std::shared_ptr<Model> model_;  ///< Analysis model with constructs.
  core::Settings settings_;  ///< Settings for analysis.
  std::shared_ptr<MissionTime> mission_time_;  ///< Mission time expression.

  /// The main schema for validation.
  static std::stringstream schema_;

  /// Parsers with all documents saved for later access.
  std::vector<std::unique_ptr<XmlParser>> parsers_;

  /// Map roots of documents to files. This is for error reporting.
  std::map<const xmlpp::Node*, std::string> doc_to_file_;

  /// Collection of elements that are defined late
  /// because of unordered registration and definition of their dependencies.
  struct {
    /// Parameters rely on parameter registration.
    std::vector<std::pair<Parameter*, const xmlpp::Element*>> parameters;
    /// Basic events rely on parameter registration.
    std::vector<std::pair<BasicEvent*, const xmlpp::Element*>> basic_events;
    /// Gates rely on gate, basic event, and house event registrations.
    std::vector<std::pair<Gate*, const xmlpp::Element*>> gates;
    /// CCF groups rely on both parameter and basic event registration.
    std::vector<std::pair<CcfGroup*, const xmlpp::Element*>> ccf_groups;
  } tbd_;  ///< Elements are assumed to be unique.

  /// Container for defined expressions for later validation.
  std::vector<Expression*> expressions_;
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_INITIALIZER_H_
