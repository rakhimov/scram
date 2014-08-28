#include <gtest/gtest.h>

#include "fault_tree_analysis_tests.h"

// Benchmark Tests for  fault tree from OpenFTA.
TEST_F(FaultTreeAnalysisTest, ThreeMotor) {
  std::string tree_input = "./share/scram/input/benchmark/three_motor.xml";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.
  std::string T3 = "t3";
  std::string S1 = "s1";
  std::string T4 = "t4";
  std::string KT2 = "kt2";
  std::string KT3 = "kt3";
  std::string KT1 = "kt1";
  std::string T1 = "t1";
  std::string K2 = "k2";
  std::string T2 = "t2";
  std::string K5 = "k5";
  std::string K1 = "k1";
  std::string KT1inc = "kt1inc";
  std::string KT2inc = "kt2inc";
  std::string KT3inc = "kt3inc";
  std::string T1inc = "t1inc";
  std::string T2inc = "t2inc";
  std::string T3inc = "t3inc";
  std::string T4inc = "t4inc";

  // Check the tree with the transfer gate.
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  nsums(3);
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_NO_THROW(ran->Report("/dev/null"));
  double delta_sqr = std::abs(p_total() - 0.0211538);
  EXPECT_TRUE(delta_sqr < 1e-5);
  // Minimal cut set check.
  // Order 2
  cut_set.insert(S1);
  cut_set.insert(T2);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K5);
  cut_set.insert(T3);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K5);
  cut_set.insert(T1);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K5);
  cut_set.insert(S1);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert(K5);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K1);
  cut_set.insert(T2);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K1);
  cut_set.insert(K5);
  mcs.insert(cut_set);
  cut_set.clear();

  // Order 3
  cut_set.insert(T1inc);
  cut_set.insert(T2);
  cut_set.insert(T3);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(T1);
  cut_set.insert(T1inc);
  cut_set.insert(T2);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert(T1inc);
  cut_set.insert(T2);
  mcs.insert(cut_set);
  cut_set.clear();

  // Order 4
  cut_set.insert(K5);
  cut_set.insert(KT1);
  cut_set.insert(KT2);
  cut_set.insert(KT3);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K5);
  cut_set.insert(KT1);
  cut_set.insert(KT3);
  cut_set.insert(T4);
  mcs.insert(cut_set);
  cut_set.clear();

  // Order 5
  cut_set.insert(KT1);
  cut_set.insert(KT2);
  cut_set.insert(KT3);
  cut_set.insert(T1inc);
  cut_set.insert(T2);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT3);
  cut_set.insert(T1inc);
  cut_set.insert(T2);
  cut_set.insert(T4);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3);
  cut_set.insert(T3inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(T1);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3);
  cut_set.insert(T3inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT3inc);
  cut_set.insert(T1);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT2inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT2inc);
  cut_set.insert(T1);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT2inc);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT2inc);
  cut_set.insert( KT3inc);
  cut_set.insert(T1);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1inc);
  cut_set.insert( T2);
  cut_set.insert(T3);
  cut_set.insert(T3inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1inc);
  cut_set.insert( T1);
  cut_set.insert(T2);
  cut_set.insert(T3inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1inc);
  cut_set.insert( KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T3);
  cut_set.insert(T3inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1inc);
  cut_set.insert( KT3inc);
  cut_set.insert(T1);
  cut_set.insert(T2);
  cut_set.insert(T3inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1inc);
  cut_set.insert( KT2inc);
  cut_set.insert(T2);
  cut_set.insert(T3);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1inc);
  cut_set.insert( KT2inc);
  cut_set.insert(T1);
  cut_set.insert(T2);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1inc);
  cut_set.insert( KT2inc);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T3);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1inc);
  cut_set.insert( KT2inc);
  cut_set.insert(KT3inc);
  cut_set.insert(T1);
  cut_set.insert(T2);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert( T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert( KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert( KT2inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert( KT2inc);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert( KT1inc);
  cut_set.insert(T2);
  cut_set.insert(T3inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert( KT1inc);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T3inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert( KT1inc);
  cut_set.insert(KT2inc);
  cut_set.insert(T2);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(K2);
  cut_set.insert( KT1inc);
  cut_set.insert(KT2inc);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  mcs.insert(cut_set);
  cut_set.clear();

  // Order 7
  cut_set.insert(KT1);
  cut_set.insert(KT2);
  cut_set.insert(KT3);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT2);
  cut_set.insert(KT3);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT2);
  cut_set.insert(KT2inc);
  cut_set.insert(KT3);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT2);
  cut_set.insert(KT2inc);
  cut_set.insert(KT3);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT1inc);
  cut_set.insert(KT2);
  cut_set.insert(KT3);
  cut_set.insert(T2);
  cut_set.insert(T3inc);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT1inc);
  cut_set.insert(KT2);
  cut_set.insert(KT3);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T3inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT1inc);
  cut_set.insert(KT2);
  cut_set.insert(KT2inc);
  cut_set.insert(KT3);
  cut_set.insert(T2);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT1inc);
  cut_set.insert(KT2);
  cut_set.insert(KT2inc);
  cut_set.insert(KT3);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT3);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3inc);
  cut_set.insert(T4);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert( KT3);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T3inc);
  cut_set.insert(T4);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert( KT2inc);
  cut_set.insert(KT3);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T4);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert( KT2inc);
  cut_set.insert(KT3);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T2inc);
  cut_set.insert(T4);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert( KT1inc);
  cut_set.insert(KT3);
  cut_set.insert(T2);
  cut_set.insert(T3inc);
  cut_set.insert(T4);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT1inc);
  cut_set.insert(KT3);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T3inc);
  cut_set.insert(T4);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT1inc);
  cut_set.insert(KT2inc);
  cut_set.insert(KT3);
  cut_set.insert(T2);
  cut_set.insert(T4);
  cut_set.insert(T4inc);
  mcs.insert(cut_set);
  cut_set.clear();

  cut_set.insert(KT1);
  cut_set.insert(KT1inc);
  cut_set.insert(KT2inc);
  cut_set.insert(KT3);
  cut_set.insert(KT3inc);
  cut_set.insert(T2);
  cut_set.insert(T4);
  mcs.insert(cut_set);
  cut_set.clear();

  EXPECT_EQ(54, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
