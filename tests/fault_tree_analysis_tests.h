#ifndef SCRAM_TESTS_FAULT_TREE_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_FAULT_TREE_ANALYSIS_TESTS_H_

#include <gtest/gtest.h>

#include "error.h"
#include "fault_tree_analysis.h"

using namespace scram;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

typedef boost::shared_ptr<Superset> SupersetPtr;

class FaultTreeAnalysisTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    fta = new FaultTreeAnalysis("default");
  }

  virtual void TearDown() {
    delete fta;
  }

  void new_fta(FaultTreeAnalysis* f) {
    delete fta;
    fta = f;
  }

  void ExpandSets(int inter_index, std::vector< SupersetPtr >& sets) {
    return fta->ExpandSets(inter_index, sets);
  }

  // ----------- Probability calculation algorithm related part ------------
  double ProbAnd(const std::set<int>& min_cut_set) {
    return fta->ProbAnd(min_cut_set);
  }

  double ProbOr(std::set< std::set<int> >& min_cut_sets, int nsums = 1000000) {
    return fta->ProbOr(min_cut_sets, nsums);
  }

  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >& combo_set) {
    return fta->CombineElAndSet(el, set, combo_set);
  }

  void AssignIndices() {
    fta->AssignIndices(ft);
  }

  int GetIndex(std::string id) {
    if (fta->prime_to_int_.count(id)) {
      return fta->prime_to_int_.find(id)->second;
    } else {
      return fta->inter_to_int_.find(id)->second;
    }
  }

  void AddPrimeIntProb(double prob) {
    fta->iprobs_.push_back(prob);
  }
  // -----------------------------------------------------------------------
  // -------------- Monte Carlo simulation algorithms ----------------------
  void MProbOr(std::set< std::set<int> >& min_cut_sets, int sign = 1,
               int nsums = 1000000) {
    return fta->MProbOr(min_cut_sets, sign, nsums);
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
    inter = GatePtr(new Gate("inter", gate));
    A = PrimaryEventPtr(new PrimaryEvent("a"));
    B = PrimaryEventPtr(new PrimaryEvent("b"));
    C = PrimaryEventPtr(new PrimaryEvent("c"));
    D = GatePtr(new Gate("d", "or"));
    GatePtr top_event(new Gate("TopEvent", "null"));
    top_event->AddChild(inter);
    ft = FaultTreePtr(new FaultTree("dummy"));
    ft->AddGate(top_event);
    ft->AddGate(inter);
    ft->AddGate(D);

    D->AddChild(A);
    D->AddChild(B);
    D->AddChild(C);
  }

  void GetIndices() {
    AssignIndices();
    a_id = GetIndex("a");
    b_id = GetIndex("b");
    c_id = GetIndex("c");
    inter_id = GetIndex("inter");
    d_id = GetIndex("d");
  }

  // Members

  FaultTreeAnalysis* fta;
  FaultTreePtr ft;
  GatePtr inter;  // No gate is defined.
  PrimaryEventPtr A;
  PrimaryEventPtr B;
  PrimaryEventPtr C;
  GatePtr D;
  int a_id;
  int b_id;
  int c_id;
  int inter_id;
  int d_id;
};

#endif  // SCRAM_TESTS_FAULT_TREE_ANALYSIS_TESTS_H_
