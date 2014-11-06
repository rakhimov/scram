#include <gtest/gtest.h>

#include <boost/shared_ptr.hpp>

#include "error.h"
#include "event.h"

using namespace scram;

typedef boost::shared_ptr<Event> EventPtr;
typedef boost::shared_ptr<Gate> GatePtr;
typedef boost::shared_ptr<PrimaryEvent> PrimaryEventPtr;
typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
typedef boost::shared_ptr<BasicEvent> BasicEventPtr;

// Test for Event base class.
TEST(EventTest, Id) {
  EventPtr event(new Event("event_name"));
  EXPECT_EQ(event->id(), "event_name");
}

// Test Gate class
TEST(GateTest, Gate) {
  GatePtr top(new Gate("top_event"));
  // No gate has been set, so the request is an error.
  EXPECT_THROW(top->type(), LogicError);
  // Setting the gate.
  EXPECT_NO_THROW(top->type("and"));
  // Trying to set the gate again should cause an error.
  EXPECT_THROW(top->type("and"), LogicError);
  // Requesting the gate should work without errors after setting.
  ASSERT_NO_THROW(top->type());
  EXPECT_EQ(top->type(), "and");
}

TEST(GateTest, VoteNumber) {
  GatePtr top(new Gate("top_event"));
  // No gate has been set, so the request is an error.
  EXPECT_THROW(top->vote_number(), LogicError);
  // Setting the wrong AND gate.
  EXPECT_NO_THROW(top->type("and"));
  // Setting a vote number for non-Vote gate is an error.
  EXPECT_THROW(top->vote_number(2), LogicError);
  // Resetting to VOTE gate.
  top = GatePtr(new Gate("top_event"));
  EXPECT_NO_THROW(top->type("atleast"));
  // Illegal vote number.
  EXPECT_THROW(top->vote_number(-2), InvalidArgument);
  // Legal vote number.
  EXPECT_NO_THROW(top->vote_number(2));
  // Trying to reset the vote number.
  EXPECT_THROW(top->vote_number(2), LogicError);
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
  EXPECT_THROW(top->children(), LogicError);
  // Adding first child.
  EXPECT_NO_THROW(top->AddChild(first_child));
  // Readding a child must cause an error.
  EXPECT_THROW(top->AddChild(first_child), LogicError);
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
  EXPECT_THROW(inter_event->parents(), LogicError);
  // Setting a parent.
  EXPECT_NO_THROW(inter_event->AddParent(parent_event));
  EXPECT_THROW(inter_event->AddParent(parent_event), LogicError);  // Re-adding.
  EXPECT_NO_THROW(inter_event->parents());
  EXPECT_EQ(inter_event->parents().count(parent_event->id()), 1);
}

TEST(PrimaryEventTest, HouseProbability) {
  // House primary event.
  HouseEventPtr primary(new HouseEvent("valve"));
  EXPECT_FALSE(primary->state());  // Default state.
  // Setting with a valid values.
  EXPECT_NO_THROW(primary->state(true));
  EXPECT_TRUE(primary->state());
  EXPECT_NO_THROW(primary->state(false));
  EXPECT_FALSE(primary->state());
}

TEST(PrimaryEventTest, Parent) {
  PrimaryEventPtr primary(new PrimaryEvent("valve"));
  GatePtr first_parent(new Gate("trainone"));
  GatePtr second_parent(new Gate("traintwo"));
  std::map<std::string, GatePtr> parents;
  // Request for the parents when it has not been set.
  EXPECT_THROW(primary->parents(), LogicError);
  // Setting a parent. Note that there is no check if the parent is not a
  // primary event. This should be checked by a user creating this instance.
  EXPECT_NO_THROW(primary->AddParent(first_parent));
  EXPECT_THROW(primary->AddParent(first_parent), LogicError);  // Resetting.
  EXPECT_NO_THROW(primary->parents());
  parents.insert(std::make_pair(first_parent->id(), first_parent));
  EXPECT_EQ(primary->parents(), parents);
  // Adding another parent.
  EXPECT_NO_THROW(primary->AddParent(second_parent));
  EXPECT_THROW(primary->AddParent(second_parent), LogicError);  // Resetting.
  EXPECT_NO_THROW(primary->parents());
  parents.insert(std::make_pair(second_parent->id(), second_parent));
  EXPECT_EQ(primary->parents(), parents);
}
