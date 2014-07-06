#include <gtest/gtest.h>

#include "error.h"
#include "risk_analysis.h"
#include "superset.h"

using namespace scram;

// Test AddMember function
TEST(SupersetTest, AddMember) {
  // Initialize the fault tree from an input file.
  FaultTree* ftree = new FaultTree("fta-default", false);
  ftree->ProcessInput("./input/fta/correct_tree_input.scramf");

  Superset* sset = new Superset();

  std::string prime_event = "valveone";  // Prime event in the tree.
  std::string inter_event = "trainone";  // Inter event in the tree.

  std::string new_prime_event = "valvethree";  // Prime event not in the tree.
  std::string new_inter_event = "trainthree";  // Inter event not in the tree.

  std::set<std::string> inters;
  std::set<std::string> primes;
  // Expect empty superset at beginning.
  EXPECT_EQ(sset->primes(), primes);
  EXPECT_EQ(sset->inters(), inters);
  // Add members.
  EXPECT_NO_THROW(sset->AddMember(inter_event, ftree));
  EXPECT_NO_THROW(sset->AddMember(prime_event, ftree));
  EXPECT_THROW(sset->AddMember(new_inter_event, ftree), ValueError);
  EXPECT_THROW(sset->AddMember(new_prime_event, ftree), ValueError);
  // Test if members were added successfully.
  primes.insert(prime_event);
  inters.insert(inter_event);
  EXPECT_EQ(sset->primes(), primes);
  EXPECT_EQ(sset->inters(), inters);
  // Test if repeated addition handled in a set.
  EXPECT_NO_THROW(sset->AddMember(inter_event, ftree));
  EXPECT_NO_THROW(sset->AddMember(prime_event, ftree));
  EXPECT_EQ(sset->primes(), primes);
  EXPECT_EQ(sset->inters(), inters);

  delete ftree, sset;
}

// Test Insert function
TEST(SupersetTest, Insert) {
  // Initialize the fault tree from an input file.
  FaultTree* ftree = new FaultTree("fta-default", false);
  ftree->ProcessInput("./input/fta/correct_tree_input.scramf");

  Superset* sset_one = new Superset();
  Superset* sset_two = new Superset();

  std::string prime_event_one = "valveone";
  std::string inter_event_one = "trainone";
  std::string prime_event_two = "valvetwo";
  std::string inter_event_two = "traintwo";

  sset_one->AddMember(prime_event_one, ftree);
  sset_one->AddMember(inter_event_one, ftree);
  sset_two->AddMember(prime_event_two, ftree);
  sset_two->AddMember(inter_event_two, ftree);

  sset_one->Insert(sset_two);

  std::set<std::string> primes;
  std::set<std::string> inters;
  primes.insert(prime_event_one);
  primes.insert(prime_event_two);
  inters.insert(inter_event_one);
  inters.insert(inter_event_two);

  EXPECT_EQ(sset_one->primes(), primes);
  EXPECT_EQ(sset_one->inters(), inters);

  delete ftree, sset_one, sset_two;
}

// Test PopInter function
TEST(SupersetTest, PopInter) {
  // Initialize the fault tree from an input file.
  FaultTree* ftree = new FaultTree("fta-default", false);
  ftree->ProcessInput("./input/fta/correct_tree_input.scramf");

  Superset* sset = new Superset();
  // Fail to pop when the set is empty.
  EXPECT_THROW(sset->PopInter(), ValueError);
  // Add intermediate event into the set.
  std::string inter_event = "trainone";
  sset->AddMember(inter_event, ftree);
  EXPECT_EQ(sset->PopInter(), inter_event);
  // Test emptyness after poping the only inserted event.
  EXPECT_THROW(sset->PopInter(), ValueError);

  delete ftree, sset;
}

// Test NumOfPrimeEvents function
TEST(SupersetTest, NumOfPrimeEvents) {
  // Initialize the fault tree from an input file.
  FaultTree* ftree = new FaultTree("fta-default", false);
  ftree->ProcessInput("./input/fta/correct_tree_input.scramf");
  Superset* sset = new Superset();
  EXPECT_EQ(sset->NumOfPrimeEvents(), 0);  // Empty case.
  sset->AddMember("pumpone", ftree);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 1);
  // Repeat addition to check if the size changes. It should not.
  sset->AddMember("pumpone", ftree);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 1);
  // Add a new member.
  sset->AddMember("pumptwo", ftree);
  EXPECT_EQ(sset->NumOfPrimeEvents(), 2);

  delete ftree, sset;
}

// Test NumOfInterEvents function
TEST(SupersetTest, NumOfInterEvents) {
  // Initialize the fault tree from an input file.
  FaultTree* ftree = new FaultTree("fta-default", false);
  ftree->ProcessInput("./input/fta/correct_tree_input.scramf");
  Superset* sset = new Superset();
  EXPECT_EQ(sset->NumOfInterEvents(), 0);  // Empty case.
  sset->AddMember("trainone", ftree);
  EXPECT_EQ(sset->NumOfInterEvents(), 1);
  // Repeat addition to check if the size changes. It should not.
  sset->AddMember("trainone", ftree);
  EXPECT_EQ(sset->NumOfInterEvents(), 1);
  // Add and delete a new member.
  sset->AddMember("traintwo", ftree);
  EXPECT_EQ(sset->NumOfInterEvents(), 2);
  sset->PopInter();
  EXPECT_EQ(sset->NumOfInterEvents(), 1);
  // Empty the set.
  sset->PopInter();
  EXPECT_EQ(sset->NumOfInterEvents(), 0);

  delete ftree, sset;
}

// Test corner case with deleting the superset instance.
TEST(SupersetTest, DeleteSet) {
  // Initialize the fault tree from an input file.
  FaultTree* ftree = new FaultTree("fta-default", false);
  ftree->ProcessInput("./input/fta/correct_tree_input.scramf");
  Superset* sset = new Superset();
  std::string prime_event = "valveone";
  std::string inter_event = "trainone";
  sset->AddMember(prime_event, ftree);
  sset->AddMember(inter_event, ftree);
  std::set<std::string> primes;
  std::set<std::string> inters;
  primes = sset->primes();
  inters = sset->inters();
  primes.insert("invalid");
  EXPECT_EQ(sset->primes().size(), 1);
  delete sset;  // This is a wild test that should be deleted in future.
  EXPECT_EQ(primes.size(), 2);
  EXPECT_EQ(inters.size(), 1);

  delete ftree, sset;
}
