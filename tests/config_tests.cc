/*
 * Copyright (C) 2014-2015, 2017-2018 Olzhas Rakhimov
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

#include "config.h"

#include <catch.hpp>

#include <boost/filesystem.hpp>

#include "error.h"

namespace scram::test {

// Test with a wrong input.
TEST_CASE("ConfigTest.IOError", "[config]") {
  std::string config_file = "./nonexistent_configurations.xml";
  REQUIRE_THROWS_AS(Config(config_file), IOError);
}

// Test with XML content validation issues.
TEST_CASE("ConfigTest.ValidityError", "[config]") {
  std::string config_file = "tests/input/fta/invalid_configuration.xml";
  REQUIRE_THROWS_AS(Config(config_file), xml::ValidityError);
}

// Test with XML content numerical issues.
TEST_CASE("ConfigTest.NumericalErrors", "[config]") {
  std::string config_file = "tests/input/fta/int_overflow_config.xml";
  REQUIRE_THROWS_AS(Config(config_file), xml::ValidityError);
}

// Tests all settings with one file.
TEST_CASE("ConfigTest.FullSettings", "[config]") {
  std::string config_file = "tests/input/fta/full_configuration.xml";
  std::string cwd = boost::filesystem::current_path().generic_string();
  Config config(config_file);
  // Check the input files.
  CHECK(config.input_files().size() == 1);
  if (!config.input_files().empty()) {
    auto prob = cwd + "/tests/input/fta/correct_tree_input_with_probs.xml";
    CHECK(config.input_files().back() == prob);
  }
  // Check the output destination.
  auto out_dest = cwd + "/tests/input/fta/./temp_results.xml";
  CHECK(config.output_path() == out_dest);

  const core::Settings& settings = config.settings();
  CHECK(settings.algorithm() == core::Algorithm::kBdd);
  CHECK_FALSE(settings.prime_implicants());
  CHECK(settings.probability_analysis());
  CHECK(settings.importance_analysis());
  CHECK(settings.uncertainty_analysis());
  CHECK(settings.ccf_analysis());
  CHECK(settings.safety_integrity_levels());
  CHECK(settings.approximation() == core::Approximation::kRareEvent);
  CHECK(settings.limit_order() == 11);
  CHECK(settings.mission_time() == 48);
  CHECK(settings.time_step() == 1);
  CHECK(settings.cut_off() == 0.009);
  CHECK(settings.num_trials() == 777);
  CHECK(settings.num_quantiles() == 13);
  CHECK(settings.num_bins() == 31);
  CHECK(settings.seed() == 97531);
}

TEST_CASE("ConfigTest.PrimeImplicantsSettings", "[config]") {
  std::string config_file = "tests/input/fta/pi_configuration.xml";
  std::string cwd = boost::filesystem::current_path().generic_string();
  Config config(config_file);
  // Check the input files.
  CHECK(config.input_files().size() == 1);
  if (!config.input_files().empty()) {
    auto prob = cwd + "/tests/input/fta/correct_tree_input_with_probs.xml";
    CHECK(config.input_files().back() == prob);
  }
  // Check the output destination.
  auto out_dest = cwd + "/tests/input/fta/temp_results.xml";
  CHECK(config.output_path() == out_dest);

  const core::Settings& settings = config.settings();
  CHECK(settings.algorithm() == core::Algorithm::kBdd);
  CHECK(settings.prime_implicants());
}

TEST_CASE("ConfigTest.CanonicalPath", "[config]") {
  std::string config_file = "tests/input/win_path_in_config.xml";
  std::string cwd = boost::filesystem::current_path().generic_string();
  Config config(config_file);
  // Check the input files.
  REQUIRE(config.input_files().size() == 1);
  auto prob = cwd + "/tests/input/fta/correct_tree_input_with_probs.xml";
  REQUIRE(config.input_files().back() == prob);
}

}  // namespace scram::test
