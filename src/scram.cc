/// @file scram.cc
/// Main entrance.
#include <iostream>
#include <string>
#include <vector>

#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

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
  std::string usage = "Usage:    scram [input-files] [opts]";
  po::options_description desc("Allowed options");

  try {
    desc.add_options()
        ("help,h", "display this help message")
        ("version", "display version information")
        ("input-file", po::value< std::vector<std::string> >(),
         "XML input files with analysis entities")
        ("config,C", po::value<std::string>(),
         "XML configuration file for analysis (NOT SUPPERTED)")
        ("validate,v", "only validate input files")
        ("graph-only,g", "produce graph without analysis")
        ("probability", po::value<bool>()->default_value(false),
         "perform probability analysis")
        ("importance", po::value<bool>()->default_value(false),
         "perform importance analysis")
        ("uncertainty", po::value<bool>()->default_value(false),
         "perform uncertainty analysis")
        ("ccf", po::value<bool>()->default_value(false),
         "perform common-cause failure analysis")
        ("rare-event,r", "use the rare event approximation")
        ("mcub,m", "use the MCUB approximation for probability calculations")
        ("limit-order,l", po::value<int>()->default_value(20),
         "upper limit for cut set order")
        ("nsums,s", po::value<int>()->default_value(7),
         "number of sums in series expansion for probability calculations")
        ("cut-off,c", po::value<double>()->default_value(1e-8),
         "cut-off probability for cut sets")
        ("mission-time,t", po::value<double>()->default_value(8760),
         "system mission time in hours")
        ("trials,S", po::value<int>()->default_value(1e3),
         "number of trials for Monte Carlo simulations")
        ("output,o", po::value<std::string>(), "output file")
        ("log,L", "Turn on logging system")
        ;

    po::store(po::parse_command_line(argc, argv, desc), *vm);
  } catch (std::exception& err) {
    std::cout << "Invalid arguments.\n"
        << usage << "\n\n" << desc << "\n";
    return 1;
  }
  po::notify(*vm);

  po::positional_options_description p;
  p.add("input-file", -1);

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

  if (!vm->count("input-file")) {
    std::string msg = "No input file given.\n";
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

/// Constructs analysis settings from command-line arguments.
/// @param[in] vm Variables map of program options.
/// @throws std::exception if vm does not contain a required option.
///                        At least defaults are expected.
Settings ConstructSettings(const po::variables_map& vm) {
  // Analysis settings.
  Settings settings;

  // Determine if the probability approximation is requested.
  if (vm.count("rare-event")) {
    assert(!vm.count("mcub"));
    settings.approx("rare");
  } else if (vm.count("mcub")) {
    settings.approx("mcub");
  }

  settings.limit_order(vm["limit-order"].as<int>())
      .num_sums(vm["nsums"].as<int>())
      .cut_off(vm["cut-off"].as<double>())
      .mission_time(vm["mission-time"].as<double>())
      .trials(vm["trials"].as<int>())
      .probability_analysis(vm["probability"].as<bool>())
      .importance_analysis(vm["importance"].as<bool>())
      .uncertainty_analysis(vm["uncertainty"].as<bool>())
      .ccf_analysis(vm["ccf"].as<bool>());

  return settings;
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
  ran->AddSettings(ConstructSettings(vm));

  // Process input files and validate it.
  ran->ProcessInputFiles(vm["input-file"].as< std::vector<std::string> >());

  // Stop if only validation is requested.
  if (vm.count("validate")) {
    std::cout << "The files are VALID." << std::endl;
    return 0;
  }

  // Graph if requested.
  if (vm.count("graph-only")) {
    if (vm.count("output")) {
      ran->GraphingInstructions(vm["output"].as<std::string>());
    } else {
      ran->GraphingInstructions();
    }
    return 0;
  }

  ran->Analyze();

  if (vm.count("output")) {
    ran->Report(vm["output"].as<std::string>());
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
