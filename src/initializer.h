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

/// @file initializer.h
/// A facility that processes input files into analysis constructs.

#ifndef SCRAM_SRC_INITIALIZER_H_
#define SCRAM_SRC_INITIALIZER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/variant.hpp>
#include <libxml++/libxml++.h>

#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "event_tree.h"
#include "expression.h"
#include "expression/constant.h"
#include "fault_tree.h"
#include "model.h"
#include "parameter.h"
#include "settings.h"

namespace scram {
namespace mef {

/// This class operates on input files
/// to initialize analysis constructs
/// like models, fault trees, and events.
/// The initialization phase includes
/// validation and proper setup of the constructs
/// for future use or analysis.
class Initializer : private boost::noncopyable {
 public:
  /// Reads input files with the structure of analysis constructs.
  /// Initializes the analysis model from the given input files.
  /// Puts all events into their appropriate containers in the model.
  ///
  /// @param[in] xml_files  The MEF XML input files.
  /// @param[in] settings  Analysis settings.
  ///
  /// @throws DuplicateArgumentError  Input contains duplicate files.
  /// @throws ValidationError  The input contains errors.
  /// @throws IOError  One of the input files is not accessible.
  Initializer(const std::vector<std::string>& xml_files,
              core::Settings settings);

  /// @returns The model built from the input files.
  std::shared_ptr<Model> model() const { return model_; }

 private:
  /// Convenience alias for expression extractor function types.
  using ExtractorFunction = std::unique_ptr<Expression> (*)(
      const xmlpp::NodeSet&, const std::string&, Initializer*);
  /// Map of expression names and their extractor functions.
  using ExtractorMap = std::unordered_map<std::string, ExtractorFunction>;
  /// Container for late defined constructs.
  template <class... Ts>
  using TbdContainer =
      std::vector<std::pair<boost::variant<Ts*...>, const xmlpp::Element*>>;

  /// Expressions mapped to their extraction functions.
  static const ExtractorMap kExpressionExtractors_;

  /// @tparam T  Type of an expression.
  /// @tparam N  The number of arguments for the expression.
  ///
  /// Extracts argument expressions from XML elements
  /// and constructs the requested expression T.
  template <class T, int N>
  struct Extractor;

  /// Calls Extractor with an appropriate N to construct the expression.
  ///
  /// @tparam T  Type of an expression.
  ///
  /// @param[in] args  A vector of XML elements containing the arguments.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] init  The host Initializer.
  ///
  /// @returns The new extracted expression.
  template <class T>
  static std::unique_ptr<Expression> Extract(const xmlpp::NodeSet& args,
                                             const std::string& base_path,
                                             Initializer* init);

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

  /// @copybrief Initializer::Initializer
  ///
  /// @param[in] xml_files  The formatted XML input files.
  ///
  /// @throws DuplicateArgumentError  Input contains duplicate files.
  /// @throws ValidationError  The input contains errors.
  /// @throws IOError  One of the input files is not accessible.
  void ProcessInputFiles(const std::vector<std::string>& xml_files);

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

  /// Registers an element into the model.
  ///
  /// @tparam T  A pointer type to the element.
  ///
  /// @param[in] element  The initialized element ready to be added into models.
  /// @param[in] xml_element  The related XML element for error messages.
  ///
  /// @throws ValidationError  Issues with adding the element into the model.
  template <class T>
  void Register(T&& element, const xmlpp::Element* xml_element);

  /// Constructs and registers an element in the model.
  ///
  /// @tparam T  The Element type to be specialized for.
  ///
  /// @param[in] xml_node  XML node defining the element.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in] base_role  The parent container's role.
  ///
  /// @returns Pointer to the registered element.
  ///
  /// @throws ValidationError  Issues with the new element or registration.
  template <class T>
  std::shared_ptr<T> Register(const xmlpp::Element* xml_node,
                              const std::string& base_path,
                              RoleSpecifier base_role);

  /// Adds additional data to element definition
  /// after processing all the input files.
  ///
  /// @tparam T  The Element type to be specialized for.
  ///
  /// @param[in] xml_node  XML element defining the element.
  /// @param[in,out] element  Registered element ready to be defined.
  ///
  /// @throws ValidationError  Issues with the additional data.
  template <class T>
  void Define(const xmlpp::Element* xml_node, T* element);

  /// Defines an event tree for the analysis.
  ///
  /// @param[in] et_node  XML element defining the event tree.
  ///
  /// @throws ValidationError  There are issues with registering and defining
  ///                          the event tree and its data.
  void DefineEventTree(const xmlpp::Element* et_node);

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

  /// Processes event tree branch instructions and target from XML data.
  ///
  /// @param[in] xml_nodes  The XML element data of the branch.
  /// @param[in,out] event_tree  The host event tree as a context container.
  /// @param[out] branch  The branch to be defined with instructions and target.
  ///
  /// @pre All the XML elements except for the last one are instructions.
  ///
  /// @post All forks in the event tree get registered.
  ///
  /// @throws ValidationError  Errors in instruction or target definitions.
  void DefineBranch(const xmlpp::NodeSet& xml_nodes,
                    EventTree* event_tree, Branch* branch);

  /// Processes Instruction definitions.
  ///
  /// @param[in] xml_element  The XML element with instruction definitions.
  ///
  /// @returns The newly defined or registered instruction.
  ///
  /// @throws ValidationError  Errors in instruction definitions.
  Instruction* GetInstruction(const xmlpp::Element* xml_element);

  /// Processes Expression definitions in input file.
  ///
  /// @param[in] expr_element  XML expression element containing the definition.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  ///
  /// @returns The newly defined or registered expression.
  ///
  /// @throws ValidationError  There are problems with getting the expression.
  Expression* GetExpression(const xmlpp::Element* expr_element,
                            const std::string& base_path);

  /// Processes Parameter Expression definitions in input file.
  ///
  /// @param[in] expr_type  The expression type name.
  /// @param[in] expr_element  XML expression element containing the definition.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  ///
  /// @returns Parameter expression described in XML input expression node.
  /// @returns nullptr if the expression type is not a parameter.
  ///
  /// @throws ValidationError  The parameter variable is not reachable.
  Expression* GetParameter(const std::string& expr_type,
                           const xmlpp::Element* expr_element,
                           const std::string& base_path);

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
  ///
  /// @throws CycleError  Model contains cycles.
  /// @throws ValidationError  The initialization contains mistakes.
  ///
  /// @note Cyclic structures need to be broken up by other methods
  ///       if this error condition may lead resource leaks.
  void ValidateInitialization();

  /// Checks the proper order of functional events in event tree forks.
  ///
  /// @param[in] branch  The event tree branch to start the check.
  ///
  /// @throws ValidationError  The order of forks is invalid.
  ///
  /// @pre All named branches are fed separately from initial states.
  void CheckFunctionalEventOrder(const Branch& branch);

  /// Checks that link instructions are used only in event-tree sequences.
  ///
  /// @param[in] branch  The event tree branch to start the check.
  ///
  /// @throws ValidationError  The Link instruction is misused.
  ///
  /// @pre All named branches are fed separately from initial states.
  void EnsureLinksOnlyInSequences(const Branch& branch);

  /// Ensures that event-tree does not mix
  /// collect-expression and collect-formula.
  ///
  /// @param[in] branch  The event tree branch to start the check.
  ///
  /// @throws ValidationError  The Link instruction is misused.
  ///
  /// @pre All named branches are fed separately from initial states.
  void EnsureHomogeneousEventTree(const Branch& branch);

  /// Validates expressions and anything
  /// that is dependent on them,
  /// such as parameters and basic events.
  ///
  /// @throws CycleError  Cyclic parameters are detected.
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

  /// Saved parsers to keep XML documents alive.
  std::vector<std::unique_ptr<xmlpp::DomParser>> parsers_;

  /// Map roots of documents to files for error reporting.
  std::unordered_map<const xmlpp::Node*, std::string> doc_to_file_;

  /// Collection of elements that are defined late
  /// because of unordered registration and definition of their dependencies.
  ///
  /// Parameters and Expressions rely on parameter registrations.
  /// Basic events rely on parameter registrations.
  /// Gates rely on gate, basic event, and house event registrations.
  /// CCF groups rely on both parameter and basic event registrations.
  /// Event tree branches and instructions have complex interdependencies.
  /// Initiating events may reference their associated event trees.
  ///
  /// Elements are assumed to be unique.
  TbdContainer<Parameter, BasicEvent, Gate, CcfGroup, Sequence, EventTree,
               InitiatingEvent, Rule>
      tbd_;

  /// Container of defined expressions for later validation due to cycles.
  std::vector<std::pair<Expression*, const xmlpp::Element*>> expressions_;
  /// Container for event tree links to check for cycles.
  std::vector<Link*> links_;
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_INITIALIZER_H_
