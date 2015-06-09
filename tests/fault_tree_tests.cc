#include "fault_tree.h"

#include <gtest/gtest.h>

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
  EXPECT_NO_THROW(ft->AddGate(gate_two));  // No parent.
  delete ft;
}

TEST(FaultTreeTest, MultipleTopEvents) {
  FaultTree* ft = new FaultTree("never_fail");
  GatePtr top(new Gate("Top"));
  GatePtr second_top(new Gate("SecondTop"));
  GatePtr middle(new Gate("Middle"));
  GatePtr bottom(new Gate("Bottom"));
  top->AddChild(middle);
  middle->AddChild(bottom);
  EXPECT_NO_THROW(ft->AddGate(top));
  EXPECT_NO_THROW(ft->AddGate(middle));
  EXPECT_NO_THROW(ft->AddGate(bottom));
  EXPECT_NO_THROW(ft->AddGate(second_top));
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
