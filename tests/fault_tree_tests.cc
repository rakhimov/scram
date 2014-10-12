#include <gtest/gtest.h>

#include "fault_tree.h"

#include "error.h"

using namespace scram;

TEST(FaultTreeTest, AddGate) {
  FaultTree* ft = new FaultTree("never_fail");
  GatePtr gate(new Gate("Golden"));
  EXPECT_NO_THROW(ft->AddGate(gate));
  EXPECT_THROW(ft->AddGate(gate), ValidationError);  // Trying to readd.

  GatePtr gate_two(new Gate("Iron"));
  EXPECT_THROW(ft->AddGate(gate_two), ValidationError);  // No parent issue.

  gate_two->AddParent(GatePtr(new Gate("gt")));
  EXPECT_THROW(ft->AddGate(gate_two), ValidationError);  // Doesn't belong.

  gate_two->AddParent(gate);
  EXPECT_NO_THROW(ft->AddGate(gate_two));  // Valid parent exists.
  delete ft;
}

TEST(FaultTreeTest, Validate) {
  FaultTree* ft = new FaultTree("never_fail");
  GatePtr top(new Gate("Golden"));
  EventPtr gate(new Event("Iron"));
  top->AddChild(gate);
  EXPECT_NO_THROW(ft->AddGate(top));

  // Not events are defined to be either primary or gates.
  EXPECT_THROW(ft->Validate(), ValidationError);
  delete ft;
}
