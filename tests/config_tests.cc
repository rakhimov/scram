#include "config.h"

#include <gtest/gtest.h>

#include "error.h"

using namespace scram;

// Test with a wrong input.
TEST(ConfigTest, FullSettings) {
  std::string config_file = "./nonexistent_configurations.xml";
  ASSERT_THROW(Config(config_file), IOError);
}

// Tests all settings with one file.
TEST(ConfigTest, FullSettings) {
  std::string config_file = "./share/scram/input/fta/full_configuration.xml";
  Config* config = new Config(config_file);
  // Check the input files.
  ASSERT_EQ(config->input_files().size(), 1);
  EXPECT_EQ(config->input_files().back(),
            "./input/fta/correct_tree_input_with_probs.xml");
  // Check the output destination.
  EXPECT_EQ(config->output_path(), "temp_results.xml");
  // Check options.
  Settings settings;
  settings.probability_analysis(true).importance_analysis(true)
      .uncertainty_analysis(true).ccf_analysis(true)
      .approx("rare-event").limit_order(11).mission_time(48)
      .cut_off(0.009).num_sums(42).num_trials(777).seed(97531);
  EXPECT_EQ(settings, config->settings());

  delete config;
}
