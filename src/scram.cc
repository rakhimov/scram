/// @file scram.cc
/// Main entrance.
#include <iostream>
#include <string>

#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "error.h"
#include "fault_tree_analysis.h"
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
  // Parse command line options.
  std::string usage = "Usage:    scram [input-file] [opts]";
  po::options_description desc("Allowed options");

  try {
    desc.add_options()
        ("help,h", "display this help message")
        ("version", "display version information")
        ("input-file", po::value<std::string>(),
         "xml input file with analysis entities")
        ("validate,v", "only validate input files")
        ("graph-only,g", "produce graph without analysis")
        ("analysis,a", po::value<std::string>()->default_value("default"),
         "type of analysis to be performed on this input")
        ("rare-event,r", "use the rare event approximation")
        ("mcub,m", "use the MCUB approximation for probability calculations")
        ("limit-order,l", po::value<int>()->default_value(20),
         "upper limit for cut set order")
        ("nsums,s", po::value<int>()->default_value(1000000),
         "number of sums in series expansion for probability calculations")
        ("cut-off,c", po::value<double>()->default_value(1e-8),
         "cut-off probability for cut sets")
        ("output,o", po::value<std::string>(), "output file")
        ;

    po::store(po::parse_command_line(argc, argv, desc), *vm);
  } catch (std::exception& err) {
    std::cout << "Invalid arguments.\n"
        << usage << "\n\n" << desc << "\n";
    return 1;
  }
  po::notify(*vm);

  po::positional_options_description p;
  p.add("input-file", 1);

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
/// @param[out] vm Variables map of program options.
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
      .fta_type(vm["analysis"].as<std::string>());

  return settings;
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
    // Initiate risk analysis.
    RiskAnalysis* ran = new RiskAnalysis();
    ran->AddSettings(ConstructSettings(vm));

    // Read input files and setup.
    std::string input_file = vm["input-file"].as<std::string>();

    // Process input and validate it.
    ran->ProcessInput(input_file);

    // Stop if only validation is requested.
    if (vm.count("validate")) {
      std::cout << "The files are VALID." << std::endl;
      return 0;
    }

    // Graph if requested.
    if (vm.count("graph-only")) {
      ran->GraphingInstructions();
      return 0;
    }

    ran->Analyze();

    // Report results.
    std::string output = "cli";  // Output to command line by default.
    if (vm.count("output")) {
      output = vm["output"].as<std::string>();
    }
    ran->Report(output);  // May throw boost exceptions according to Coverity.

    delete ran;

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
