#include <gtest/gtest.h>

#include <boost/shared_ptr.hpp>

#include "error.h"
#include "superset.h"

using namespace scram;

typedef boost::shared_ptr<Superset> SupersetPtr;

// Test InsertPrimary function
TEST(SupersetTest, InsertPrimary) {
  SupersetPtr sset(new Superset());

  int prime_event = 1;  // An index of a prime event in the tree.
  int new_prime_event = 10;  // A new prime event not in the tree.

  std::set<int> primes;
  // Expect empty superset at beginning.
  EXPECT_EQ(sset->primes(), primes);
  // Add members.
  EXPECT_NO_THROW(sset->InsertPrimary(prime_event));
  // Test if members were added successfully.
  primes.insert(prime_event);
  EXPECT_EQ(sset->primes(), primes);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->InsertPrimary(prime_event));
  EXPECT_EQ(sset->primes(), primes);

  // Add the second element.
  EXPECT_NO_THROW(sset->InsertPrimary(new_prime_event));
  // Test if members were added successfully.
  primes.insert(new_prime_event);
  EXPECT_EQ(sset->primes(), primes);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->InsertPrimary(new_prime_event));
  EXPECT_EQ(sset->primes(), primes);

  // Negative events assiciated with NOT logic.
  int neg_prime = -11;
  primes.insert(neg_prime);
  EXPECT_NO_THROW(sset->InsertPrimary(neg_prime));
  EXPECT_EQ(sset->primes(), primes);

  // This is a use in not intended way.
  // The user should only use this function for initialization.
  int neg_existing = -1;  // This is a negation of an existing event.
  primes.insert(neg_existing);
  EXPECT_NO_THROW(sset->InsertPrimary(neg_existing));
  EXPECT_TRUE(sset->gates().empty());
  EXPECT_EQ(sset->primes(), primes);
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
  EXPECT_TRUE(sset->primes().empty());
  EXPECT_EQ(sset->gates(), gates);
}

// Test Insert function
TEST(SupersetTest, Insert) {
  SupersetPtr sset_one(new Superset());
  SupersetPtr sset_two(new Superset());

  int prime_event_one = 1;
  int gate_one = 10;
  int prime_event_two = 5;
  int gate_two = 50;

  sset_one->InsertPrimary(prime_event_one);
  sset_two->InsertPrimary(prime_event_two);
  sset_one->InsertGate(gate_one);
  sset_two->InsertGate(gate_two);

  EXPECT_TRUE(sset_one->InsertSet(sset_two));

  std::set<int> primes;
  std::set<int> gates;
  primes.insert(prime_event_one);
  primes.insert(prime_event_two);
  gates.insert(gate_one);
  gates.insert(gate_two);

  EXPECT_EQ(sset_one->primes(), primes);
  EXPECT_EQ(sset_one->gates(), gates);
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

// Test NumOfPrimeEvents function
TEST(SupersetTest, NumOfPrimeEvents) {
  SupersetPtr sset(new Superset());
  EXPECT_EQ(sset->NumOfPrimeEvents(), 0);  // Empty case.
  sset->InsertPrimary(1);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 1);
  // Repeat addition to check if the size changes. It should not.
  sset->InsertPrimary(1);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 1);
  // Add a new member.
  sset->InsertPrimary(10);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 2);
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
