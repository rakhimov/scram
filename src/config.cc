/// @file config.cc
/// Implementation of configuration facilities.
#include "config.h"

#include <fstream>
#include <sstream>

#include <boost/lexical_cast.hpp>

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
  const xmlpp::Document* doc = parser->Document();
  const xmlpp::Node* root = doc->get_root_node();
  assert(root->get_name() == "config");
  xmlpp::NodeSet roots_children = root->find("./*");
  xmlpp::NodeSet::iterator it_ch;
  for (it_ch = roots_children.begin();
       it_ch != roots_children.end(); ++it_ch) {
    const xmlpp::Element* element =
        dynamic_cast<const xmlpp::Element*>(*it_ch);
    assert(element);

    std::string name = element->get_name();
    if (name == "input-files") {
      xmlpp::NodeSet input_files = element->find("./*");
      assert(!input_files.empty());
      xmlpp::NodeSet::iterator it_if;
      for (it_if = input_files.begin(); it_if != input_files.end(); ++it_if) {
        const xmlpp::Element* file =
            dynamic_cast<const xmlpp::Element*>(*it_if);
        assert(file);
        assert(file->get_name() == "file");
        input_files_.push_back(file->get_child_text()->get_content());
      }
    } else if (name == "output-path") {
      output_path_ = element->get_child_text()->get_content();

    } else if (name == "options") {
      xmlpp::NodeSet options = element->find("./*");
      xmlpp::NodeSet::iterator it_op;
      for (it_op = options.begin(); it_op != options.end(); ++it_op) {
        const xmlpp::Element* option_group =
            dynamic_cast<const xmlpp::Element*>(*it_op);
        assert(option_group);
        std::string name = option_group->get_name();
        if (name == "analysis") {
          Config::SetAnalysis(option_group);

        } else if (name == "approximations") {
          Config::SetApprox(option_group);

        } else if (name == "limits") {
          Config::SetLimits(option_group);
        }
      }
    }
  }
}

void Config::SetAnalysis(const xmlpp::Element* analysis) {
  const xmlpp::Element::AttributeList attr = analysis->get_attributes();
  xmlpp::Element::AttributeList::const_iterator it;
  for (it = attr.begin(); it != attr.end(); ++it) {
    const xmlpp::Attribute* type = *it;
    std::string name = type->get_name();
    bool flag = Config::GetBoolFromString(type->get_value());
    if (name == "probability") {
      settings_.probability_analysis(flag);

    } else if (name == "importance") {
      settings_.importance_analysis(flag);

    } else if (name == "uncertainty") {
      settings_.uncertainty_analysis(flag);

    } else if (name == "ccf") {
      settings_.ccf_analysis(flag);
    }
  }
}

void Config::SetApprox(const xmlpp::Element* approx) {
  xmlpp::NodeSet elements = approx->find("./*");
  assert(elements.size() == 1);
  xmlpp::NodeSet::iterator it;
  for (it = elements.begin(); it != elements.end(); ++it) {
    std::string name = (*it)->get_name();
    assert(name == "rare-event" || name == "mcub");
    settings_.approx(name);
  }
}

void Config::SetLimits(const xmlpp::Element* limits) {
  xmlpp::NodeSet elements = limits->find("./*");
  xmlpp::NodeSet::iterator it;
  for (it = elements.begin(); it != elements.end(); ++it) {
    const xmlpp::Element* limit =
        dynamic_cast<const xmlpp::Element*>(*it);
    assert(limit);
    std::string name = limit->get_name();
    std::string content = limit->get_child_text()->get_content();
    if (name == "limit-order") {
      settings_.limit_order(boost::lexical_cast<int>(content));

    } else if (name == "cut-off") {
      settings_.cut_off(boost::lexical_cast<double>(content));

    } else if (name == "number-of-sums") {
      settings_.num_sums(boost::lexical_cast<int>(content));

    } else if (name == "mission-time") {
      settings_.mission_time(boost::lexical_cast<double>(content));

    } else if (name == "number-of-trials") {
      settings_.num_trials(boost::lexical_cast<int>(content));

    } else if (name == "seed") {
      settings_.seed(boost::lexical_cast<int>(content));
    }
  }
}

bool Config::GetBoolFromString(std::string flag) {
  assert(flag == "1" || flag == "true" || flag == "0" || flag == "false");
  if (flag == "1" || flag == "true") return true;
  return false;
}

}  // namespace scram
