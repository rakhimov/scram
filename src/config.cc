/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file config.cc
/// Implementation of configuration facilities.

#include "config.h"

#include <cassert>

#include <fstream>
#include <memory>
#include <sstream>

#include "env.h"
#include "error.h"
#include "xml_parser.h"

namespace scram {

Config::Config(const std::string& config_file) {
  std::ifstream file_stream(config_file.c_str());
  if (!file_stream) {
    throw IOError("The file '" + config_file + "' could not be loaded.");
  }

  std::stringstream stream;
  stream << file_stream.rdbuf();
  file_stream.close();

  std::unique_ptr<XmlParser> parser;
  try {
    parser = std::make_unique<XmlParser>(stream);
    std::stringstream schema;
    std::string schema_path = Env::config_schema();
    std::ifstream schema_stream(schema_path.c_str());
    schema << schema_stream.rdbuf();
    schema_stream.close();
    parser->Validate(schema);
  } catch (ValidationError& err) {
    err.msg("In file '" + config_file + "', " + err.msg());
    throw;
  }
  const xmlpp::Document* doc = parser->Document();
  const xmlpp::Node* root = doc->get_root_node();
  assert(root->get_name() == "config");
  Config::GatherInputFiles(root);
  Config::GetOutputPath(root);
  try {
    Config::GatherOptions(root);
  } catch (Error& err) {
    err.msg("In file '" + config_file + "', " + err.msg());
    throw;
  }
}

void Config::GatherInputFiles(const xmlpp::Node* root) {
  xmlpp::NodeSet input_files = root->find("./input-files");
  if (input_files.empty()) return;
  assert(input_files.size() == 1);
  const xmlpp::Element* files =
      static_cast<const xmlpp::Element*>(input_files.front());
  xmlpp::NodeSet all_files = files->find("./*");
  assert(!all_files.empty());
  for (const xmlpp::Node* node : all_files) {
    const xmlpp::Element* file = static_cast<const xmlpp::Element*>(node);
    assert(file->get_name() == "file");
    input_files_.push_back(file->get_child_text()->get_content());
  }
}

void Config::GatherOptions(const xmlpp::Node* root) {
  xmlpp::NodeSet options = root->find("./options");
  if (options.empty()) return;
  assert(options.size() == 1);
  const xmlpp::Element* element =
      static_cast<const xmlpp::Element*>(options.front());
  xmlpp::NodeSet all_options = element->find("./*");
  assert(!all_options.empty());
  int line_number = 0;  // For error reporting.
  try {
    // The loop is used instead of query
    // because the order of options matters,
    // yet this function should not know what the order is.
    for (const xmlpp::Node* node : all_options) {
      line_number = node->get_line();
      const xmlpp::Element* option_group =
          static_cast<const xmlpp::Element*>(node);
      std::string name = option_group->get_name();
      if (name == "algorithm") {
        Config::SetAlgorithm(option_group);

      } else if (name == "analysis") {
        Config::SetAnalysis(option_group);

      } else if (name == "prime-implicants") {
        settings_.prime_implicants(true);

      } else if (name == "approximation") {
        Config::SetApproximation(option_group);

      } else if (name == "limits") {
        Config::SetLimits(option_group);
      }
    }
  } catch (InvalidArgument& err) {
    std::stringstream msg;
    msg << "Line " << line_number << ":\n";
    err.msg(msg.str() + err.msg());
    throw;
  }
}

void Config::GetOutputPath(const xmlpp::Node* root) {
  xmlpp::NodeSet out = root->find("./output-path");
  if (out.empty()) return;
  assert(out.size() == 1);
  const xmlpp::Element* element =
      static_cast<const xmlpp::Element*>(out.front());
  output_path_ = element->get_child_text()->get_content();
}

void Config::SetAlgorithm(const xmlpp::Element* analysis) {
  settings_.algorithm(analysis->get_attribute_value("name"));
}

void Config::SetAnalysis(const xmlpp::Element* analysis) {
  for (const xmlpp::Attribute* type : analysis->get_attributes()) {
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

void Config::SetApproximation(const xmlpp::Element* approx) {
  settings_.approximation(approx->get_attribute_value("name"));
}

void Config::SetLimits(const xmlpp::Element* limits) {
  for (const xmlpp::Node* node : limits->find("./*")) {
    const xmlpp::Element* limit = static_cast<const xmlpp::Element*>(node);
    std::string name = limit->get_name();
    if (name == "product-order") {
      settings_.limit_order(CastChildText<int>(limit));

    } else if (name == "cut-off") {
      settings_.cut_off(CastChildText<double>(limit));

    } else if (name == "mission-time") {
      settings_.mission_time(CastChildText<double>(limit));

    } else if (name == "number-of-trials") {
      settings_.num_trials(CastChildText<int>(limit));

    } else if (name == "number-of-quantiles") {
      settings_.num_quantiles(CastChildText<int>(limit));

    } else if (name == "number-of-bins") {
      settings_.num_bins(CastChildText<int>(limit));

    } else if (name == "seed") {
      settings_.seed(CastChildText<int>(limit));
    }
  }
}

bool Config::GetBoolFromString(const std::string& flag) {
  assert(flag == "1" || flag == "true" || flag == "0" || flag == "false");
  if (flag == "1" || flag == "true") return true;
  return false;
}

}  // namespace scram
