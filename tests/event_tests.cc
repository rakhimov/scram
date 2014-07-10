#include <gtest/gtest.h>

#include <boost/shared_ptr.hpp>

#include "error.h"
#include "event.h"

using namespace scram;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::TopEvent> TopEventPtr;
typedef boost::shared_ptr<scram::InterEvent> InterEventPtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

// Test for Event base class.
TEST(EventTest, Id) {
  EventPtr event(new Event("event_name"));
  EXPECT_EQ(event->id(), "event_name");
}

// Test TopEvent class
TEST(TopEventTest, Gate) {
  TopEventPtr top(new TopEvent("top_event"));
  // No gate has been set, so the request is an error.
  EXPECT_THROW(top->gate(), ValueError);
  // Setting the gate.
  EXPECT_NO_THROW(top->gate("and"));
  // Trying to set the gate again should cause an error.
  EXPECT_THROW(top->gate("and"), ValueError);
  // Requesting the gate should work without errors after setting.
  EXPECT_NO_THROW(top->gate());
  EXPECT_EQ(top->gate(), "and");
}

TEST(TopEventTest, Children) {
  TopEventPtr top(new TopEvent("top_event"));
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

// Test InterEvent class
TEST(InterEventTest, Parent) {
  InterEventPtr inter_event(new InterEvent("inter"));
  TopEventPtr parent_event(new TopEvent("parent"));
  // Request for the parent when it has not been set.
  EXPECT_THROW(inter_event->parent(), ValueError);
  // Setting a parent. Note that there is no check if the parent is not a
  // primary event. This should be checked by a user creating this instance.
  EXPECT_NO_THROW(inter_event->parent(parent_event));
  EXPECT_THROW(inter_event->parent(parent_event), ValueError);  // Resetting.
  EXPECT_NO_THROW(inter_event->parent());
  EXPECT_EQ(inter_event->parent(), parent_event);
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
}

TEST(PrimaryEventTest, Parent) {
  PrimaryEventPtr primary(new PrimaryEvent("valve"));
  InterEventPtr first_parent(new InterEvent("trainone"));
  InterEventPtr second_parent(new InterEvent("traintwo"));
  std::map<std::string, TopEventPtr> parents;
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
