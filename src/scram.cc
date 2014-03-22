// Main entrance

#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main(int argc, char** argv[]) {
  // parse command line options
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help,h", "produce help message")
      ("tree-file,input-file,t,i", po::value<std::string>(),
       "input file with tree description")
      ("prob-file,p", po::value<std::string>(), "file with probabilities")
      ("graph-only,g", "produce graph without analysis")
      ("analysis,a", "type of analysis to be performed on this input")
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



}  // end of main
