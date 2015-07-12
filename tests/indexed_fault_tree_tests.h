#ifndef SCRAM_TESTS_INDEXED_FAULT_TREE_TESTS_H_
#define SCRAM_TESTS_INDEXED_FAULT_TREE_TESTS_H_

#include "indexed_fault_tree.h"

#include <gtest/gtest.h>

using namespace scram;

/// @class IndexedFaultTreeTest
/// This test fixture is for white-box testing of algorithms for fault tree
/// preprossing.
class IndexedFaultTreeTest : public ::testing::Test {
 protected:
  typedef boost::shared_ptr<IndexedGate> IndexedGatePtr;

  virtual void SetUp() {
    fault_tree = new IndexedFaultTree(-1);  // The index is defined by tests.
  }

  virtual void TearDown() {
    delete fault_tree;
  }

  bool ProcessConstantChild(const IndexedGatePtr& gate, int child,
                            bool state, std::vector<int>* to_erase) {
    return fault_tree->ProcessConstantChild(gate, child, state, to_erase);
  }

  // Members for tests.
  IndexedFaultTree* fault_tree;
};

#endif // SCRAM_TESTS_INDEXED_FAULT_TREE_TESTS_H_
