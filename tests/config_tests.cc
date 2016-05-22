/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

#include <memory>

#include <gtest/gtest.h>

#include "error.h"

namespace scram {
namespace test {

// Test with a wrong input.
TEST(ConfigTest, IOError) {
  std::string config_file = "./nonexistent_configurations.xml";
  ASSERT_THROW(Config config(config_file), IOError);
}

// Test with XML content validation issues.
TEST(ConfigTest, ValidationError) {
  std::string config_file = "./share/scram/input/fta/invalid_configuration.xml";
  ASSERT_THROW(Config config(config_file), ValidationError);
}

// Test with XML content numerical issues.
TEST(ConfigTest, NumericalErros) {
  std::string config_file = "./share/scram/input/fta/int_overflow_config.xml";
  ASSERT_THROW(Config config(config_file), ValidationError);
}

// Tests all settings with one file.
TEST(ConfigTest, FullSettings) {
  std::string config_file = "./share/scram/input/fta/full_configuration.xml";
  std::unique_ptr<Config> config(new Config(config_file));
  // Check the input files.
  EXPECT_EQ(config->input_files().size(), 1);
  if (!config->input_files().empty())
    EXPECT_EQ("input/fta/correct_tree_input_with_probs.xml",
              config->input_files().back());
  // Check the output destination.
  EXPECT_EQ("temp_results.xml", config->output_path());

  const core::Settings& settings = config->settings();
  EXPECT_EQ("bdd", settings.algorithm());
  EXPECT_FALSE(settings.prime_implicants());
  EXPECT_EQ(true, settings.probability_analysis());
  EXPECT_EQ(true, settings.importance_analysis());
  EXPECT_EQ(true, settings.uncertainty_analysis());
  EXPECT_EQ(true, settings.ccf_analysis());
  EXPECT_EQ("rare-event", settings.approximation());
  EXPECT_EQ(11, settings.limit_order());
  EXPECT_EQ(48, settings.mission_time());
  EXPECT_EQ(0.009, settings.cut_off());
  EXPECT_EQ(777, settings.num_trials());
  EXPECT_EQ(13, settings.num_quantiles());
  EXPECT_EQ(31, settings.num_bins());
  EXPECT_EQ(97531, settings.seed());
}

TEST(ConfigTest, PrimeImplicantsSettings) {
  std::string config_file = "./share/scram/input/fta/pi_configuration.xml";
  std::unique_ptr<Config> config(new Config(config_file));
  // Check the input files.
  EXPECT_EQ(config->input_files().size(), 1);
  if (!config->input_files().empty())
    EXPECT_EQ("input/fta/correct_tree_input_with_probs.xml",
              config->input_files().back());
  // Check the output destination.
  EXPECT_EQ("temp_results.xml", config->output_path());

  const core::Settings& settings = config->settings();
  EXPECT_EQ("bdd", settings.algorithm());
  EXPECT_TRUE(settings.prime_implicants());
}

}  // namespace test
}  // namespace scram
