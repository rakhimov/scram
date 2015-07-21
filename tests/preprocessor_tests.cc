#include "preprocessor_tests.h"

#include <vector>

using namespace scram;

// Handle constant children according to the Boolean logic of the parent.
TEST_F(PreprocessorTest, ProcessConstantChild) {
  std::vector<int> to_erase;
  IndexedGatePtr gate(new IndexedGate(1, kAndGate));
  gate->AddChild(2);
  gate->AddChild(3);
  ProcessConstantChild(gate, 2, false, &to_erase);
}
