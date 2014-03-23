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
      ("graph-only,g", "produce graph without analysis")
      ("analysis,a", po::value<std::string>(),
       "type of analysis to be performed on this input")
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
  } else if (!vm.count("prob-file")) {
    std::string msg = "Scram requires a file with probabilities for events.\n";
    std::cout << msg << std::endl;
    std::cout << desc << "\n";
    return 0;
  }

  // determine required analysis
  // FTA naive is assumed if no arguments are given
  std::string analysis = "fta-naive";
  if (!vm.count("analysis")) {
    std::string msg = "WARNING: FTA naive is assumed.\n";
    std::cout << msg << std::endl;
  } else {
    analysis = vm["analysis"].as<std::string>();
  }

  // read input files and setup.
  std::string input_file = vm["input-file"].as<std::string>();
  std::string prob_file = vm["prob-file"].as<std::string>();

  // initiate risk analysis
  RiskAnalysis* ran;

  if (analysis == "fta-naive") {
      ran = new FaultTree(analysis);
  }

  // process input
  ran->process_input(input_file);

  // populate probabilities
  ran->populate_probabilities(prob_file);

  // graph if requested
  if (vm.count("graph-only")) {
    ran->graphing_instructions();
    return 0;
  }

  // analyze
  ran->analyze();

  return 0;

}  // end of main
