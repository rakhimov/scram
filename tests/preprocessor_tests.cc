#include "preprocessor_tests.h"

#include <vector>

using namespace scram;

// Handle constant children according to the Boolean logic of the parent.
TEST_F(PreprocessorTest, DISABLED_ProcessConstantChild) {
  std::vector<int> to_erase;
  IGatePtr gate(new IGate(kAndGate));
}
