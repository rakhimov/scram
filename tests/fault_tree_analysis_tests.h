#ifndef SCRAM_TESTS_FAULT_TREE_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_FAULT_TREE_ANALYSIS_TESTS_H_

#include <gtest/gtest.h>

#include "error.h"
#include "risk_analysis.h"
#include "fault_tree_analysis.h"

using namespace scram;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

typedef boost::shared_ptr<Superset> SupersetPtr;


class FaultTreeAnalysisTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    ran = new RiskAnalysis();
  }

  virtual void TearDown() {
    delete ran;
  }

  FaultTreeAnalysis* fta() {
    return ran->fta_;
  }

  void fta(FaultTreeAnalysis* f) {
    delete ran;
    ran = new RiskAnalysis();
    ran->fta_ = f;
  }

  std::map<std::string, std::string>& orig_ids() { return ran->orig_ids_; }

  boost::unordered_map<std::string, GatePtr>& gates() {
    return ran->gates_;
  }

  boost::unordered_map<std::string, PrimaryEventPtr>& primary_events() {
    return ran->primary_events_;
  }

  std::set< std::set<std::string> >& min_cut_sets() {
    return ran->fta_->min_cut_sets_;
  }

  double p_total() { return ran->fta_->p_total_; }

  std::map< std::set<std::string>, double >& prob_of_min_sets() {
    return ran->fta_->prob_of_min_sets_;
  }

  std::map< std::string, double >& imp_of_primaries() {
    return ran->fta_->imp_of_primaries_;
  }

  bool CheckGate(GatePtr event) {
    return (ran->CheckGate(event) == "") ? true : false;
  }

  void ExpandSets(int inter_index, std::vector< SupersetPtr >& sets) {
    return ran->fta_->ExpandSets(inter_index, sets);
  }

  // ----------- Probability calculation algorithm related part ------------
  double ProbAnd(const std::set<int>& min_cut_set) {
    return ran->fta_->ProbAnd(min_cut_set);
  }

  double ProbOr(std::set< std::set<int> >& min_cut_sets, int nsums = 1000000) {
    return ran->fta_->ProbOr(min_cut_sets, nsums);
  }

  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >& combo_set) {
    return ran->fta_->CombineElAndSet(el, set, combo_set);
  }

  void AssignIndices() {
    ran->fta_->AssignIndices(ran->fault_tree_);
  }

  int GetIndex(std::string id) {
    if (ran->fta_->prime_to_int_.count(id)) {
      return ran->fta_->prime_to_int_[id];
    } else {
      return ran->fta_->inter_to_int_[id];
    }
  }

  void AddPrimeIntProb(double prob) {
    ran->fta_->iprobs_.push_back(prob);
  }

  void nsums(int n) {
    ran->fta_->nsums_ = n;
  }
  // -----------------------------------------------------------------------
  // -------------- Monte Carlo simulation algorithms ----------------------
  void MProbOr(std::set< std::set<int> >& min_cut_sets, int sign = 1,
               int nsums = 1000000) {
    return ran->fta_->MProbOr(min_cut_sets, sign, nsums);
  }

  std::vector< std::set<int> >& pos_terms() {
    return ran->fta_->pos_terms_;
  }

  std::vector< std::set<int> >& neg_terms() {
    return ran->fta_->neg_terms_;
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
    // primary_events().insert(std::make_pair("a", A));
    // primary_events().insert(std::make_pair("b", B));
    // primary_events().insert(std::make_pair("c", C));
    // inter_events().insert(std::make_pair("d", D));
    // inter_events().insert(std::make_pair("inter", inter));
    ran->fault_tree_ = FaultTreePtr(new FaultTree("dummy"));
    ran->fault_tree_->AddGate(top_event);
    ran->fault_tree_->AddGate(inter);

    ran->fault_tree_->AddGate(D);
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

  RiskAnalysis* ran;
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
