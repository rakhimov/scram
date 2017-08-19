/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

#include <memory>

#include <boost/filesystem.hpp>
#include <boost/predef.h>
#include <boost/range/algorithm.hpp>

#include "env.h"
#include "error.h"
#include "xml.h"

namespace fs = boost::filesystem;

namespace scram {

namespace {  // Path normalization helpers.

std::string normalize(const std::string& file_path, const fs::path& base_path) {
  fs::path abs_path = fs::absolute(file_path, base_path).generic_string();
#if BOOST_OS_WINDOWS
  return abs_path.generic_string();
#else
  std::string abnormal_path = abs_path.string();
  boost::replace(abnormal_path, '\\', '/');
  return abnormal_path;
#endif
}

}  // namespace

Config::Config(const std::string& config_file) {
  static xmlpp::RelaxNGValidator validator(Env::config_schema());

  if (fs::exists(config_file) == false)
    throw IOError("The file '" + config_file + "' could not be loaded.");

  std::unique_ptr<xmlpp::DomParser> parser = ConstructDomParser(config_file);
  try {
    validator.validate(parser->get_document());
  } catch (const xmlpp::validity_error&) {
    throw ValidationError("Config XML failed schema validation:\n" +
                          xmlpp::format_xml_error());
  }
  const xmlpp::Node* root = parser->get_document()->get_root_node();
  assert(root->get_name() == "scram");
  fs::path base_path = fs::path(config_file).parent_path();
  GatherInputFiles(root, base_path);
  GetOutputPath(root, base_path);
  try {
    GatherOptions(root);
  } catch (Error& err) {
    err.msg("In file '" + config_file + "', " + err.msg());
    throw;
  }
}

void Config::GatherInputFiles(const xmlpp::Node* root,
                              const fs::path& base_path) {
  xmlpp::NodeSet input_files = root->find("./input-files");
  if (input_files.empty())
    return;
  assert(input_files.size() == 1);
  const xmlpp::Element* files = XmlElement(input_files.front());
  xmlpp::NodeSet all_files = files->find("./*");
  assert(!all_files.empty());
  for (const xmlpp::Node* node : all_files) {
    const xmlpp::Element* file = XmlElement(node);
    assert(file->get_name() == "file");
    input_files_.push_back(
        normalize(GetContent(file->get_child_text()), base_path));
  }
}

void Config::GatherOptions(const xmlpp::Node* root) {
  xmlpp::NodeSet options = root->find("./options");
  if (options.empty())
    return;
  assert(options.size() == 1);
  const xmlpp::Element* element = XmlElement(options.front());
  const xmlpp::Element* option_group = nullptr;  // For error reporting.
  try {
    const xmlpp::Element* analysis_group = nullptr;  // Needs to be set last.
    // The loop is used instead of query
    // because the order of options matters,
    // yet this function should not know what the order is.
    for (const xmlpp::Node* node : element->find("./*")) {
      option_group = XmlElement(node);
      std::string name = option_group->get_name();
      if (name == "algorithm") {
        SetAlgorithm(option_group);

      } else if (name == "analysis") {
        analysis_group = option_group;

      } else if (name == "prime-implicants") {
        settings_.prime_implicants(true);

      } else if (name == "approximation") {
        SetApproximation(option_group);

      } else if (name == "limits") {
        SetLimits(option_group);
      }
    }
    if (analysis_group) {
      option_group = analysis_group;
      SetAnalysis(analysis_group);
    }
  } catch (InvalidArgument& err) {
    err.msg(GetLine(option_group) + err.msg());
    throw;
  }
}

void Config::GetOutputPath(const xmlpp::Node* root, const fs::path& base_path) {
  xmlpp::NodeSet out = root->find("./output-path");
  if (out.empty())
    return;
  assert(out.size() == 1);
  const xmlpp::Element* element = XmlElement(out.front());
  output_path_ = normalize(GetContent(element->get_child_text()), base_path);
}

void Config::SetAlgorithm(const xmlpp::Element* analysis) {
  settings_.algorithm(GetAttributeValue(analysis, "name"));
}

void Config::SetAnalysis(const xmlpp::Element* analysis) {
  for (const xmlpp::Attribute* type : analysis->get_attributes()) {
    std::string name = type->get_name();
    bool flag = CastAttributeValue<bool>(type);
    if (name == "probability") {
      settings_.probability_analysis(flag);

    } else if (name == "importance") {
      settings_.importance_analysis(flag);

    } else if (name == "uncertainty") {
      settings_.uncertainty_analysis(flag);

    } else if (name == "ccf") {
      settings_.ccf_analysis(flag);

    } else if (name == "sil") {
      settings_.safety_integrity_levels(flag);
    }
  }
}

void Config::SetApproximation(const xmlpp::Element* approx) {
  settings_.approximation(GetAttributeValue(approx, "name"));
}

void Config::SetLimits(const xmlpp::Element* limits) {
  for (const xmlpp::Node* node : limits->find("./*")) {
    const xmlpp::Element* limit = XmlElement(node);
    std::string name = limit->get_name();
    if (name == "product-order") {
      settings_.limit_order(CastChildText<int>(limit));

    } else if (name == "cut-off") {
      settings_.cut_off(CastChildText<double>(limit));

    } else if (name == "mission-time") {
      settings_.mission_time(CastChildText<double>(limit));

    } else if (name == "time-step") {
      settings_.time_step(CastChildText<double>(limit));

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

}  // namespace scram
