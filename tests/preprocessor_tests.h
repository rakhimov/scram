#ifndef SCRAM_TESTS_PREPROCESSOR_TESTS_H_
#define SCRAM_TESTS_PREPROCESSOR_TESTS_H_

#include "preprocessor.h"

#include <gtest/gtest.h>

using namespace scram;

/// @class PreprocessorTest
/// This test fixture is for white-box testing of algorithms for fault tree
/// preprossing.
class PreprocessorTest : public ::testing::Test {
 protected:
  typedef boost::shared_ptr<IGate> IGatePtr;

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  bool ProcessConstantChild(const IGatePtr& gate, int child,
                            bool state, std::vector<int>* to_erase) {
    return false;
  }

  // Members for tests.
  Preprocessor* prep;
  IndexedFaultTree* fault_tree;
};

#endif // SCRAM_TESTS_PREPROCESSOR_TESTS_H_
