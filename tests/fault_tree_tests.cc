#include "fault_tree_tests.h"

using namespace scram;

// ---------------------- Test Private Functions -------------------------
// Test the function that gets arguments from a line in an input file.
TEST_F(FaultTreeTest, GetArgs_) {
  std::string line = "";  // An empty line.
  std::string orig_line = "";
  std::vector<std::string> args;
  EXPECT_FALSE(GetArgs_(args, line, orig_line));  // Test for empty line.
  EXPECT_EQ(line, "");
  line = "# This is a comment";
  EXPECT_FALSE(GetArgs_(args, line, orig_line));  // Test for comments.
  line = "  Arg_1 Arg_2 ";  // Some valid arguments.
  EXPECT_TRUE(GetArgs_(args, line, orig_line));
  EXPECT_EQ("Arg_1 Arg_2", orig_line);
  EXPECT_EQ("arg_1 arg_2", line);
  EXPECT_EQ("arg_1", args[0]);
  EXPECT_EQ("arg_2", args[1]);
  line = "  Arg  # comments.";  // Inline comments.
  EXPECT_TRUE(GetArgs_(args, line, orig_line));
  EXPECT_EQ("Arg", orig_line);
  EXPECT_EQ("arg", line);
  EXPECT_EQ("arg", args[0]);
}

// ---------------------- Test Public Functions --------------------------
// Test Input Processing
// Note that there are tests specificly for correct and incorrect inputs
// in fault_tree_input_tests.cc, so this test only concerned with actual changes
// after processing the input.
TEST_F(FaultTreeTest, ProcessInput) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  EXPECT_EQ(7, orig_ids().size());
  EXPECT_EQ("topevent", top_event_id());
  EXPECT_EQ(2, inter_events().size());
  EXPECT_EQ(1, inter_events().count("trainone"));
  EXPECT_EQ(1, inter_events().count("traintwo"));
  EXPECT_EQ(4, primary_events().size());
  EXPECT_EQ(1, primary_events().count("pumpone"));
  EXPECT_EQ(1, primary_events().count("pumptwo"));
  EXPECT_EQ(1, primary_events().count("valveone"));
  EXPECT_EQ(1, primary_events().count("valvetwo"));
  if (inter_events().count("trainone")) {
    InterEvent* inter = inter_events()["trainone"];
    EXPECT_EQ("trainone", inter->id());
    ASSERT_NO_THROW(inter->gate());
    EXPECT_EQ("or", inter->gate());
    ASSERT_NO_THROW(inter->parent());
    EXPECT_EQ("topevent", inter->parent()->id());
  }
  if (primary_events().count("valveone")) {
    PrimaryEvent* primary = primary_events()["valveone"];
    EXPECT_EQ("valveone", primary->id());
    ASSERT_NO_THROW(primary->parents());
    EXPECT_EQ(1, primary->parents().size());
    EXPECT_EQ(1, primary->parents().count("trainone"));
    ASSERT_NO_THROW(primary->type());
    EXPECT_EQ("basic", primary->type());
    EXPECT_THROW(primary->p(), Error);
  }
}

// Test Probability Assignment
TEST_F(FaultTreeTest, PopulateProbabilities) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  ASSERT_THROW(fta->PopulateProbabilities(prob_input), Error);  // No tree.
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_EQ(4, primary_events().size());
  ASSERT_EQ(1, primary_events().count("pumpone"));
  ASSERT_EQ(1, primary_events().count("pumptwo"));
  ASSERT_EQ(1, primary_events().count("valveone"));
  ASSERT_EQ(1, primary_events().count("valvetwo"));
  ASSERT_NO_THROW(primary_events()["pumpone"]->p());
  ASSERT_NO_THROW(primary_events()["pumptwo"]->p());
  ASSERT_NO_THROW(primary_events()["valveone"]->p());
  ASSERT_NO_THROW(primary_events()["valvetwo"]->p());
  EXPECT_EQ(0.6, primary_events()["pumpone"]->p());
  EXPECT_EQ(0.7, primary_events()["pumptwo"]->p());
  EXPECT_EQ(0.4, primary_events()["valveone"]->p());
  EXPECT_EQ(0.5, primary_events()["valvetwo"]->p());
}

// Test Graphing Intructions
TEST_F(FaultTreeTest, GraphingInstructions) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  ASSERT_THROW(fta->GraphingInstructions(), Error);
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->GraphingInstructions());
}

// Test Analysis
TEST_F(FaultTreeTest, Analyze) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
}

// Test Reporting capabilities
TEST_F(FaultTreeTest, Report) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
}
