/// @file config.cc
/// Implementation of configuration facilities.
#include "config.h"

#include <fstream>
#include <sstream>

#include "env.h"
#include "error.h"
#include "xml_parser.h"

namespace scram {

Config::Config(std::string config_file) : output_path_("") {
  std::ifstream file_stream(config_file.c_str());
  if (!file_stream) {
    throw IOError("The file '" + config_file + "' could not be loaded.");
  }

  std::stringstream stream;
  stream << file_stream.rdbuf();
  file_stream.close();

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  try {
    parser->Init(stream);
    std::stringstream schema;
    std::string schema_path = Env::config_schema();
    std::ifstream schema_stream(schema_path.c_str());
    schema << schema_stream.rdbuf();
    schema_stream.close();
    parser->Validate(schema);

  } catch (ValidationError& err) {
    err.msg("In file '" + config_file + "', " + err.msg());
    throw err;
  }
  xmlpp::Document* doc = parser->Document();
  const xmlpp::Node* root = doc->get_root_node();
  assert(root->get_name() == "config");
}

}  // namespace scram
