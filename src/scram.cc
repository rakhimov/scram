/// @file scram.cc
/// Main entrance.
#include <iostream>
#include <string>
#include <vector>

#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "config.h"
#include "error.h"
#include "logger.h"
#include "risk_analysis.h"
#include "settings.h"
#include "version.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace scram;

/// Parses the command-line arguments.
/// @param[in] argc Count of arguments.
/// @param[in] argv Values of arguments.
/// @param[out] vm Variables map of program options.
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
        ("validate-only,v", "Validate input files without analysis")
        ("graph-only,g", "Validate and produce graph without analysis")
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
        ("log", "Turn on the logging system")
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

  // Process command line args.
  if (vm->count("help")) {
    std::cout << usage << "\n\n" << desc << "\n";
    return -1;
  }

  if (vm->count("version")) {
    std::cout << "SCRAM " << version::core()
        << " (" << version::describe() << ")"
        << "\n\nDependencies:\n";
    std::cout << "   Boost    " << version::boost() << "\n";
    std::cout << "   xml2     " << version::xml2() << "\n";
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

  return 0;
}

/// Updates analysis settings from command-line arguments.
/// @param[in] vm Variables map of program options.
/// @param[in,out] settings Pre-configured or default settings.
/// @throws std::exception if vm does not contain a required option.
///                        At least defaults are expected.
void ConstructSettings(const po::variables_map& vm, Settings* settings) {
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
  if (vm.count("ccf")) settings->ccf_analysis(vm["ccf"].as<bool>());;
}

/// Main body of commond-line entrance to run the program.
/// @param[in] vm Variables map of program options.
/// @throws Error for any type of internal problems like validation.
/// @throws boost::exception for possible Boost usage errors.
/// @throws std::exception for any other problems.
/// @returns 0 for success.
/// @returns 1 for errored state.
int RunScram(const po::variables_map& vm) {
  if (vm.count("log")) Logger::active() = true;
  // Initiate risk analysis.
  RiskAnalysis* ran = new RiskAnalysis();
  // Analysis settings.
  Settings settings;
  std::vector<std::string> input_files;
  std::string output_path = "";
  // Get configurations if any.
  if (vm.count("config-file")) {
    Config* config = new Config(vm["config-file"].as<std::string>());
    settings = config->settings();
    input_files = config->input_files();
    output_path = config->output_path();
    delete config;
  }

  // Command-line settings overwrites the settings from the configurations.
  ConstructSettings(vm, &settings);
  ran->AddSettings(settings);

  // Add input files from the comand line.
  if (vm.count("input-files")) {
    std::vector<std::string> cmd_input =
        vm["input-files"].as< std::vector<std::string> >();
    input_files.insert(input_files.end(), cmd_input.begin(), cmd_input.end());
  }

  // Overwrite output path if it is given from the command-line.
  if (vm.count("output-path")) {
    output_path = vm["output-path"].as<std::string>();
  }

  // Process input files and validate it.
  ran->ProcessInputFiles(input_files);

  // Stop if only validation is requested.
  if (vm.count("validate-only")) {
    std::cout << "The files are VALID." << std::endl;
    return 0;
  }

  // Graph if requested.
  if (vm.count("graph-only")) {
    if (output_path != "") {
      ran->GraphingInstructions(output_path);
    } else {
      ran->GraphingInstructions();
    }
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

/// Command line SCRAM entrance.
/// @returns 0 for success.
/// @returns 1 for errored state.
int main(int argc, char* argv[]) {
  // Parse command line options.
  po::variables_map vm;
  int ret = ParseArguments(argc, argv, &vm);
  if (ret == 1) return 1;
  if (ret == -1) return 0;

#ifdef NDEBUG
  try {  // Catch exceptions only for non-debug builds.
#endif

    return RunScram(vm);

#ifdef NDEBUG
  } catch (IOError& io_err) {
    std::cerr << "SCRAM I/O Error\n" << std::endl;
    std::cerr << io_err.what() << std::endl;
    return 1;
  } catch (ValidationError& vld_err) {
    std::cerr << "SCRAM Validation Error\n" << std::endl;
    std::cerr << vld_err.what() << std::endl;
    return 1;
  } catch (ValueError& val_err) {
    std::cerr << "SCRAM Value Error\n" << std::endl;
    std::cerr << val_err.what() << std::endl;
    return 1;
  } catch (LogicError& logic_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "SCRAM Logic Error\n" << std::endl;
    std::cerr << logic_err.what() << std::endl;
    return 1;
  } catch (IllegalOperation& iopp_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "SCRAM Illegal Operation\n" << std::endl;
    std::cerr << iopp_err.what() << std::endl;
    return 1;
  } catch (InvalidArgument& iarg_err) {
    std::cerr << "Bad, bad news. Please report this error. Thank you!\n"
        << std::endl;
    std::cerr << "SCRAM Invalid Argument Error\n" << std::endl;
    std::cerr << iarg_err.what() << std::endl;
    return 1;
  } catch (boost::exception& boost_err) {
    std::cerr << "Boost Exception:\n" << std::endl;
    std::cerr << boost::diagnostic_information(boost_err) << std::endl;
    return 1;
  } catch (std::exception& std_err) {
    std::cerr << "Standard Exception:\n" << std::endl;
    std::cerr << std_err.what() << std::endl;
    return 1;
  }
#endif

  return 0;
}  // End of main.
