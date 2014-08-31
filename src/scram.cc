/// @file scram.cc
/// Main entrance.
#include <iostream>
#include <string>

#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "fault_tree_analysis.h"
#include "risk_analysis.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace scram;

/// Command line SCRAM entrance.
/// @retuns 0 for success.
/// @retuns 1 for errored state.
int main(int argc, char* argv[]) {
  // Parse command line options.
  std::string usage = "Usage:    scram [input-file] [opts]";
  po::options_description desc("Allowed options");
  po::variables_map vm;

  try {
    desc.add_options()
        ("help,h", "produce help message")
        ("input-file", po::value<std::string>(),
         "xml input file with analysis entities")
        ("validate,v", "only validate input files")
        ("graph-only,g", "produce graph without analysis")
        ("analysis,a", po::value<std::string>()->default_value("fta-default"),
         "type of analysis to be performed on this input")
        ("rare-event-approx,r", "use the rare event approximation")
        ("mcub,m", "use the MCUB approximation for probability calculations")
        ("limit-order,l", po::value<int>()->default_value(20),
         "upper limit for cut set order")
        ("nsums,s", po::value<int>()->default_value(1000000),
         "number of sums in series expansion for probability calculations")
        ("cut-off,c", po::value<double>()->default_value(1e-8),
         "cut-off probability for cut sets")
        ("output,o", po::value<std::string>(), "output file")
        ;

    po::store(po::parse_command_line(argc, argv, desc), vm);
  } catch (std::exception& err) {
    std::cout << "Invalid arguments.\n"
              << usage << "\n\n" << desc << "\n";
    return 1;
  }
  po::notify(vm);

  po::positional_options_description p;
  p.add("input-file", 1);

  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).
            run(), vm);
  po::notify(vm);

  // Process command line args.
  if (vm.count("help")) {
    std::cout << usage << "\n\n" << desc << "\n";
    return 0;
  }

  if (!vm.count("input-file")) {
    std::string msg = "No input file given.\n";
    std::cout << usage << "\n\n" << desc << "\n";
    return 1;
  }

  try {
    // Determine required analysis.
    // FTA naive is assumed if no arguments are given.
    std::string analysis = vm["analysis"].as<std::string>();
    bool graph_only = false;
    bool rare_event = false;
    bool mcub = false;

    // Determin if only graphing instructions are requested.
    if (vm.count("graph-only")) graph_only = true;
    // Determine if the rare event approximation is requested.
    if (vm.count("rare-event-approx")) rare_event = true;
    // Determine if the MCUB approximation is requested.
    if (vm.count("mcub")) mcub = true;

    // Read input files and setup.
    std::string input_file = vm["input-file"].as<std::string>();

    // Initiate risk analysis.
    RiskAnalysis* ran = new RiskAnalysis();

    /// @todo New sequence and architecture of analysis
    /// Read configurations.
    /// Initializer from input files.
    /// Run analysis.

    FaultTreeAnalysis* fta;
    if (analysis == "fta-default" || analysis == "fta-mc") {
      if (vm["limit-order"].as<int>() < 1) {
        std::string msg = "Upper limit for cut sets can't be less than 1\n";
        std::cout << msg << std::endl;
        std::cout << desc << "\n";
        return 1;
      }

      if (vm["nsums"].as<int>() < 1) {
        std::string msg = "Number of sums for series can't be less than 1\n";
        std::cout << msg << std::endl;
        std::cout << desc << "\n";
        return 1;
      }

      if (vm["cut-off"].as<double>() < 0 || vm["cut-off"].as<double>() >= 1) {
        std::string msg = "Illegal value for the cut-off probability\n";
        std::cout << msg << std::endl;
        std::cout << desc << "\n";
        return 1;
      }
      std::string fta_analysis = "default";
      if (analysis == "fta-mc") fta_analysis = "mc";

      std::string approx = "no";
      if (rare_event) approx = "rare";
      if (mcub) approx = "mcub";

      fta = new FaultTreeAnalysis(fta_analysis, approx,
                                  vm["limit-order"].as<int>(),
                                  vm["nsums"].as<int>(),
                                  vm["cut-off"].as<double>());
    } else {
      std::string msg = analysis + ": this analysis is not recognized.\n";
      std::cout << msg << std::endl;
      std::cout << desc << "\n";
      return 1;
    }

    // Set the fault tree analysis type.
    ran->fta(fta);

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

    // Analyze.
    ran->Analyze();

    // Report results.
    std::string output = "cli";  // Output to command line by default.
    if (vm.count("output")) {
      output = vm["output"].as<std::string>();
    }
    ran->Report(output);  // May throw boost exceptions according to Coverity.

    delete ran;

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

  return 0;
}  // End of main.
