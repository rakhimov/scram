#include <gtest/gtest.h>

#include <math.h>

#include <boost/shared_ptr.hpp>

#include "error.h"
#include "event.h"

using namespace scram;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

// Test for Event base class.
TEST(EventTest, Id) {
  EventPtr event(new Event("event_name"));
  EXPECT_EQ(event->id(), "event_name");
}

// Test Gate class
TEST(GateTest, Gate) {
  GatePtr top(new Gate("top_event"));
  // No gate has been set, so the request is an error.
  EXPECT_THROW(top->type(), ValueError);
  // Setting the gate.
  EXPECT_NO_THROW(top->type("and"));
  // Trying to set the gate again should cause an error.
  EXPECT_THROW(top->type("and"), ValueError);
  // Requesting the gate should work without errors after setting.
  ASSERT_NO_THROW(top->type());
  EXPECT_EQ(top->type(), "and");
}

TEST(GateTest, VoteNumber) {
  GatePtr top(new Gate("top_event"));
  // No gate has been set, so the request is an error.
  EXPECT_THROW(top->vote_number(), ValueError);
  // Setting the wrong AND gate.
  EXPECT_NO_THROW(top->type("and"));
  // Setting a vote number for non-Vote gate is an error.
  EXPECT_THROW(top->vote_number(2), ValueError);
  // Resetting to VOTE gate.
  top = GatePtr(new Gate("top_event"));
  EXPECT_NO_THROW(top->type("vote"));
  // Illegal vote number.
  EXPECT_THROW(top->vote_number(-2), ValueError);
  // Legal vote number.
  EXPECT_NO_THROW(top->vote_number(2));
  // Trying to reset the vote number.
  EXPECT_THROW(top->vote_number(2), ValueError);
  // Requesting the vote number should succeed.
  ASSERT_NO_THROW(top->vote_number());
  EXPECT_EQ(top->vote_number(), 2);
}

TEST(GateTest, Children) {
  GatePtr top(new Gate("top_event"));
  std::map<std::string, EventPtr> children;
  EventPtr first_child(new Event("first"));
  EventPtr second_child(new Event("second"));
  // Request for children when there are no children is an error.
  EXPECT_THROW(top->children(), ValueError);
  // Adding first child.
  EXPECT_NO_THROW(top->AddChild(first_child));
  // Readding a child must cause an error.
  EXPECT_THROW(top->AddChild(first_child), ValueError);
  // Check the contents of the children container.
  children.insert(std::make_pair(first_child->id(), first_child));
  EXPECT_EQ(top->children(), children);
  // Adding another child.
  EXPECT_NO_THROW(top->AddChild(second_child));
  children.insert(std::make_pair(second_child->id(), second_child));
  EXPECT_EQ(top->children(), children);
}

// Test Gate class
TEST(GateTest, Parent) {
  GatePtr inter_event(new Gate("inter"));
  GatePtr parent_event(new Gate("parent"));
  // Request for the parent when it has not been set.
  EXPECT_THROW(inter_event->parents(), ValueError);
  // Setting a parent. Note that there is no check if the parent is not a
  // primary event. This should be checked by a user creating this instance.
  EXPECT_NO_THROW(inter_event->AddParent(parent_event));
  EXPECT_THROW(inter_event->AddParent(parent_event), ValueError);  // Re-adding.
  EXPECT_NO_THROW(inter_event->parents());
  EXPECT_EQ(inter_event->parents().count(parent_event->id()), 1);
}

// Test PrimaryEvent class
TEST(PrimaryEventTest, Type) {
  PrimaryEventPtr primary(new PrimaryEvent("valve"));
  std::string event_type = "basic";
  // Request for a type without setting it beforehand.
  EXPECT_THROW(primary->type(), ValueError);
  // Setting and getting the type of a primary event.
  EXPECT_NO_THROW(primary->type(event_type));
  EXPECT_THROW(primary->type(event_type), ValueError);  // Resetting.
  EXPECT_NO_THROW(primary->type());
  EXPECT_EQ(primary->type(), event_type);
}

TEST(PrimaryEventTest, GeneralProbability) {
  PrimaryEventPtr primary(new PrimaryEvent("valve"));
  double prob = 0.5;
  // Request without having set.
  EXPECT_THROW(primary->p(), ValueError);
  // Setting probability with various illegal values.
  EXPECT_THROW(primary->p(-1), ValueError);
  EXPECT_THROW(primary->p(100), ValueError);
  // Setting a correct value.
  EXPECT_NO_THROW(primary->p(prob));
  EXPECT_THROW(primary->p(prob), ValueError);  // Resetting is an error.
  EXPECT_NO_THROW(primary->p());
  EXPECT_EQ(primary->p(), prob);
}

TEST(PrimaryEventTest, LambdaProbability) {
  PrimaryEventPtr primary(new PrimaryEvent("valve"));
  double prob = 0.5e-4;
  double time = 1e2;
  // Request without having set.
  EXPECT_THROW(primary->p(), ValueError);
  // Setting probability with various illegal values.
  EXPECT_THROW(primary->p(-1, 100), ValueError);
  EXPECT_THROW(primary->p(1, -100), ValueError);
  // Setting a correct value.
  EXPECT_NO_THROW(primary->p(prob, time));
  EXPECT_THROW(primary->p(prob, time), ValueError);  // Resetting is an error.
  EXPECT_NO_THROW(primary->p());
  double p_value = 1 - exp(prob * time);
  EXPECT_DOUBLE_EQ(primary->p(), p_value);
}

TEST(PrimaryEventTest, HouseProbability) {
  // House primary event.
  PrimaryEventPtr primary(new PrimaryEvent("valve", "house"));
  // Test for empty probability.
  EXPECT_THROW(primary->p(), ValueError);
  // Setting probability with various illegal values.
  EXPECT_THROW(primary->p(0.5), ValueError);
  // Setting with a valid values.
  EXPECT_NO_THROW(primary->p(0));
  EXPECT_EQ(primary->p(), 0);
  primary = PrimaryEventPtr(new PrimaryEvent("valve", "house"));
  EXPECT_NO_THROW(primary->p(1));
  EXPECT_EQ(primary->p(), 1);
  // Lambda model for a house event.
  primary = PrimaryEventPtr(new PrimaryEvent("valve", "house"));
  EXPECT_THROW(primary->p(0.5, 100), ValueError);
}

TEST(PrimaryEventTest, Parent) {
  PrimaryEventPtr primary(new PrimaryEvent("valve"));
  GatePtr first_parent(new Gate("trainone"));
  GatePtr second_parent(new Gate("traintwo"));
  std::map<std::string, GatePtr> parents;
  // Request for the parents when it has not been set.
  EXPECT_THROW(primary->parents(), ValueError);
  // Setting a parent. Note that there is no check if the parent is not a
  // primary event. This should be checked by a user creating this instance.
  EXPECT_NO_THROW(primary->AddParent(first_parent));
  EXPECT_THROW(primary->AddParent(first_parent), ValueError);  // Resetting.
  EXPECT_NO_THROW(primary->parents());
  parents.insert(std::make_pair(first_parent->id(), first_parent));
  EXPECT_EQ(primary->parents(), parents);
  // Adding another parent.
  EXPECT_NO_THROW(primary->AddParent(second_parent));
  EXPECT_THROW(primary->AddParent(second_parent), ValueError);  // Resetting.
  EXPECT_NO_THROW(primary->parents());
  parents.insert(std::make_pair(second_parent->id(), second_parent));
  EXPECT_EQ(primary->parents(), parents);
}
