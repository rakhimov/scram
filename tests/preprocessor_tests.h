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
  typedef boost::shared_ptr<IndexedGate> IndexedGatePtr;

  virtual void SetUp() {
    fault_tree = new IndexedFaultTree(-1);  // The index is defined by tests.
    prep = new Preprocessor(fault_tree);
  }

  virtual void TearDown() {
    delete fault_tree;
    delete prep;
  }

  bool ProcessConstantChild(const IndexedGatePtr& gate, int child,
                            bool state, std::vector<int>* to_erase) {
    return prep->ProcessConstantChild(gate, child, state, to_erase);
  }

  // Members for tests.
  Preprocessor* prep;
  IndexedFaultTree* fault_tree;
};

#endif // SCRAM_TESTS_PREPROCESSOR_TESTS_H_
