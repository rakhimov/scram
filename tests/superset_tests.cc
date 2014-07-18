#include <gtest/gtest.h>

#include <boost/shared_ptr.hpp>

#include "error.h"
#include "superset.h"

using namespace scram;

typedef boost::shared_ptr<Superset> SupersetPtr;

// Test AddPrime function
TEST(SupersetTest, AddPrime) {
  SupersetPtr sset(new Superset());

  int prime_event = 1;  // An index of a prime event in the tree.
  int new_prime_event = 10;  // A new prime event not in the tree.

  std::set<int> primes;
  // Expect empty superset at beginning.
  EXPECT_EQ(sset->primes(), primes);
  // Add members.
  EXPECT_NO_THROW(sset->AddPrimary(prime_event));
  // Test if members were added successfully.
  primes.insert(prime_event);
  EXPECT_EQ(sset->primes(), primes);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->AddPrimary(prime_event));
  EXPECT_EQ(sset->primes(), primes);

  // Add the second element.
  EXPECT_NO_THROW(sset->AddPrimary(new_prime_event));
  // Test if members were added successfully.
  primes.insert(new_prime_event);
  EXPECT_EQ(sset->primes(), primes);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->AddPrimary(new_prime_event));
  EXPECT_EQ(sset->primes(), primes);
}

// Test AddInter function.
TEST(SupersetTest, AddInter) {
  SupersetPtr sset(new Superset());

  int inter_event = 100;  // An index of an inter event in the tree.
  int new_inter_event = 200;  // A new inter event not in the tree.

  std::set<int> inters;
  // Expect empty superset at beginning.
  EXPECT_EQ(sset->inters(), inters);
  // Add members.
  EXPECT_NO_THROW(sset->AddInter(inter_event));
  // Test if members were added successfully.
  inters.insert(inter_event);
  EXPECT_EQ(sset->inters(), inters);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->AddInter(new_inter_event));
  inters.insert(new_inter_event);
  EXPECT_EQ(sset->inters(), inters);
}

// Test Insert function
TEST(SupersetTest, Insert) {
  SupersetPtr sset_one(new Superset());
  SupersetPtr sset_two(new Superset());

  int prime_event_one = 1;
  int inter_event_one = 10;
  int prime_event_two = 5;
  int inter_event_two = 50;

  sset_one->AddPrimary(prime_event_one);
  sset_two->AddPrimary(prime_event_two);
  sset_one->AddInter(inter_event_one);
  sset_two->AddInter(inter_event_two);

  sset_one->Insert(sset_two);

  std::set<int> primes;
  std::set<int> inters;
  primes.insert(prime_event_one);
  primes.insert(prime_event_two);
  inters.insert(inter_event_one);
  inters.insert(inter_event_two);

  EXPECT_EQ(sset_one->primes(), primes);
  EXPECT_EQ(sset_one->inters(), inters);
}

// Test PopInter function
TEST(SupersetTest, PopInter) {
  SupersetPtr sset(new Superset());
  // Fail to pop when the set is empty.
  EXPECT_THROW(sset->PopInter(), ValueError);
  // Add intermediate event into the set.
  int inter_event = 100;
  sset->AddInter(inter_event);
  EXPECT_EQ(sset->PopInter(), inter_event);
  // Test emptyness after poping the only inserted event.
  EXPECT_THROW(sset->PopInter(), ValueError);
}

// Test NumOfPrimeEvents function
TEST(SupersetTest, NumOfPrimeEvents) {
  SupersetPtr sset(new Superset());
  EXPECT_EQ(sset->NumOfPrimeEvents(), 0);  // Empty case.
  sset->AddPrimary(1);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 1);
  // Repeat addition to check if the size changes. It should not.
  sset->AddPrimary(1);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 1);
  // Add a new member.
  sset->AddPrimary(10);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 2);
}

// Test NumOfInterEvents function
TEST(SupersetTest, NumOfInterEvents) {
  SupersetPtr sset(new Superset());
  EXPECT_EQ(sset->NumOfInterEvents(), 0);  // Empty case.
  sset->AddInter(100);
  EXPECT_EQ(sset->NumOfInterEvents(), 1);
  // Repeat addition to check if the size changes. It should not.
  sset->AddInter(100);
  EXPECT_EQ(sset->NumOfInterEvents(), 1);
  // Add and delete a new member.
  sset->AddInter(500);
  EXPECT_EQ(sset->NumOfInterEvents(), 2);
  sset->PopInter();
  EXPECT_EQ(sset->NumOfInterEvents(), 1);
  // Empty the set.
  sset->PopInter();
  EXPECT_EQ(sset->NumOfInterEvents(), 0);
}

// Test corner case with deleting the superset instance.
TEST(SupersetTest, DeleteSet) {
  SupersetPtr sset(new Superset());
  int prime_event = 1;
  int inter_event = 100;
  sset->AddPrimary(prime_event);
  sset->AddInter(inter_event);
  std::set<int> primes;
  std::set<int> inters;
  primes = sset->primes();
  inters = sset->inters();
  primes.insert(1000);
  EXPECT_EQ(sset->primes().size(), 1);
}
