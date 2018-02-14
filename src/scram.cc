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
/// Main entrance.

#include <cstdarg>
#include <cstdio>  // vsnprintf
#include <cstring>  // strerror

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/core/typeinfo.hpp>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>

#include <libxml/parser.h>  // xmlInitParser, xmlCleanupParser
#include <libxml/xmlerror.h>  // initGenericErrorDefaultFunc
#include <libxml/xmlversion.h>  // LIBXML_TEST_VERSION

#include "config.h"
#include "error.h"
#include "ext/scope_guard.h"
#include "initializer.h"
#include "logger.h"
#include "reporter.h"
#include "risk_analysis.h"
#include "serialization.h"
#include "settings.h"
#include "version.h"

namespace po = boost::program_options;

namespace {

/// Provides an options value type.
#define OPT_VALUE(type) po::value<type>()->value_name(#type)

/// @returns Command-line option descriptions.
po::options_description ConstructOptions() {
  using path = std::string;  // To print argument type as path.

  po::options_description desc("Options");
  // clang-format off
  desc.add_options()
      ("help", "Display this help message")
      ("version", "Display version information")
      ("config-file", OPT_VALUE(path), "XML file with analysis configurations")
      ("allow-extern", "**UNSAFE** Allow external libraries")
      ("validate", "Validate input files without analysis")
      ("bdd", "Perform qualitative analysis with BDD")
      ("zbdd", "Perform qualitative analysis with ZBDD")
      ("mocus", "Perform qualitative analysis with MOCUS")
      ("prime-implicants", "Calculate prime implicants")
      ("probability", OPT_VALUE(bool), "Perform probability analysis")
      ("importance", OPT_VALUE(bool), "Perform importance analysis")
      ("uncertainty", OPT_VALUE(bool), "Perform uncertainty analysis")
      ("ccf", OPT_VALUE(bool), "Perform common-cause failure analysis")
      ("sil", OPT_VALUE(bool), "Compute the Safety Integrity Level metrics")
      ("rare-event", "Use the rare event approximation")
      ("mcub", "Use the MCUB approximation")
      ("limit-order,l", OPT_VALUE(int), "Upper limit for the product order")
      ("cut-off", OPT_VALUE(double), "Cut-off probability for products")
      ("mission-time", OPT_VALUE(double), "System mission time in hours")
      ("time-step", OPT_VALUE(double),
       "Time step in hours for probability analysis")
      ("num-trials", OPT_VALUE(int),
       "Number of trials for Monte Carlo simulations")
      ("num-quantiles", OPT_VALUE(int),
       "Number of quantiles for distributions")
      ("num-bins", OPT_VALUE(int), "Number of bins for histograms")
      ("seed", OPT_VALUE(int), "Seed for the pseudo-random number generator")
      ("output-path,o", OPT_VALUE(path), "Output path for reports")
      ("no-indent", "Omit indentation whitespace in output XML")
      ("verbosity", OPT_VALUE(int), "Set log verbosity");
#ifndef NDEBUG
  po::options_description debug("Debug Options");
  debug.add_options()
      ("serialize", "Serialize the input model without further analysis")
      ("preprocessor", "Stop analysis after the preprocessing step")
      ("print", "Print analysis results in a terminal friendly way")
      ("no-report", "Don't generate analysis report");
  desc.add(debug);
#endif
  // clang-format on
  return desc;
}
#undef OPT_VALUE

/// Parses the command-line arguments.
///
/// @param[in] argc  Count of arguments.
/// @param[in] argv  Values of arguments.
/// @param[out] vm  Variables map of program options.
///
/// @returns 0 for success.
/// @returns 1 for errored state.
/// @returns -1 for information only state like help and version.
int ParseArguments(int argc, char* argv[], po::variables_map* vm) {
  const char* usage = "Usage:    scram [options] input-files...";
  po::options_description desc = ConstructOptions();
  try {
    po::store(po::parse_command_line(argc, argv, desc), *vm);
  } catch (std::exception& err) {
    std::cerr << "Option error: " << err.what() << "\n\n"
              << usage << "\n\n"
              << desc << std::endl;
    return 1;
  }
  po::options_description options("All options with positional input files.");
  options.add(desc).add_options()("input-files",
                                  po::value<std::vector<std::string>>(),
                                  "XML input files with analysis constructs");
  po::positional_options_description p;
  p.add("input-files", -1);  // All input files are implicit.
  po::store(
      po::command_line_parser(argc, argv).options(options).positional(p).run(),
      *vm);
  po::notify(*vm);

  auto print_help = [&usage, &desc](std::ostream& out) {
    out << usage << "\n\n" << desc << std::endl;
  };

  // Process command-line arguments.
  if (vm->count("help")) {
    print_help(std::cout);
    return -1;
  }
  if (vm->count("version")) {
    std::cout << "SCRAM " << scram::version::core() << " ("
              << scram::version::describe() << ")"
              << "\n\nDependencies:\n"
              << "   Boost       " << scram::version::boost() << "\n"
              << "   libxml2     " << scram::version::libxml() << std::endl;
    return -1;
  }

  if (vm->count("verbosity")) {
    int level = (*vm)["verbosity"].as<int>();
    if (level < 0 || level > scram::kMaxVerbosity) {
      std::cerr << "Log verbosity must be between 0 and "
                << scram::kMaxVerbosity << ".\n\n";
      print_help(std::cerr);
      return 1;
    }
  }

  if (!vm->count("input-files") && !vm->count("config-file")) {
    std::cerr << "No input or configuration file is given.\n\n";
    print_help(std::cerr);
    return 1;
  }
  if ((vm->count("bdd") + vm->count("zbdd") + vm->count("mocus")) > 1) {
    std::cerr << "Mutually exclusive qualitative analysis algorithms.\n"
              << "(MOCUS/BDD/ZBDD) cannot be applied at the same time.\n\n";
    print_help(std::cerr);
    return 1;
  }
  if (vm->count("rare-event") && vm->count("mcub")) {
    std::cerr << "The rare event and MCUB approximations cannot be "
              << "applied at the same time.\n\n";
    print_help(std::cerr);
    return 1;
  }
  return 0;
}

// clang-format off
/// Helper macro for ConstructSettings
/// to set the flag in "settings"
/// only if provided by "vm" arguments.
#define SET(tag, type, member) \
  if (vm.count(tag)) settings->member(vm[tag].as<type>())
// clang-format on

/// Updates analysis settings from command-line arguments.
///
/// @param[in] vm  Variables map of program options.
/// @param[in,out] settings  Pre-configured or default settings.
///
/// @throws SettingsError  The indication of an error in arguments.
/// @throws std::exception  vm does not contain a required option.
///                         At least defaults are expected.
void ConstructSettings(const po::variables_map& vm,
                       scram::core::Settings* settings) {
  if (vm.count("bdd")) {
    settings->algorithm("bdd");
  } else if (vm.count("zbdd")) {
    settings->algorithm("zbdd");
  } else if (vm.count("mocus")) {
    settings->algorithm("mocus");
  }
  settings->prime_implicants(vm.count("prime-implicants"));
  // Determine if the probability approximation is requested.
  if (vm.count("rare-event")) {
    assert(!vm.count("mcub"));
    settings->approximation("rare-event");
  } else if (vm.count("mcub")) {
    settings->approximation("mcub");
  }
  SET("time-step", double, time_step);
  SET("sil", bool, safety_integrity_levels);

  SET("probability", bool, probability_analysis);
  SET("importance", bool, importance_analysis);
  SET("uncertainty", bool, uncertainty_analysis);
  SET("ccf", bool, ccf_analysis);
  SET("seed", int, seed);
  SET("limit-order", int, limit_order);
  SET("cut-off", double, cut_off);
  SET("mission-time", double, mission_time);
  SET("num-trials", int, num_trials);
  SET("num-quantiles", int, num_quantiles);
  SET("num-bins", int, num_bins);
#ifndef NDEBUG
  settings->preprocessor = vm.count("preprocessor");
  settings->print = vm.count("print");
#endif
}
#undef SET

/// Main body of command-line entrance to run the program.
///
/// @param[in] vm  Variables map of program options.
///
/// @throws Error  Exceptions specific to SCRAM.
/// @throws boost::exception  Boost errors with the variables map.
/// @throws std::exception  All other problems.
void RunScram(const po::variables_map& vm) {
  scram::core::Settings settings;  // Analysis settings.
  std::vector<std::string> input_files;
  std::string output_path;
  // Get configurations if any.
  // Invalid configurations will throw.
  if (vm.count("config-file")) {
    auto config =
        std::make_unique<scram::Config>(vm["config-file"].as<std::string>());
    settings = config->settings();
    input_files = config->input_files();
    output_path = config->output_path();
  }
  // Command-line settings overwrite
  // the settings from the configurations.
  ConstructSettings(vm, &settings);
  if (vm.count("input-files")) {
    auto cmd_input = vm["input-files"].as<std::vector<std::string>>();
    input_files.insert(input_files.end(), cmd_input.begin(), cmd_input.end());
  }
  if (vm.count("output-path")) {
    output_path = vm["output-path"].as<std::string>();
  }
  // Process input files
  // into valid analysis containers and constructs.
  // Throws if anything is invalid.
  std::unique_ptr<scram::mef::Model> model =
      scram::mef::Initializer(input_files, settings, vm.count("allow-extern"))
          .model();
#ifndef NDEBUG
  if (vm.count("serialize"))
    return Serialize(*model, stdout);
#endif
  if (vm.count("validate"))
    return;  // Stop if only validation is requested.

  // Initiate risk analysis with the given information.
  scram::core::RiskAnalysis analysis(model.get(), settings);
  analysis.Analyze();
#ifndef NDEBUG
  if (vm.count("no-report") || vm.count("preprocessor") || vm.count("print"))
    return;
#endif
  scram::Reporter reporter;
  bool indent = vm.count("no-indent") ? false : true;
  if (output_path.empty()) {
    reporter.Report(analysis, stdout, indent);
  } else {
    reporter.Report(analysis, output_path, indent);
  }
}

/// Callback function to redirect XML library error/warning messages to logging.
/// Otherwise, the messages are printed to the standard error.
///
/// @param[in] msg  The printf-style format string.
/// @param[in] ...  The variadic arguments for the format string.
///
/// @pre The library strictly follows validity conditions of printf.
void LogXmlError(void* /*ctx*/, const char* msg, ...) noexcept {
  std::va_list args;
  va_start(args, msg);
  SCOPE_EXIT([&args] { va_end(args); });

  std::va_list args_for_nchar;  // Only used to determine the string length.
  va_copy(args_for_nchar, args);
  int nchar = std::vsnprintf(nullptr, 0, msg, args_for_nchar);
  va_end(args_for_nchar);
  if (nchar < 0) {
    LOG(scram::ERROR) << "String formatting failure: " << std::strerror(errno);
    return;
  }

  std::vector<char> buffer(nchar + /*null terminator*/ 1);
  std::vsnprintf(buffer.data(), buffer.size(), msg, args);
  LOG(scram::WARNING) << buffer.data();
}

/// Prints error information into the standard error.
///
/// @tparam Tag  The error info tag to retrieve the error value.
///
/// @param[in] tag_string  The string for the tag type.
/// @param[in] err  The error.
template <class Tag>
void PrintErrorInfo(const char* tag_string, const scram::Error& err) {
  if (const auto* value = boost::get_error_info<Tag>(err))
    std::cerr << tag_string << ": " << *value << "\n";
}

}  // namespace

/// Command-line SCRAM entrance.
///
/// @param[in] argc  Argument count.
/// @param[in] argv  Argument vector.
///
/// @returns 0 for success.
/// @returns 1 for errored state.
int main(int argc, char* argv[]) {
  LIBXML_TEST_VERSION
  xmlInitParser();
  SCOPE_EXIT(&xmlCleanupParser);

  xmlGenericErrorFunc xml_error_printer = LogXmlError;
  initGenericErrorDefaultFunc(&xml_error_printer);

  try {
    // Parse command-line options.
    po::variables_map vm;
    int ret = ParseArguments(argc, argv, &vm);
    if (ret == 1)
      return 1;

    if (vm.count("verbosity")) {
      scram::Logger::report_level(
          static_cast<scram::LogLevel>(vm["verbosity"].as<int>()));
    }

    if (ret == 0)
      RunScram(vm);
  } catch (const scram::LogicError& err) {
    LOG(scram::ERROR) << "Logic Error:\n" << boost::diagnostic_information(err);
    return 1;
  } catch (const scram::IOError& err) {
    LOG(scram::DEBUG1) << boost::diagnostic_information(err);
    std::cerr << boost::core::demangled_name(typeid(err)) << "\n\n";
    PrintErrorInfo<boost::errinfo_file_name>("File", err);
    PrintErrorInfo<boost::errinfo_file_open_mode>("Open mode", err);
    if (const int* errnum = boost::get_error_info<boost::errinfo_errno>(err)) {
      std::cerr << "Error code: " << *errnum << "\n";
      std::cerr << "Error string: " << std::strerror(*errnum) << "\n";
    }
    std::cerr << "\n" << err.what() << std::endl;
    return 1;
  } catch (const scram::Error& err) {
    using namespace scram;  // NOLINT
    LOG(DEBUG1) << boost::diagnostic_information(err);
    std::cerr << boost::core::demangled_name(typeid(err)) << "\n\n";
    PrintErrorInfo<errinfo_value>("Value", err);
    PrintErrorInfo<boost::errinfo_file_name>("File", err);
    PrintErrorInfo<boost::errinfo_at_line>("Line", err);
    PrintErrorInfo<mef::errinfo_connective>("MEF Connective", err);
    PrintErrorInfo<mef::errinfo_reference>("MEF reference", err);
    PrintErrorInfo<mef::errinfo_base_path>("MEF base path", err);
    PrintErrorInfo<mef::errinfo_element_id>("MEF Element ID", err);
    PrintErrorInfo<mef::errinfo_element_type>("MEF Element type", err);
    PrintErrorInfo<mef::errinfo_container_id>("MEF Container", err);
    PrintErrorInfo<mef::errinfo_container_type>("MEF Container type", err);
    PrintErrorInfo<mef::errinfo_attribute>("MEF Attribute", err);
    PrintErrorInfo<mef::errinfo_cycle>("Cycle", err);
    PrintErrorInfo<xml::errinfo_element>("XML element", err);
    PrintErrorInfo<xml::errinfo_attribute>("XML attribute", err);
    std::cerr << "\n" << err.what() << std::endl;
    return 1;
  } catch (const boost::exception& boost_err) {
    LOG(scram::ERROR) << "Unexpected Boost Exception:\n"
                      << boost::diagnostic_information(boost_err);
    return 1;
  } catch (const std::exception& err) {
    LOG(scram::ERROR) << "Unexpected Exception: "
                      << boost::core::demangled_name(typeid(err)) << ":\n"
                      << err.what();
    return 1;
  }
}  // End of main.
