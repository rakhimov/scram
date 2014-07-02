#ifndef SCRAM_TESTS_FAULT_TREE_TESTS_H_
#define SCRAM_TESTS_FAULT_TREE_TESTS_H_

#include <gtest/gtest.h>

#include "error.h"
#include "fault_tree.h"

using namespace scram;

class FaultTreeTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    fta = new FaultTree("fta-default", false);
  }

  virtual void TearDown() {
    delete fta;
  }

  bool GetArgs_(std::vector<std::string>& args, std::string& line,
                std::string& orig_line) {
    return fta->GetArgs_(args, line, orig_line);
  }

  std::map<std::string, std::string> orig_ids() { return fta->orig_ids_; }
  std::string top_event_id() { return fta->top_event_id_; }
  boost::unordered_map<std::string, scram::InterEvent*> inter_events() {
    return fta->inter_events_;
  }
  boost::unordered_map<std::string, scram::PrimaryEvent*> primary_events() {
    return fta->primary_events_;
  }
  std::set< std::set<std::string> > min_cut_sets() {
    return fta->min_cut_sets_;
  }
  double p_total() { return fta->p_total_; }
  std::map< std::set<std::string>, double > prob_of_min_sets() {
    return fta->prob_of_min_sets_;
  }
  std::map< std::string, double > imp_of_primaries() {
    return fta->imp_of_primaries_;
  }

  FaultTree* fta;
};

#endif  // SCRAM_TESTS_FAULT_TREE_TESTS_H_
