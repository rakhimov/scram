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

#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

namespace scram {
namespace core {
namespace test {

// Benchmark Tests for [A or B or C] fault tree.
TEST_P(RiskAnalysisTest, ABC) {
  std::string tree_input = "./share/scram/input/core/abc.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(0.6, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.496, p_total());
  }

  std::set<std::set<std::string>> mcs = {{"A"}, {"B"}, {"C"}};
  EXPECT_EQ(3, products().size());
  EXPECT_EQ(mcs, products());
}

// Benchmark Tests for [AB or BC] fault tree.
TEST_P(RiskAnalysisTest, ABorBC) {
  std::string tree_input = "./share/scram/input/core/ab_bc.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(0.08, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.074, p_total());
  }

  std::set<std::set<std::string>> mcs = {{"A", "B"}, {"B", "C"}};
  EXPECT_EQ(2, products().size());
  EXPECT_EQ(mcs, products());
}

// Benchmark Tests for [AB or ~AC] fault tree.
TEST_P(RiskAnalysisTest, ABorNotAC) {
  std::string tree_input = "./share/scram/input/core/ab_or_not_ac.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(0.32, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.29, p_total());
  }

  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"A", "B"}, {"not A", "C"},
                                          {"B", "C"}};
    EXPECT_EQ(3, products().size());
    EXPECT_EQ(pi, products());
  } else {
    std::set<std::set<std::string>> mcs = {{"A", "B"}, {"C"}};
    EXPECT_EQ(2, products().size());
    EXPECT_EQ(mcs, products());
  }
}

// Simple verification tests for K/N gate fault tree.
TEST_P(RiskAnalysisTest, Vote) {
  std::string tree_input = "./share/scram/input/core/atleast.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(0.11, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.098, p_total());
  }

  std::set<std::set<std::string>> mcs = {{"A", "B"}, {"B", "C"}, {"A", "C"}};
  EXPECT_EQ(3, products().size());
  EXPECT_EQ(mcs, products());
}

// Benchmark tests for NOT gate.
// [A OR NOT A]
// This produces UNITY top gate.
TEST_P(RiskAnalysisTest, AorNotA) {
  std::string tree_input = "./share/scram/input/core/a_or_not_a.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());

  // Special case of one empty cut set in a container.
  EXPECT_EQ(1, products().size());
  EXPECT_TRUE(products().begin()->empty());
}

// [A OR NOT B]
TEST_P(RiskAnalysisTest, AorNotB) {
  std::string tree_input = "./share/scram/input/core/a_or_not_b.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(1, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.82, p_total());
  }

  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"A"}, {"not B"}};
    EXPECT_EQ(2, products().size());
    EXPECT_EQ(pi, products());
  } else {
    EXPECT_EQ(kUnity, products());
  }
}

// [A AND NOT A]
TEST_P(RiskAnalysisTest, AandNotA) {
  std::string tree_input = "./share/scram/input/core/a_and_not_a.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(0, p_total());
  EXPECT_TRUE(products().empty());
}

// [A AND NOT B]
TEST_P(RiskAnalysisTest, AandNotB) {
  std::string tree_input = "./share/scram/input/core/a_and_not_b.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(0.1, p_total());
  } else {
    EXPECT_NEAR(0.08, p_total(), 1e-5);
  }

  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"A", "not B"}};
    EXPECT_EQ(1, products().size());
    EXPECT_EQ(pi, products());
  } else {
    std::set<std::set<std::string>> mcs = {{"A"}};
    EXPECT_EQ(1, products().size());
    EXPECT_EQ(mcs, products());
  }
}

// [A OR (B, NOT A)]
TEST_P(RiskAnalysisTest, AorNotAB) {
  std::string tree_input = "./share/scram/input/core/a_or_not_ab.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(0.3, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.28, p_total());
  }

  std::set<std::set<std::string>> mcs = {{"A"}, {"B"}};
  EXPECT_EQ(2, products().size());
  EXPECT_EQ(mcs, products());
}

// Uncertainty report for Unity case.
TEST_F(RiskAnalysisTest, MonteCarloAorNotA) {
  std::string tree_input = "./share/scram/input/core/a_or_not_a.xml";
  settings.uncertainty_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
}

// [A OR NOT B] FTA MC
TEST_F(RiskAnalysisTest, MonteCarloAorNotB) {
  settings.uncertainty_analysis(true);
  std::string tree_input = "./share/scram/input/core/a_or_not_b.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
}

// Repeated negative gate expansion.
TEST_P(RiskAnalysisTest, MultipleParentNegativeGate) {
  std::string tree_input = "./share/scram/input/core/"
                           "multiple_parent_negative_gate.xml";

  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(1, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.9, p_total());
  }

  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"not A"}};
    EXPECT_EQ(1, products().size());
    EXPECT_EQ(pi, products());
  } else {
    EXPECT_EQ(kUnity, products());
  }
}

// Checks for NAND gate.
TEST_P(RiskAnalysisTest, Nand) {
  std::string tree_input = "./share/scram/input/core/nand.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"not A"}, {"not B"}};
    EXPECT_EQ(2, products().size());
    EXPECT_EQ(pi, products());
  } else {
    EXPECT_EQ(kUnity, products());
  }
}

// Checks for NOR gate.
TEST_P(RiskAnalysisTest, Nor) {
  std::string tree_input = "./share/scram/input/core/nor.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"not A", "not B"}};
    EXPECT_EQ(pi, products());
  } else {
    EXPECT_EQ(kUnity, products());
  }
}

// Checks for NAND UNITY top gate cases.
TEST_P(RiskAnalysisTest, NandUnity) {
  std::string tree_input = "./share/scram/input/core/nand_or_equality.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());

  // Special case of one empty cut set in a container.
  EXPECT_EQ(1, products().size());
  EXPECT_TRUE(products().begin()->empty());
}

// Checks for OR UNITY top gate cases.
TEST_P(RiskAnalysisTest, OrUnity) {
  std::string tree_input = "./share/scram/input/core/not_and_or_equality.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());

  // Special case of one empty cut set in a container.
  EXPECT_EQ(1, products().size());
  EXPECT_TRUE(products().begin()->empty());
}

// Checks for UNITY due to house event.
TEST_P(RiskAnalysisTest, HouseUnity) {
  std::string tree_input = "./share/scram/input/core/unity.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());

  // Special case of one empty cut set in a container.
  EXPECT_EQ(1, products().size());
  EXPECT_TRUE(products().begin()->empty());
}

// Checks for NULL due to house event.
TEST_P(RiskAnalysisTest, HouseNull) {
  std::string tree_input = "./share/scram/input/core/null.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(0, p_total());

  // Special case of one empty cut set in a container.
  EXPECT_TRUE(products().empty());
}

// Checks for NAND UNITY top gate cases.
TEST_P(RiskAnalysisTest, SubtleUnity) {
  std::string tree_input = "./share/scram/input/core/subtle_unity.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());

  // Special case of one empty cut set in a container.
  EXPECT_EQ(1, products().size());
  EXPECT_TRUE(products().begin()->empty());
}

// Checks for NAND UNITY top gate cases.
TEST_P(RiskAnalysisTest, SubtleNull) {
  std::string tree_input = "./share/scram/input/core/subtle_null.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_TRUE(products().empty());
}

// Handling of complement a module.
TEST_P(RiskAnalysisTest, ComplementModule) {
  std::string tree_input = "./share/scram/input/core/complement_module.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {{"e1", "e2", "e3"}};
  EXPECT_EQ(mcs, products());
}

// Benchmark Tests for [A xor B xor C] fault tree.
TEST_P(RiskAnalysisTest, XorABC) {
  std::string tree_input = "./share/scram/input/core/xor.xml";
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(0.6, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.404, p_total());
  }

  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"A", "B", "C"},
                                          {"A", "not B", "not C"},
                                          {"not A", "B", "not C"},
                                          {"not A", "not B", "C"}};
    EXPECT_EQ(4, products().size());
    EXPECT_EQ(pi, products());
  } else {
    std::set<std::set<std::string>> mcs = {{"A"}, {"B"}, {"C"}};
    EXPECT_EQ(3, products().size());
    EXPECT_EQ(mcs, products());
  }
}

// Checks for top gate of NOT with a single basic event child.
TEST_P(RiskAnalysisTest, NotA) {
  std::string tree_input = "./share/scram/input/core/not_a.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());

  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"not OnlyChild"}};
    EXPECT_EQ(1, products().size());
    EXPECT_EQ(pi, products());
  } else {
    EXPECT_EQ(kUnity, products());
  }
}

// Checks for top gate of NULL with a single basic event child.
TEST_P(RiskAnalysisTest, NullA) {
  std::string tree_input = "./share/scram/input/core/null_a.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());

  std::set<std::set<std::string>> mcs = {{"OnlyChild"}};
  EXPECT_EQ(1, products().size());
  EXPECT_EQ(mcs, products());
}

// Benchmark Tests for Beta factor common cause failure model.
TEST_P(RiskAnalysisTest, BetaFactorCCF) {
  std::string tree_input = "./share/scram/input/core/beta_factor_ccf.xml";
  std::string p1 = "[PumpOne]";
  std::string p2 = "[PumpTwo]";
  std::string p3 = "[PumpThree]";
  std::string v1 = "[ValveOne]";
  std::string v2 = "[ValveTwo]";
  std::string v3 = "[ValveThree]";
  std::string pumps = "[PumpOne PumpThree PumpTwo]";
  std::string valves = "[ValveOne ValveThree ValveTwo]";

  settings.ccf_analysis(true).probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_NEAR(0.044096, p_total(), 1e-5);
  } else {
    EXPECT_NEAR(0.04308, p_total(), 1e-5);
  }
  // Minimal cut set check.
  std::set<std::set<std::string>> mcs = {{pumps},
                                         {valves},
                                         {v1, v2, v3},
                                         {p1, v2, v3},
                                         {p2, v1, v3},
                                         {p3, v1, v2},
                                         {p3, p2, v1},
                                         {p1, p2, v3},
                                         {p1, p3, v2},
                                         {p1, p2, p3}};
  EXPECT_EQ(10, products().size());
  EXPECT_EQ(mcs, products());
}

// Benchmark Tests for Phi factor common cause failure calculations.
TEST_P(RiskAnalysisTest, PhiFactorCCF) {
  std::string tree_input = "./share/scram/input/core/phi_factor_ccf.xml";
  settings.ccf_analysis(true).probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_NEAR(0.04434, p_total(), 1e-5);
  } else {
    EXPECT_NEAR(0.04104, p_total(), 1e-5);
  }
  EXPECT_EQ(34, products().size());
  std::vector<int> distr = {2, 24, 8};
  EXPECT_EQ(distr, ProductDistribution());
}

// Benchmark Tests for MGL factor common cause failure calculations.
TEST_P(RiskAnalysisTest, MGLFactorCCF) {
  std::string tree_input = "./share/scram/input/core/mgl_ccf.xml";
  settings.ccf_analysis(true).probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_NEAR(0.01771, p_total(), 1e-5);
  } else {
    EXPECT_NEAR(0.01630, p_total(), 1e-5);
  }
  EXPECT_EQ(34, products().size());
  std::vector<int> distr = {2, 24, 8};
  EXPECT_EQ(distr, ProductDistribution());
}

// Benchmark Tests for Alpha factor common cause failure calculations.
TEST_P(RiskAnalysisTest, AlphaFactorCCF) {
  std::string tree_input = "./share/scram/input/core/alpha_factor_ccf.xml";
  settings.ccf_analysis(true).probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_NEAR(0.05488, p_total(), 1e-5);
  } else {
    EXPECT_NEAR(0.05298, p_total(), 1e-5);
  }
  EXPECT_EQ(34, products().size());
  std::vector<int> distr = {2, 24, 8};
  EXPECT_EQ(distr, ProductDistribution());
}

}  // namespace test
}  // namespace core
}  // namespace scram
