#include "preprocessor_tests.h"

#include <vector>

using namespace scram;

// Handle constant children according to the Boolean logic of the parent.
TEST_F(PreprocessorTest, DISABLED_ProcessConstantChild) {
  std::vector<int> to_erase;
  IGatePtr gate(new IGate(kAndGate));
  gate->AddChild(2);
  gate->AddChild(3);
  ProcessConstantChild(gate, 2, false, &to_erase);
}
