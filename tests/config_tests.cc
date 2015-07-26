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

#include <gtest/gtest.h>

#include "error.h"

using namespace scram;

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

// Tests all settings with one file.
TEST(ConfigTest, FullSettings) {
  std::string config_file = "./share/scram/input/fta/full_configuration.xml";
  Config* config = new Config(config_file);
  // Check the input files.
  EXPECT_EQ(config->input_files().size(), 1);
  if (!config->input_files().empty())
    EXPECT_EQ("input/fta/correct_tree_input_with_probs.xml",
              config->input_files().back());
  // Check the output destination.
  EXPECT_EQ("temp_results.xml", config->output_path());
  // Check options.
  Settings settings;
  settings.probability_analysis(true).importance_analysis(true)
      .uncertainty_analysis(true).ccf_analysis(true)
      .approx("rare-event").limit_order(11).mission_time(48)
      .cut_off(0.009).num_sums(42).num_trials(777).seed(97531);
  EXPECT_EQ(settings, config->settings());

  delete config;
}
