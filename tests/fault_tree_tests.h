#ifndef SCRAM_TESTS_FAULT_TREE_TESTS_H_
#define SCRAM_TESTS_FAULT_TREE_TESTS_H_

#include <gtest/gtest.h>

#include "error.h"
#include "fault_tree.h"

using namespace scram;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::TopEvent> TopEventPtr;
typedef boost::shared_ptr<scram::InterEvent> InterEventPtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

typedef boost::shared_ptr<Superset> SupersetPtr;

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

  std::map<std::string, std::string>& orig_ids() { return fta->orig_ids_; }

  std::string top_event_id() { return fta->top_event_id_; }

  boost::unordered_map<std::string, InterEventPtr>& inter_events() {
    return fta->inter_events_;
  }

  boost::unordered_map<std::string, PrimaryEventPtr>& primary_events() {
    return fta->primary_events_;
  }

  std::set< std::set<std::string> >& min_cut_sets() {
    return fta->min_cut_sets_;
  }

  double p_total() { return fta->p_total_; }

  std::map< std::set<std::string>, double >& prob_of_min_sets() {
    return fta->prob_of_min_sets_;
  }

  std::map< std::string, double >& imp_of_primaries() {
    return fta->imp_of_primaries_;
  }

  bool CheckGate(TopEventPtr event) {
    return (fta->CheckGate_(event) == "") ? true : false;
  }

  void ExpandSets(int inter_index, std::vector< SupersetPtr >& sets) {
    return fta->ExpandSets_(inter_index, sets);
  }

  // ----------- Probability calculation algorithm related part ------------
  double ProbAnd(const std::set<int>& min_cut_set) {
    return fta->ProbAnd_(min_cut_set);
  }

  double ProbOr(std::set< std::set<int> >& min_cut_sets, int nsums = 1000000) {
    return fta->ProbOr_(min_cut_sets, nsums);
  }

  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >& combo_set) {
    return fta->CombineElAndSet_(el, set, combo_set);
  }

  void AssignIndexes() {
    fta->AssignIndexes_();
  }

  int GetIndex(std::string id) {
    if (fta->prime_to_int_.count(id)) {
      return fta->prime_to_int_[id];
    } else {
      return fta->inter_to_int_[id];
    }
  }

  void AddPrimeIntProb(double prob) {
    fta->iprobs_.push_back(prob);
  }

  void nsums(int n) {
    fta->nsums_ = n;
  }
  // -----------------------------------------------------------------------
  // -------------- Monte Carlo simulation algorithms ----------------------
  void MProbOr(std::set< std::set<int> >& min_cut_sets, int sign = 1,
               int nsums = 1000000) {
    return fta->MProbOr_(min_cut_sets, sign, nsums);
  }

  std::vector< std::set<int> >& pos_terms() {
    return fta->pos_terms_;
  }

  std::vector< std::set<int> >& neg_terms() {
    return fta->neg_terms_;
  }
  // -----------------------------------------------------------------------

  // SetUp for Gate Testing.
  void SetUpGate(std::string gate) {
    inter = InterEventPtr(new InterEvent("inter", gate));
    A = PrimaryEventPtr(new PrimaryEvent("a"));
    B = PrimaryEventPtr(new PrimaryEvent("b"));
    C = PrimaryEventPtr(new PrimaryEvent("c"));
    D = InterEventPtr(new InterEvent("d"));
    primary_events().insert(std::make_pair("a", A));
    primary_events().insert(std::make_pair("b", B));
    primary_events().insert(std::make_pair("c", C));
    inter_events().insert(std::make_pair("d", D));
    inter_events().insert(std::make_pair("inter", inter));
    AssignIndexes();
    a_id = GetIndex("a");
    b_id = GetIndex("b");
    c_id = GetIndex("c");
    inter_id = GetIndex("inter");
    d_id = GetIndex("d");
  }
  // Members

  FaultTree* fta;
  InterEventPtr inter;  // No gate is defined.
  PrimaryEventPtr A;
  PrimaryEventPtr B;
  PrimaryEventPtr C;
  InterEventPtr D;
  int a_id;
  int b_id;
  int c_id;
  int inter_id;
  int d_id;
};

#endif  // SCRAM_TESTS_FAULT_TREE_TESTS_H_
