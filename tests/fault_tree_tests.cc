#include <gtest/gtest.h>

#include "fault_tree.h"

#include "error.h"

using namespace scram;

typedef boost::shared_ptr<Gate> GatePtr;
typedef boost::shared_ptr<Event> EventPtr;

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

TEST(FaultTreeTest, CyclicTree) {
  FaultTree* ft = new FaultTree("never_fail");
  GatePtr top(new Gate("Top"));
  GatePtr middle(new Gate("Middle"));
  GatePtr bottom(new Gate("Bottom"));
  top->AddChild(middle);
  middle->AddChild(bottom);
  bottom->AddChild(top);  // Looping here.
  EXPECT_NO_THROW(ft->AddGate(top));
  EXPECT_THROW(ft->Validate(), ValidationError);
  delete ft;
}

TEST(FaultTreeTest, SetupForAnalysis) {
  FaultTree* ft = new FaultTree("never_fail");
  GatePtr top(new Gate("Golden"));
  EventPtr gate(new Event("Iron"));  // This is not a gate but a generic event.
  top->AddChild(gate);
  EXPECT_NO_THROW(ft->AddGate(top));
  EXPECT_NO_THROW(ft->Validate());

  // Undefined event. Nodes must be gates or primary events.
  EXPECT_THROW(ft->SetupForAnalysis(), LogicError);
  delete ft;
}
