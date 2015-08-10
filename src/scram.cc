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
/// @file scram.cc
/// Main entrance.
#include <iostream>
#include <string>
#include <vector>

#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>

#include "config.h"
#include "error.h"
#include "initializer.h"
#include "logger.h"
#include "risk_analysis.h"
#include "settings.h"
#include "version.h"

namespace po = boost::program_options;

namespace {

/// Parses the command-line arguments.
///
/// @param[in] argc Count of arguments.
/// @param[in] argv Values of arguments.
/// @param[out] vm Variables map of program options.
///
/// @returns 0 for success.
/// @returns 1 for errored state.
/// @returns -1 for information only state like help and version.
int ParseArguments(int argc, char* argv[], po::variables_map* vm) {
  std::string usage = "Usage:    scram [input-files] [options]";
  po::options_description desc("Options");

  try {
    desc.add_options()
        ("help", "Display this help message")
        ("version", "Display version information")
        ("input-files", po::value< std::vector<std::string> >(),
         "XML input files with analysis entities")
        ("config-file", po::value<std::string>(),
         "XML configuration file for analysis")
        ("validate", "Validate input files without analysis")
        ("graph", "Validate and produce graph without analysis")
        ("probability", po::value<bool>(), "Perform probability analysis")
        ("importance", po::value<bool>(), "Perform importance analysis")
        ("uncertainty", po::value<bool>(), "Perform uncertainty analysis")
        ("ccf", po::value<bool>(), "Perform common-cause failure analysis")
        ("rare-event",
         "Use the rare event approximation for probability calculations")
        ("mcub", "Use the MCUB approximation for probability calculations")
        ("limit-order,l", po::value<int>(), "Upper limit for cut set order")
        ("num-sums,s", po::value<int>(),
         "Number of sums in series expansion for probability calculations")
        ("cut-off", po::value<double>(), "Cut-off probability for cut sets")
        ("mission-time", po::value<double>(), "System mission time in hours")
        ("num-trials", po::value<int>(),
         "Number of trials for Monte Carlo simulations")
        ("seed", po::value<int>(),
         "Seed for the pseudo-random number generator")
        ("output-path,o", po::value<std::string>(), "Output path")
        ("verbosity", po::value<int>(), "Set log verbosity")
        ;

    po::store(po::parse_command_line(argc, argv, desc), *vm);
  } catch (std::exception& err) {
    std::cout << "Option error: " << err.what() << "\n\n"
        << usage << "\n\n" << desc << "\n";
    return 1;
  }
  po::notify(*vm);

  po::positional_options_description p;
  p.add("input-files", -1);

  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).
            run(), *vm);
  po::notify(*vm);

  // Process command-line arguments.
  if (vm->count("help")) {
    std::cout << usage << "\n\n" << desc << "\n";
    return -1;
  }

  if (vm->count("version")) {
    std::string build_type =
        strlen(scram::version::build()) ? scram::version::build() : "Non-Debug";

    std::cout << "SCRAM " << scram::version::core()
              << " (" << scram::version::describe() << ")";
    if (build_type != "Release") {
      std::cout << " " << build_type << " Build";
    }
    std::cout << "\n\nDependencies:\n";
    std::cout << "   Boost    " << scram::version::boost() << "\n";
    std::cout << "   xml2     " << scram::version::xml2() << "\n";
    return -1;
  }

  if (!vm->count("input-files") && !vm->count("config-file")) {
    std::string msg = "No input or configuration file is given.\n";
    std::cout << msg << std::endl;
    std::cout << usage << "\n\n" << desc << "\n";
    return 1;
  }

  if (vm->count("rare-event") && vm->count("mcub")) {
    std::string msg = "The rare event and MCUB approximations cannot be "
                      "applied at the time.";
    std::cout << msg << "\n" << std::endl;
    std::cout << usage << "\n\n" << desc << std::endl;
    return 1;
  }

  if (vm->count("verbosity")) {
    int verb = (*vm)["verbosity"].as<int>();
    if (verb < 0 || verb > 7) {
      std::string msg = "Log verbosity must be between 0 and 7.";
      std::cout << msg << "\n" << std::endl;
      std::cout << usage << "\n\n" << desc << std::endl;
      return 1;
    }
  }

  return 0;
}

/// Updates analysis settings from command-line arguments.
///
/// @param[in] vm Variables map of program options.
/// @param[in,out] settings Pre-configured or default settings.
///
/// @throws std::exception vm does not contain a required option.
///                        At least defaults are expected.
void ConstructSettings(const po::variables_map& vm, scram::Settings* settings) {
  // Determine if the probability approximation is requested.
  if (vm.count("rare-event")) {
    assert(!vm.count("mcub"));
    settings->approx("rare-event");
  } else if (vm.count("mcub")) {
    settings->approx("mcub");
  }
  if (vm.count("seed")) settings->seed(vm["seed"].as<int>());
  if (vm.count("limit-order"))
    settings->limit_order(vm["limit-order"].as<int>());
  if (vm.count("num-sums")) settings->num_sums(vm["num-sums"].as<int>());
  if (vm.count("cut-off")) settings->cut_off(vm["cut-off"].as<double>());
  if (vm.count("mission-time"))
    settings->mission_time(vm["mission-time"].as<double>());
  if (vm.count("num-trials")) settings->num_trials(vm["num-trials"].as<int>());
  if (vm.count("importance"))
    settings->importance_analysis(vm["importance"].as<bool>());
  if (vm.count("uncertainty"))
    settings->uncertainty_analysis(vm["uncertainty"].as<bool>());
  if (vm.count("probability"))
    settings->probability_analysis(vm["probability"].as<bool>());
  if (vm.count("ccf")) settings->ccf_analysis(vm["ccf"].as<bool>());
}

/// Main body of command-line entrance to run the program.
///
/// @param[in] vm Variables map of program options.
///
/// @returns 0 for success.
/// @returns 1 for errored state.
///
/// @throws Error Internal problems specific to SCRAM like validation.
/// @throws boost::exception Boost errors.
/// @throws std::exception All other problems.
int RunScram(const po::variables_map& vm) {
  if (vm.count("verbosity")) {
    scram::Logger::ReportLevel() =
        static_cast<scram::LogLevel>(vm["verbosity"].as<int>());
  }
  // Analysis settings.
  scram::Settings settings;
  std::vector<std::string> input_files;
  std::string output_path = "";
  // Get configurations if any.
  if (vm.count("config-file")) {
    scram::Config* config =
        new scram::Config(vm["config-file"].as<std::string>());
    settings = config->settings();
    input_files = config->input_files();
    output_path = config->output_path();
    delete config;
  }

  // Command-line settings overwrites the settings from the configurations.
  ConstructSettings(vm, &settings);

  // Add input files from the command-line.
  if (vm.count("input-files")) {
    std::vector<std::string> cmd_input =
        vm["input-files"].as< std::vector<std::string> >();
    input_files.insert(input_files.end(), cmd_input.begin(), cmd_input.end());
  }
  // Overwrite output path if it is given from the command-line.
  if (vm.count("output-path")) {
    output_path = vm["output-path"].as<std::string>();
  }
  // Process input files into valid analysis containers and constructs.
  scram::Initializer* init = new scram::Initializer(settings);
  init->ProcessInputFiles(input_files);
  // Initiate risk analysis with the given information.
  scram::RiskAnalysis* ran = new scram::RiskAnalysis(init->model(), settings);
  delete init;

  // Stop if only validation is requested.
  if (vm.count("validate")) {
    std::cout << "The files are VALID." << std::endl;
    delete ran;
    return 0;
  }

  // Graph if requested.
  if (vm.count("graph")) {
    ran->GraphingInstructions();
    delete ran;
    return 0;
  }

  ran->Analyze();

  if (output_path != "") {
    ran->Report(output_path);
  } else {
    ran->Report(std::cout);
  }

  delete ran;
  return 0;
}

}  // namespace

/// Command-line SCRAM entrance.
///
/// @param[in] argc Argument count.
/// @param[in] argv Argument vector.
///
/// @returns 0 for success.
/// @returns 1 for errored state.
int main(int argc, char* argv[]) {
#ifdef NDEBUG
  try {  // Catch exceptions only for non-debug builds.
#endif

    // Parse command-line options.
    po::variables_map vm;
    int ret = ParseArguments(argc, argv, &vm);
    if (ret == 1) return 1;
    if (ret == -1) return 0;

    return RunScram(vm);

#ifdef NDEBUG
  } catch (scram::IOError& io_err) {
    std::cerr << "SCRAM I/O Error\n" << std::endl;
    std::cerr << io_err.what() << std::endl;
    return 1;
  } catch (scram::ValidationError& vld_err) {
    std::cerr << "SCRAM Validation Error\n" << std::endl;
    std::cerr << vld_err.what() << std::endl;
    return 1;
  } catch (scram::ValueError& val_err) {
    std::cerr << "SCRAM Value Error\n" << std::endl;
    std::cerr << val_err.what() << std::endl;
    return 1;
  } catch (scram::LogicError& logic_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "SCRAM Logic Error\n" << std::endl;
    std::cerr << logic_err.what() << std::endl;
    return 1;
  } catch (scram::IllegalOperation& iopp_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "SCRAM Illegal Operation\n" << std::endl;
    std::cerr << iopp_err.what() << std::endl;
    return 1;
  } catch (scram::InvalidArgument& iarg_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "SCRAM Invalid Argument Error\n" << std::endl;
    std::cerr << iarg_err.what() << std::endl;
    return 1;
  } catch (scram::Error& scram_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "SCRAM Error\n" << std::endl;
    std::cerr << scram_err.what() << std::endl;
    return 1;
  } catch (boost::exception& boost_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "Boost Exception:\n" << std::endl;
    std::cerr << boost::diagnostic_information(boost_err) << std::endl;
    return 1;
  } catch (std::exception& std_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "Standard Exception:\n" << std::endl;
    std::cerr << std_err.what() << std::endl;
    return 1;
  }
#endif

  return 0;
}  // End of main.
