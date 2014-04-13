// Main entrance

#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "risk_analysis.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace scram;

int main(int argc, char* argv[]) {
  // parse command line options
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help,h", "produce help message")
      ("input-file,i", po::value<std::string>(),
       "input file with tree description")
      ("prob-file,p", po::value<std::string>(), "file with probabilities")
      ("validate,v", "only validate input files")
      ("graph-only,g", "produce graph without analysis")
      ("analysis,a", po::value<std::string>()->default_value("fta-naive"),
       "type of analysis to be performed on this input")
      ("rare-event-approx,r", "whether or not to use a rare event approximation")
      ("limit-order,l", po::value<int>()->default_value(20),
       "upper limit for cut set order")
      ("nsums,s", po::value<int>()->default_value(1000000),
       "number of sums in series expansion for probability calculations")
      ("output,o", po::value<std::string>(), "output file")
      ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  po::positional_options_description p;
  p.add("input-file", 1);
  p.add("prob-file", 2);

  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).
            run(), vm);
  po::notify(vm);

  // process command line args
  if (vm.count("help")) {
    std::string msg = "Scram requires a file with input description and a"
                      " file with probabilities for events.\n";
    std::cout << msg << std::endl;
    std::cout << desc << "\n";
    return 0;
  }

  if (!vm.count("input-file")) {
    std::string msg = "Scram requires an input file with a system description.\n";
    std::cout << msg << std::endl;
    std::cout << desc << "\n";
    return 0;
  }

  // determine required analysis
  // FTA naive is assumed if no arguments are given
  std::string analysis = vm["analysis"].as<std::string>();
  bool rare_event = false;

  // determine if a rare event approximation is requested
  if (vm.count("rare-event-approx")) rare_event = true;

  // read input files and setup.
  std::string input_file = vm["input-file"].as<std::string>();

  // initiate risk analysis
  RiskAnalysis* ran;

  if (analysis == "fta-naive") {
    if (vm["limit-order"].as<int>() < 1) {
      std::string msg = "Upper limit for cut sets can't be less than 1\n";
      std::cout << msg << std::endl;
      std::cout << desc << "\n";
      return 0;
    }

    if (vm["nsums"].as<int>() < 1) {
      std::string msg = "Number of sums for series can't be less than 1\n";
      std::cout << msg << std::endl;
      std::cout << desc << "\n";
      return 0;
    }

    ran = new FaultTree(analysis, rare_event, vm["limit-order"].as<int>(),
                        vm["nsums"].as<int>());
  }

  // process input and validate it
  ran->process_input(input_file);

  if (vm.count("prob-file")) {
    std::string prob_file = vm["prob-file"].as<std::string>();
    // populate probabilities
    ran->populate_probabilities(prob_file);
  }

  // stop if only validation is requested
  if (vm.count("validate")) {
    std::cout << "The files are VALID." << std::endl;
    return 0;
  }

  // graph if requested
  if (vm.count("graph-only")) {
    ran->graphing_instructions();
    return 0;
  }

  // analyze
  ran->analyze();

  // report results
  std::string output = "cli";  // output to command line by default
  if (vm.count("output")) {
    output = vm["output"].as<std::string>();
  }
  ran->report(output);

  return 0;
}  // end of main
