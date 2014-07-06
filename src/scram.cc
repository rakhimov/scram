// Main entrance.
#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "fault_tree.h"
#include "risk_analysis.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace scram;

int main(int argc, char* argv[]) {
  // Parse command line options.
  std::string usage = "Usage:    scram [input-file] [prob-file] [opts]";
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help,h", "produce help message")
      ("input-file", po::value<std::string>(),
       "input file with tree description")
      ("prob-file", po::value<std::string>(), "file with probabilities")
      ("validate,v", "only validate input files")
      ("graph-only,g", "produce graph without analysis")
      ("analysis,a", po::value<std::string>()->default_value("fta-default"),
       "type of analysis to be performed on this input")
      ("rare-event-approx,r", "use the rare event approximation")
      ("limit-order,l", po::value<int>()->default_value(20),
       "upper limit for cut set order")
      ("nsums,s", po::value<int>()->default_value(1000000),
       "number of sums in series expansion for probability calculations")
      ("output,o", po::value<std::string>(), "output file")
      ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
  } catch(std::exception err) {
    std::cout << "Invalid arguments.\n"
              << usage << "\n\n" << desc << "\n";
    return 1;
  }
  po::notify(vm);

  po::positional_options_description p;
  p.add("input-file", 1);
  p.add("prob-file", 2);

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

  // Determine required analysis.
  // FTA naive is assumed if no arguments are given.
  std::string analysis = vm["analysis"].as<std::string>();
  bool graph_only = false;
  bool rare_event = false;

  // Determin if only graphing instructions are requested.
  if (vm.count("graph-only")) graph_only = true;
  // Determine if a rare event approximation is requested.
  if (vm.count("rare-event-approx")) rare_event = true;

  // Read input files and setup.
  std::string input_file = vm["input-file"].as<std::string>();

  // Initiate risk analysis.
  RiskAnalysis* ran;

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

    ran = new FaultTree(analysis, graph_only, rare_event,
                        vm["limit-order"].as<int>(), vm["nsums"].as<int>());
  } else {
    std::string msg = analysis + ": this analysis is not recognized.\n";
    std::cout << msg << std::endl;
    std::cout << desc << "\n";
    return 1;
  }

  // Process input and validate it.
  ran->ProcessInput(input_file);

  if (vm.count("prob-file")) {
    std::string prob_file = vm["prob-file"].as<std::string>();
    // Populate probabilities.
    ran->PopulateProbabilities(prob_file);
  }

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
  ran->Report(output);

  delete ran;

  return 0;
}  // End of main.
