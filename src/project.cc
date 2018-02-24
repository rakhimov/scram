/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

/// @file
/// Implementation of project configuration facilities.

#include "project.h"

#include <cassert>

#include <array>
#include <memory>
#include <optional>
#include <string_view>

#include <boost/predef.h>

#include <boost/exception/errinfo_at_line.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/algorithm.hpp>

#include "env.h"
#include "error.h"
#include "ext/version.h"
#include "version.h"

namespace fs = boost::filesystem;

namespace scram {

namespace {  // Path normalization helper.

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

Project::Project(const std::string& project_file) {
  static xml::Validator validator(env::project_schema());

  if (fs::exists(project_file) == false) {
    SCRAM_THROW(IOError("The configuration file does not exist."))
        << boost::errinfo_file_name(project_file);
  }

  xml::Document document(project_file);
  xml::Element root = document.root();
  std::string_view version = root.attribute("version");
  if (root.name() == "scram" && !version.empty()) {
    try {
      std::optional<std::array<int, 3>> numbers = ext::extract_version(version);
      if (!numbers)
        SCRAM_THROW(xml::ValidityError("Invalid version string"));
      std::array<int, 3> current_numbers = {
          SCRAM_VERSION_MAJOR, SCRAM_VERSION_MINOR, SCRAM_VERSION_MICRO};
      if (numbers > current_numbers)
        SCRAM_THROW(VersionError("Version incompatibility"));

    } catch (Error& err) {
      err << errinfo_value(std::string(version))
          << xml::errinfo_element("scram") << xml::errinfo_attribute("version")
          << boost::errinfo_at_line(root.line())
          << boost::errinfo_file_name(root.filename());
      throw;
    }
  }
  validator.validate(document);
  assert(root.name() == "scram");
  fs::path base_path = fs::path(project_file).parent_path();
  GatherInputFiles(root, base_path);

  try {
    GatherOptions(root);
  } catch (Error& err) {
    err << boost::errinfo_file_name(project_file);
    throw;
  }
}

void Project::GatherInputFiles(const xml::Element& root,
                               const fs::path& base_path) {
  std::optional<xml::Element> input_files = root.child("model");
  if (!input_files)
    return;
  for (xml::Element input_file : input_files->children()) {
    assert(input_file.name() == "file");
    input_files_.push_back(
        normalize(std::string(input_file.text()), base_path));
  }
}

void Project::GatherOptions(const xml::Element& root) {
  std::optional<xml::Element> options_element = root.child("options");
  if (!options_element)
    return;
  // The loop is used instead of query
  // because the order of options matters,
  // yet this function should not know what the order is.
  for (xml::Element option_group : options_element->children()) {
    try {
      std::string_view name = option_group.name();
      if (name == "algorithm") {
        settings_.algorithm(option_group.attribute("name"));

      } else if (name == "prime-implicants") {
        settings_.prime_implicants(true);

      } else if (name == "approximation") {
        settings_.approximation(option_group.attribute("name"));

      } else if (name == "limits") {
        SetLimits(option_group);
      }
    } catch (SettingsError& err) {
      err << boost::errinfo_at_line(option_group.line());
      throw;
    }
  }
  if (std::optional<xml::Element> analysis_group =
          options_element->child("analysis")) {
    try {
      SetAnalysis(*analysis_group);
    } catch (SettingsError& err) {
      err << boost::errinfo_at_line(analysis_group->line());
      throw;
    }
  }
}

void Project::SetAnalysis(const xml::Element& analysis) {
  auto set_flag = [&analysis](const char* tag, auto setter) {
    if (std::optional<bool> flag = analysis.attribute<bool>(tag))
      setter(*flag);
  };
  set_flag("probability",
           [this](bool flag) { settings_.probability_analysis(flag); });
  set_flag("importance",
           [this](bool flag) { settings_.importance_analysis(flag); });
  set_flag("uncertainty",
           [this](bool flag) { settings_.uncertainty_analysis(flag); });
  set_flag("ccf", [this](bool flag) { settings_.ccf_analysis(flag); });
  set_flag("sil",
           [this](bool flag) { settings_.safety_integrity_levels(flag); });
}

void Project::SetLimits(const xml::Element& limits) {
  for (xml::Element limit : limits.children()) {
    std::string_view name = limit.name();
    if (name == "product-order") {
      settings_.limit_order(limit.text<int>());

    } else if (name == "cut-off") {
      settings_.cut_off(limit.text<double>());

    } else if (name == "mission-time") {
      settings_.mission_time(limit.text<double>());

    } else if (name == "time-step") {
      settings_.time_step(limit.text<double>());

    } else if (name == "number-of-trials") {
      settings_.num_trials(limit.text<int>());

    } else if (name == "number-of-quantiles") {
      settings_.num_quantiles(limit.text<int>());

    } else if (name == "number-of-bins") {
      settings_.num_bins(limit.text<int>());

    } else if (name == "seed") {
      settings_.seed(limit.text<int>());
    }
  }
}

}  // namespace scram
