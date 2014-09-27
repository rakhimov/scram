#ifndef SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_

#include <map>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include "error.h"
#include "risk_analysis.h"

using namespace scram;

typedef boost::shared_ptr<scram::Event> EventPtr;
typedef boost::shared_ptr<scram::Gate> GatePtr;
typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

class RiskAnalysisTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    ran = new RiskAnalysis();
  }

  virtual void TearDown() {
    delete ran;
  }

  void fta(FaultTreeAnalysis* f) {
    delete ran;
    ran = new RiskAnalysis();
    ran->fta_ = f;
  }

  boost::unordered_map<std::string, GatePtr>& gates() {
    return ran->gates_;
  }

  boost::unordered_map<std::string, PrimaryEventPtr>& primary_events() {
    return ran->primary_events_;
  }

  const std::set< std::set<std::string> >& min_cut_sets() {
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->min_cut_sets();
  }

  double p_total() {
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->p_total();
  }

  const std::map< std::set<std::string>, double >& prob_of_min_sets() {
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->prob_of_min_sets();
  }

  const std::map< std::string, double >& imp_of_primaries() {
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->imp_of_primaries();
  }

  bool CheckGate(GatePtr event) {
    return (ran->CheckGate(event) == "") ? true : false;
  }

  // Members
  RiskAnalysis* ran;
};

#endif  // SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
