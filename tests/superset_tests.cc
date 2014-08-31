#include <gtest/gtest.h>

#include <boost/shared_ptr.hpp>

#include "error.h"
#include "superset.h"

using namespace scram;

typedef boost::shared_ptr<Superset> SupersetPtr;

// Test InsertPrimary function
TEST(SupersetTest, InsertPrimary) {
  SupersetPtr sset(new Superset());

  int p_event = 1;  // An index of a primary event in the tree.
  int new_p_event = 10;  // A new primary event not in the tree.

  std::set<int> p_events;
  // Expect empty superset at beginning.
  EXPECT_EQ(sset->p_events(), p_events);
  // Add members.
  EXPECT_NO_THROW(sset->InsertPrimary(p_event));
  // Test if members were added successfully.
  p_events.insert(p_event);
  EXPECT_EQ(sset->p_events(), p_events);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->InsertPrimary(p_event));
  EXPECT_EQ(sset->p_events(), p_events);

  // Add the second element.
  EXPECT_NO_THROW(sset->InsertPrimary(new_p_event));
  // Test if members were added successfully.
  p_events.insert(new_p_event);
  EXPECT_EQ(sset->p_events(), p_events);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->InsertPrimary(new_p_event));
  EXPECT_EQ(sset->p_events(), p_events);

  // Negative events assiciated with NOT logic.
  int neg_p = -11;
  p_events.insert(neg_p);
  EXPECT_NO_THROW(sset->InsertPrimary(neg_p));
  EXPECT_EQ(sset->p_events(), p_events);

  // This is a use in not intended way.
  // The user should only use this function for initialization.
  int neg_existing = -1;  // This is a negation of an existing event.
  p_events.insert(neg_existing);
  EXPECT_NO_THROW(sset->InsertPrimary(neg_existing));
  EXPECT_TRUE(sset->gates().empty());
  EXPECT_EQ(sset->p_events(), p_events);
}

// Test InsertGate function.
TEST(SupersetTest, InsertGate) {
  SupersetPtr sset(new Superset());

  int gate = 100;  // An index of an gate in the tree.
  int new_gate = 200;  // A new gate not in the tree.

  std::set<int> gates;
  // Expect empty superset at beginning.
  EXPECT_EQ(sset->gates(), gates);
  // Add members.
  EXPECT_NO_THROW(sset->InsertGate(gate));
  // Test if members were added successfully.
  gates.insert(gate);
  EXPECT_EQ(sset->gates(), gates);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->InsertGate(new_gate));
  gates.insert(new_gate);
  EXPECT_EQ(sset->gates(), gates);

  // Negative events assiciated with NOT logic.
  int neg_gate = -11;
  gates.insert(neg_gate);
  EXPECT_NO_THROW(sset->InsertGate(neg_gate));
  EXPECT_EQ(sset->gates(), gates);

  // This is a use in not intended way.
  // The user should only use this function for initialization.
  int neg_existing = -100;  // This is a negation of an existing event.
  gates.insert(neg_existing);
  EXPECT_NO_THROW(sset->InsertGate(neg_existing));
  EXPECT_TRUE(sset->p_events().empty());
  EXPECT_EQ(sset->gates(), gates);
}

// Test Insert function
TEST(SupersetTest, InsertSet) {
  SupersetPtr sset_one(new Superset());
  SupersetPtr sset_two(new Superset());

  int p_event_one = 1;
  int gate_one = 10;
  int p_event_two = 5;
  int gate_two = 50;

  sset_one->InsertPrimary(p_event_one);
  sset_two->InsertPrimary(p_event_two);
  sset_one->InsertGate(gate_one);
  sset_two->InsertGate(gate_two);

  EXPECT_TRUE(sset_one->InsertSet(sset_two));

  std::set<int> p_events;
  std::set<int> gates;
  p_events.insert(p_event_one);
  p_events.insert(p_event_two);
  gates.insert(gate_one);
  gates.insert(gate_two);

  EXPECT_EQ(sset_one->p_events(), p_events);
  EXPECT_EQ(sset_one->gates(), gates);

  // Putting complement members.
  sset_one->InsertPrimary(-1 * p_event_two);
  EXPECT_FALSE(sset_one->InsertSet(sset_two));
  EXPECT_TRUE(sset_one->null());

  sset_one = SupersetPtr(new Superset());
  sset_one->InsertGate(-1 * gate_two);
  sset_one->InsertGate(-1000);
  sset_one->InsertGate(-100);
  sset_one->InsertGate(-9);
  sset_one->InsertGate(-7);
  sset_one->InsertGate(7);
  EXPECT_FALSE(sset_two->InsertSet(sset_one));
  EXPECT_TRUE(sset_two->null());
}

// Test PopGate function
TEST(SupersetTest, PopGate) {
  SupersetPtr sset(new Superset());
  // Empty gates container at the start.
  EXPECT_EQ(sset->NumOfGates(), 0);
  // Add intermediate event into the set.
  int gate = 100;
  sset->InsertGate(gate);
  EXPECT_EQ(sset->PopGate(), gate);
  // Test emptyness after poping the only inserted event.
  EXPECT_EQ(sset->NumOfGates(), 0);
}

// Test NumOfPrimaryEvents function
TEST(SupersetTest, NumOfPrimaryEvents) {
  SupersetPtr sset(new Superset());
  EXPECT_EQ(sset->NumOfPrimaryEvents(), 0);  // Empty case.
  sset->InsertPrimary(1);
  EXPECT_EQ(sset->NumOfPrimaryEvents(), 1);
  // Repeat addition to check if the size changes. It should not.
  sset->InsertPrimary(1);
  EXPECT_EQ(sset->NumOfPrimaryEvents(), 1);
  // Add a new member.
  sset->InsertPrimary(10);
  EXPECT_EQ(sset->NumOfPrimaryEvents(), 2);
}

// Test NumOfGates function
TEST(SupersetTest, NumOfGates) {
  SupersetPtr sset(new Superset());
  EXPECT_EQ(sset->NumOfGates(), 0);  // Empty case.
  sset->InsertGate(100);
  EXPECT_EQ(sset->NumOfGates(), 1);
  // Repeat addition to check if the size changes. It should not.
  sset->InsertGate(100);
  EXPECT_EQ(sset->NumOfGates(), 1);
  // Add and delete a new member.
  sset->InsertGate(500);
  EXPECT_EQ(sset->NumOfGates(), 2);
  sset->PopGate();
  EXPECT_EQ(sset->NumOfGates(), 1);
  // Empty the set.
  sset->PopGate();
  EXPECT_EQ(sset->NumOfGates(), 0);
}
