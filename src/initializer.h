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
/// A facility that processes input files into analysis constructs.

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/noncopyable.hpp>

#include "alignment.h"
#include "ccf_group.h"
#include "element.h"
#include "event.h"
#include "event_tree.h"
#include "expression.h"
#include "expression/constant.h"
#include "fault_tree.h"
#include "instruction.h"
#include "model.h"
#include "parameter.h"
#include "settings.h"
#include "substitution.h"
#include "xml.h"

namespace scram::mef {

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
  /// @param[in] allow_extern  Allow external libraries in the input.
  /// @param[in] extra_validator  Additional XML validator to be run
  ///                             after the MEF validator.
  ///
  /// @throws IOError  Input contains duplicate files.
  /// @throws IOError  One of the input files is not accessible.
  /// @throws xml::Error  The xml files contain errors or malformed.
  /// @throws xml::ValidityError  The xml files are not valid for schema.
  /// @throws mef::ValidityError  The input model contains errors.
  ///
  /// @warning Processing external libraries from XML input is **UNSAFE**.
  ///          It allows loading and executing arbitrary code during analysis.
  ///          Enable this feature for trusted input files and libraries only.
  Initializer(const std::vector<std::string>& xml_files,
              core::Settings settings, bool allow_extern = false,
              xml::Validator* extra_validator = nullptr);

  /// @returns The model built from the input files.
  std::unique_ptr<Model> model() && { return std::move(model_); }

 private:
  /// Convenience alias for expression extractor function types.
  using ExtractorFunction = std::unique_ptr<Expression> (*)(
      const xml::Element::Range&, const std::string&, Initializer*);
  /// Map of expression names and their extractor functions.
  using ExtractorMap = std::unordered_map<std::string_view, ExtractorFunction>;
  /// Container for late defined constructs.
  template <class... Ts>
  using TbdContainer =
      std::vector<std::pair<std::variant<Ts*...>, xml::Element>>;
  /// Container with full paths to elements.
  ///
  /// @tparam T  The element type.
  template <typename T>
  using PathTable = boost::multi_index_container<
      T*, boost::multi_index::indexed_by<boost::multi_index::hashed_unique<
              boost::multi_index::const_mem_fun<Id, std::string_view,
                                                &Id::full_path>,
              std::hash<std::string_view>>>>;

  /// @tparam T  Type of an expression.
  /// @tparam N  The number of arguments for the expression.
  ///
  /// Extracts argument expressions from XML elements
  /// and constructs the requested expression T.
  template <class T, int N>
  struct Extractor;

  /// Expressions mapped to their extraction functions.
  static const ExtractorMap kExpressionExtractors_;

  /// Calls Extractor with an appropriate N to construct the expression.
  ///
  /// @tparam T  Type of an expression.
  ///
  /// @param[in] args  XML elements containing the arguments.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in,out] init  The host Initializer.
  ///
  /// @returns The new extracted expression.
  template <class T>
  static std::unique_ptr<Expression> Extract(const xml::Element::Range& args,
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
  /// @throws DuplicateElementError  There are duplicate input files.
  void CheckDuplicateFiles(const std::vector<std::string>& xml_files);

  /// @copybrief Initializer::Initializer
  ///
  /// @param[in] xml_files  The formatted XML input files.
  ///
  /// @throws xml::Error  The xml files are erroneous or malformed.
  /// @throws xml::ValidityError The xml files do not pass validation.
  /// @throws mef::ValidityError  The input model contains errors.
  /// @throws IOError  One of the input files is not accessible.
  /// @throws IOError  Input contains duplicate files.
  void ProcessInputFiles(const std::vector<std::string>& xml_files);

  /// Reads one input XML file document with the structure of analysis entities.
  /// Initializes the analysis from the given document.
  /// Puts all events into their appropriate containers.
  /// This function mostly registers element definitions,
  /// but it may leave them to be defined later
  /// because of possible undefined dependencies of those elements.
  ///
  /// @param[in] document  The XML DOM document from a model input file.
  ///
  /// @pre The document has not been passed before.
  ///
  /// @throws ValidityError  The input model contains errors.
  /// @throws IllegalOperation  Loading external libraries is disallowed.
  void ProcessInputFile(const xml::Document& document);

  /// Processes definitions of elements
  /// that are left to be determined later.
  /// This late definition happens primarily due to unregistered dependencies.
  ///
  /// @throws ValidityError  The elements contain undefined dependencies.
  void ProcessTbdElements();

  /// Registers an element into the model.
  ///
  /// @tparam T  The element type.
  ///
  /// @param[in] element  The initialized element ready to be added into models.
  /// @param[in] xml_element  The related XML element for error messages.
  ///
  /// @throws ValidityError  Issues with adding the element into the model.
  template <class T>
  void Register(std::unique_ptr<T> element, const xml::Element& xml_element);

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
  /// @throws ValidityError  Issues with the new element or registration.
  template <class T>
  T* Register(const xml::Element& xml_node, const std::string& base_path,
              RoleSpecifier base_role);

  /// Adds additional data to element definition
  /// after processing all the input files.
  ///
  /// @tparam T  The Element type to be specialized for.
  ///
  /// @param[in] xml_node  XML element defining the element.
  /// @param[in,out] element  Registered element ready to be defined.
  ///
  /// @throws ValidityError  Issues with the additional data.
  template <class T>
  void Define(const xml::Element& xml_node, T* element);

  /// Defines an event tree for the analysis.
  ///
  /// @param[in] et_node  XML element defining the event tree.
  ///
  /// @throws ValidityError  There are issues with registering and defining
  ///                        the event tree and its data.
  void DefineEventTree(const xml::Element& et_node);

  /// Defines a fault tree for the analysis.
  ///
  /// @param[in] ft_node  XML element defining the fault tree.
  ///
  /// @throws ValidityError  There are issues with registering and defining
  ///                        the fault tree and its data
  ///                        like gates and events.
  void DefineFaultTree(const xml::Element& ft_node);

  /// Defines a component container.
  ///
  /// @param[in] component_node  XML element defining the component.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  /// @param[in] container_role  The parent container's role.
  ///
  /// @returns Component that is ready for registration.
  ///
  /// @throws ValidityError  There are issues with registering and defining
  ///                        the component and its data
  ///                        like gates and events.
  std::unique_ptr<Component> DefineComponent(const xml::Element& component_node,
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
  /// @throws ValidityError  There are issues with registering and defining
  ///                        the component's data like gates and events.
  void RegisterFaultTreeData(const xml::Element& ft_node,
                             const std::string& base_path,
                             Component* component);

  /// Processes model data with definitions of events and analysis.
  ///
  /// @param[in] model_data  XML node with model data description.
  void ProcessModelData(const xml::Element& model_data);

  /// Creates a Boolean formula from the XML elements
  /// describing the formula with events and other nested formulas.
  ///
  /// @param[in] formula_node  XML element defining the formula.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  ///
  /// @returns Boolean formula that is defined.
  ///
  /// @throws ValidityError  The defined formula is not valid.
  std::unique_ptr<Formula> GetFormula(const xml::Element& formula_node,
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
  /// @throws ValidityError  Errors in instruction or target definitions.
  template <class SinglePassRange>
  void DefineBranch(const SinglePassRange& xml_nodes, EventTree* event_tree,
                    Branch* branch);

  /// Processes the last element of the branch node as target.
  void DefineBranchTarget(const xml::Element& target_node,
                          EventTree* event_tree, Branch* branch);

  /// Processes Instruction definitions.
  ///
  /// @param[in] xml_element  The XML element with instruction definitions.
  ///
  /// @returns The newly defined or registered instruction.
  ///
  /// @pre All files have been processed (element pointers are available).
  ///
  /// @throws ValidityError  Errors in instruction definitions.
  Instruction* GetInstruction(const xml::Element& xml_element);

  /// Processes Expression definitions in input file.
  ///
  /// @param[in] expr_element  XML expression element containing the definition.
  /// @param[in] base_path  Series of ancestor containers in the path with dots.
  ///
  /// @returns The newly defined or registered expression.
  ///
  /// @throws ValidityError  There are problems with getting the expression.
  Expression* GetExpression(const xml::Element& expr_element,
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
  /// @throws ValidityError  The parameter variable is not reachable.
  Expression* GetParameter(const std::string_view& expr_type,
                           const xml::Element& expr_element,
                           const std::string& base_path);

  /// Processes common cause failure group members as defined basic events.
  ///
  /// @param[in] members_node  XML element containing all members.
  /// @param[in,out] ccf_group  CCF group of the given members.
  ///
  /// @throws ValidityError  Members are redefined,
  ///                        or there are other setup issues
  ///                        with the CCF group.
  void ProcessCcfMembers(const xml::Element& members_node, CcfGroup* ccf_group);

  /// Defines factor and adds it to CCF group.
  ///
  /// @param[in] factor_node  XML element containing one factor.
  /// @param[in,out] ccf_group  CCF group to be defined by the given factors.
  ///
  /// @throws ValidityError  There are problems with level numbers
  ///                        or factors for specific CCF models.
  void DefineCcfFactor(const xml::Element& factor_node, CcfGroup* ccf_group);

  /// Finds an entity (parameter, basic and house event, gate) from a reference.
  /// The reference is case sensitive
  /// and can contain an identifier, full path, or local path.
  ///
  /// @param[in] entity_reference  Reference string to the entity.
  /// @param[in] base_path  The series of containers indicating the scope.
  ///
  /// @returns Pointer to the entity found by following the given reference.
  ///
  /// @throws UndefinedElement  The entity cannot be found.
  /// @{
  Parameter* GetParameter(std::string_view entity_reference,
                          const std::string& base_path);
  HouseEvent* GetHouseEvent(std::string_view entity_reference,
                            const std::string& base_path);
  BasicEvent* GetBasicEvent(std::string_view entity_reference,
                            const std::string& base_path);
  Gate* GetGate(std::string_view entity_reference,
                const std::string& base_path);
  Formula::ArgEvent GetEvent(std::string_view entity_reference,
                             const std::string& base_path);
  /// @}

  /// Generic helper function to find an entity from a reference.
  /// The reference is case sensitive
  /// and can contain an identifier, full path, or local path.
  ///
  /// @tparam P  The pointer type managing the entity.
  /// @tparam T  The entity type.
  ///
  /// @param[in] entity_reference  Reference string to the entity.
  /// @param[in] base_path  The series of containers indicating the scope.
  /// @param[in] container  Model's lookup container for entities with IDs.
  /// @param[in] path_container  The full path container for entities.
  ///
  /// @returns Pointer to the requested entity.
  ///
  /// @throws UndefinedElement  The entity cannot be found.
  template <class P, class T = typename P::element_type>
  T* GetEntity(std::string_view entity_reference, const std::string& base_path,
               const TableRange<IdTable<P>>& container,
               const TableRange<PathTable<T>>& path_container);

  /// Defines and loads extern libraries.
  ///
  /// @param[in] xml_node  The XML element with the data.
  ///
  /// @throws ValidityError  The initialization contains validity errors.
  void DefineExternLibraries(const xml::Element& xml_node);

  /// Defines extern function.
  ///
  /// @param[in] xml_element  The XML element with the data.
  ///
  /// @throws ValidityError  The initialization contains validity errors.
  ///
  /// @pre All libraries are defined.
  void DefineExternFunction(const xml::Element& xml_element);

  /// Validates if the initialization of the analysis is successful.
  ///
  /// @throws CycleError  Model contains cycles.
  /// @throws ValidityError  The initialization contains mistakes.
  ///
  /// @note Cyclic structures need to be broken up by other methods
  ///       if this error condition may lead resource leaks.
  void ValidateInitialization();

  /// Checks the proper order of functional events in event tree forks.
  ///
  /// @param[in] branch  The event tree branch to start the check.
  ///
  /// @throws ValidityError  The order of forks is invalid.
  ///
  /// @pre All named branches are fed separately from initial states.
  void CheckFunctionalEventOrder(const Branch& branch);

  /// Checks that link instructions are used only in event-tree sequences.
  ///
  /// @param[in] branch  The event tree branch to start the check.
  ///
  /// @throws ValidityError  The Link instruction is misused.
  ///
  /// @pre All named branches are fed separately from initial states.
  void EnsureLinksOnlyInSequences(const Branch& branch);

  /// Ensures that event-tree does not mix
  /// collect-expression and collect-formula.
  ///
  /// @param[in] branch  The event tree branch to start the check.
  ///
  /// @throws ValidityError  The Link instruction is misused.
  ///
  /// @pre All named branches are fed separately from initial states.
  void EnsureHomogeneousEventTree(const Branch& branch);

  /// Ensures that non-declarative substitutions do not have mutual conflicts.
  ///
  /// @throws ValidityError  Conflicts in model substitutions.
  ///
  /// @pre All substitutions are fully defined and valid.
  void EnsureNoSubstitutionConflicts();

  /// Validates expressions and anything
  /// that is dependent on them,
  /// such as parameters and basic events.
  ///
  /// @throws CycleError  Cyclic parameters are detected.
  /// @throws ValidityError  There are problems detected with expressions.
  void ValidateExpressions();

  /// Applies the input information to set up for future analysis.
  /// This step is crucial to get
  /// correct fault tree structures
  /// and basic events with correct expressions.
  /// Meta-logical layer of analysis,
  /// such as CCF groups and substitutions,
  /// is applied to analysis.
  void SetupForAnalysis();

  /// Ensures that non-declarative substitutions do not contain CCF events.
  ///
  /// @throws ValidityError  Hypothesis, source, or target event is in CCF.
  ///
  /// @pre All substitutions are fully defined and valid.
  /// @pre All CCF groups are applied.
  void EnsureNoCcfSubstitutions();

  /// Ensures that non-declarative substitutions
  /// are applied only to algorithms with approximations.
  ///
  /// @throws ValidityError  Exact analysis is requested.
  ///
  /// @todo Research non-declarative substitutions with exact algorithms.
  void EnsureSubstitutionsWithApproximations();

  std::unique_ptr<Model> model_;  ///< Analysis model with constructs.
  core::Settings settings_;  ///< Settings for analysis.
  bool allow_extern_;  ///< Allow processing MEF 'extern-library'.
  xml::Validator* extra_validator_;  ///< The optional extra XML validation.

  /// Saved XML documents to keep elements alive.
  std::vector<xml::Document> documents_;

  /// Collection of elements that are defined late
  /// because of unordered registration and definition of their dependencies.
  ///
  /// Parameters and Expressions rely on parameter registrations.
  /// Basic events rely on parameter registrations.
  /// Gates rely on gate, basic event, and house event registrations.
  /// CCF groups rely on both parameter and basic event registrations.
  /// Event tree branches and instructions have complex interdependencies.
  /// Initiating events may reference their associated event trees.
  /// Alignments depend on instructions.
  /// Substitutions depend on basic events.
  ///
  /// Elements are assumed to be unique.
  TbdContainer<Parameter, BasicEvent, Gate, CcfGroup, Sequence, EventTree,
               InitiatingEvent, Rule, Alignment, Substitution>
      tbd_;

  /// Container of defined expressions for later validation due to cycles.
  std::vector<std::pair<Expression*, xml::Element>> expressions_;
  /// Container for event tree links to check for cycles.
  std::vector<Link*> links_;

  /// Containers for reference resolution with paths.
  /// @{
  PathTable<Gate> path_gates_;
  PathTable<BasicEvent> path_basic_events_;
  PathTable<HouseEvent> path_house_events_;
  PathTable<Parameter> path_parameters_;
  /// @}
};

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

/// Filters the data for MEF Element definitions.
///
/// @param[in] xml_element  The XML element with the construct definition.
///
/// @returns A range of XML child elements of MEF Element constructs.
inline
auto GetNonAttributeElements(const xml::Element& xml_element) {
  return xml_element.children() |
         boost::adaptors::filtered([](const xml::Element& child) {
           std::string_view name = child.name();
           return name != "label" && name != "attributes";
         });
}

}  // namespace scram::mef
