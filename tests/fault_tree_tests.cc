#include "fault_tree.h"

#include <gtest/gtest.h>

#include "error.h"

using namespace scram;

typedef boost::shared_ptr<Gate> GatePtr;
typedef boost::shared_ptr<Formula> FormulaPtr;

TEST(FaultTreeTest, AddGate) {
  FaultTree* ft = new FaultTree("never_fail");
  GatePtr gate(new Gate("Golden"));
  EXPECT_NO_THROW(ft->AddGate(gate));
  EXPECT_THROW(ft->AddGate(gate), ValidationError);  // Trying to re-add.

  GatePtr gate_two(new Gate("Iron"));
  EXPECT_NO_THROW(ft->AddGate(gate_two));  // No parent.
  delete ft;
}
