#include "fault_tree.h"

#include <gtest/gtest.h>

#include "error.h"
#include "ccf_group.h"

using namespace scram;

typedef boost::shared_ptr<Gate> GatePtr;
typedef boost::shared_ptr<Formula> FormulaPtr;
typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
typedef boost::shared_ptr<CcfGroup> CcfGroupPtr;
typedef boost::shared_ptr<Parameter> ParameterPtr;

TEST(FaultTreeTest, AddGate) {
  FaultTree* ft = new FaultTree("never_fail");
  GatePtr gate(new Gate("Golden"));
  EXPECT_NO_THROW(ft->AddGate(gate));
  EXPECT_THROW(ft->AddGate(gate), ValidationError);  // Trying to re-add.

  GatePtr gate_two(new Gate("Iron"));
  EXPECT_NO_THROW(ft->AddGate(gate_two));  // No parent.
  delete ft;
}

TEST(FaultTreeTest, AddBasicEvent) {
  FaultTree* ft = new FaultTree("never_fail");
  BasicEventPtr event(new BasicEvent("Golden"));
  EXPECT_NO_THROW(ft->AddBasicEvent(event));
  EXPECT_THROW(ft->AddBasicEvent(event), ValidationError);  // Trying to re-add.

  BasicEventPtr event_two(new BasicEvent("Iron"));
  EXPECT_NO_THROW(ft->AddBasicEvent(event_two));  // No parent.
  delete ft;
}

TEST(FaultTreeTest, AddHouseEvent) {
  FaultTree* ft = new FaultTree("never_fail");
  HouseEventPtr event(new HouseEvent("Golden"));
  EXPECT_NO_THROW(ft->AddHouseEvent(event));
  EXPECT_THROW(ft->AddHouseEvent(event), ValidationError);  // Trying to re-add.

  HouseEventPtr event_two(new HouseEvent("Iron"));
  EXPECT_NO_THROW(ft->AddHouseEvent(event_two));  // No parent.
  delete ft;
}

TEST(FaultTreeTest, AddCcfGroup) {
  FaultTree* ft = new FaultTree("never_fail");
  CcfGroupPtr group(new BetaFactorModel("Golden"));
  EXPECT_NO_THROW(ft->AddCcfGroup(group));
  EXPECT_THROW(ft->AddCcfGroup(group), ValidationError);  // Trying to re-add.

  CcfGroupPtr group_two(new BetaFactorModel("Iron"));
  EXPECT_NO_THROW(ft->AddCcfGroup(group_two));
  delete ft;
}

TEST(FaultTreeTest, AddParameter) {
  FaultTree* ft = new FaultTree("never_fail");
  ParameterPtr parameter(new Parameter("Golden"));
  EXPECT_NO_THROW(ft->AddParameter(parameter));
  EXPECT_THROW(ft->AddParameter(parameter), ValidationError);

  ParameterPtr parameter_two(new Parameter("Iron"));
  EXPECT_NO_THROW(ft->AddParameter(parameter_two));
  delete ft;
}
