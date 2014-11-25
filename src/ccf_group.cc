/// @file ccf_group.cc
/// Implementation of various common cause failure models.
#include "ccf_group.h"

namespace scram {

CcfGroup::CcfGroup(std::string name, std::string model)
    : name_(name),
      model_(model) {}

void CcfGroup::AddMember(const BasicEventPtr& basic_event) {
  if (distribution_) {
    throw IllegalOperation("No more members accepted. The distribution for " +
                           name_ + " CCF group has already been defined.");

  } else if (members_.count(basic_event->id())) {
    throw LogicError("Basic event " + basic_event->orig_id() + " is already" +
                     " in " + name_ + " CCF group.");
  }
  members_.insert(std::make_pair(basic_event->id(), basic_event));
}

void CcfGroup::AddDistribution(const ExpressionPtr& distr) {
  assert(!distribution_);
  distribution_ = distr;
  // Define probabilities of all basic events.
  std::map<std::string, BasicEventPtr>::iterator it;
  for (it = members_.begin(); it != members_.end(); ++it) {
    it->second->expression(distribution_);
  }
}

void CcfGroup::AddFactor(const ExpressionPtr& factor, int level) {
  assert(level > 0);
  if (factors_.empty() && model_ == "phi-factor") assert(level == 1);
  if (factors_.empty() && model_ == "alpha-factor") assert(level == 1);
  if (factors_.empty() && model_ == "beta-factor") assert(level == 2);
  if (factors_.empty() && model_ == "MGL") assert(level == 2);
  if (!factors_.empty()) assert(level == factors_.back().first + 1);
  factors_.push_back(std::make_pair(level, factor));
}

void CcfGroup::Validate() {
  if (members_.size() < 2) {
    throw ValidationError(name_ + " CCF group must have at least 2 members.");
  }

  if (distribution_->Min() < 0 || distribution_->Max() > 1) {
    throw ValidationError("Distribution for " + name_ + " CCF group" +
                          " has illegal values.");
  }
  if (factors_.back().first > members_.size()) {
    throw ValidationError("The level of factors for " + name_ + " CCF group" +
                          " cannot be more than # of members.");
  }
  std::vector<std::pair<int, ExpressionPtr> >::iterator it;
  for (it = factors_.begin(); it != factors_.end(); ++it) {
    if (it->second->Max() > 1 || it->second->Min() < 0) {
      throw ValidationError("Factors for " + name_ + " CCF group" +
                            " have illegal values.");
    }
  }
}

void CcfGroup::ConstructCcfBasicEvents(
    int max_level,
    std::map<BasicEventPtr, std::set<std::string> >* new_events) {
  assert(max_level > 1);
  assert(members_.size() > 1);
  assert(max_level <= members_.size());
  assert(new_events->empty());

  std::set<std::set<std::string> > combinations;
  std::set<std::string> comb;
  combinations.insert(comb);  // Empty set is needed for iteration.

  for (int i = 0; i < max_level; ++i) {
    std::set<std::set<std::string> > next_level;
    std::set<std::set<std::string> >::iterator it;
    for (it = combinations.begin(); it != combinations.end(); ++it) {
      std::map<std::string, BasicEventPtr>::const_iterator it_m;
      for (it_m = members_.begin(); it_m != members_.end(); ++it_m) {
        std::set<std::string> comb(*it);
        comb.insert(it_m->first);
        if (comb.size() > i) {
          next_level.insert(comb);
        }
      }
    }
    for (it = next_level.begin(); it != next_level.end(); ++it) {
      std::string id = "[";
      std::string orig_id = "[";
      std::set<std::string>::const_iterator it_s;
      for (it_s = it->begin(); it_s != it->end();) {
        id += *it_s;
        orig_id += members_.find(*it_s)->second->orig_id();
        ++it_s;
        if (it_s != it->end()) {
          id += " ";
          orig_id += " ";
        }
      }
      id += "]";
      orig_id += "]";
      BasicEventPtr new_basic_event(new BasicEvent(id));
      new_basic_event->orig_id(orig_id);
      new_events->insert(std::make_pair(new_basic_event, *it));
      new_events_.push_back(new_basic_event);
    }
    combinations = next_level;
  }
}

void BetaFactorModel::Validate() {
  if (factors_.size() != 1) {
    throw ValidationError("Beta-Factor Model " + this->name() + " CCF group" +
                          " must have exactly one factor.");
  }
  CcfGroup::Validate();
}

void BetaFactorModel::ApplyModel() {
  std::string common_name = "[";  // Event name for common failure group.
  std::string common_id = "[";  // Event id for common failure group.

  std::vector<ExpressionPtr> args;  // For expression arguments.

  // Getting the probability equation for independent events.
  assert(CcfGroup::factors_.size() == 1);
  ExpressionPtr one(new ConstantExpression(1.0));
  args.push_back(one);
  ExpressionPtr beta = CcfGroup::factors_.begin()->second;
  args.push_back(beta);
  ExpressionPtr indep_factor(new Sub(args));  // (1 - beta)
  args.clear();
  args.push_back(indep_factor);
  args.push_back(CcfGroup::distribution_);
  ExpressionPtr indep_prob(new Mul(args));  // (1 - beta) * Q

  std::map<std::string, BasicEventPtr>::const_iterator it;
  for (it = CcfGroup::members_.begin(); it != CcfGroup::members_.end();) {
    // Create indipendent events.
    std::string independent_orig_id = "[" + it->second->orig_id() + "]";
    std::string independent_id = "[" + it->second->id() + "]";

    BasicEventPtr independent(new BasicEvent(independent_id));
    independent->orig_id(independent_orig_id);
    independent->expression(indep_prob);
    CcfGroup::new_events_.push_back(independent);

    GatePtr replacement(new Gate(it->first, "or"));
    CcfGroup::gates_.insert(std::make_pair(it->first, replacement));
    it->second->ccf_gate(replacement);
    replacement->AddChild(independent);

    common_name += it->second->orig_id();
    common_id += it->second->id();
    ++it;
    if (it != CcfGroup::members_.end()) {
      common_name += " ";
      common_id += " ";
    }
  }
  common_id += "]";
  common_name += "]";
  BasicEventPtr common_failure(new BasicEvent(common_id));
  common_failure->orig_id(common_name);
  args.clear();
  args.push_back(beta);
  args.push_back(CcfGroup::distribution_);
  common_failure->expression(ExpressionPtr(new Mul(args)));
  CcfGroup::new_events_.push_back(common_failure);

  std::map<std::string, GatePtr>::iterator it_g;
  for (it_g = CcfGroup::gates_.begin(); it_g != CcfGroup::gates_.end();
       ++it_g) {
    it_g->second->AddChild(common_failure);
  }
}

void MglModel::ApplyModel() {
  assert(gates_.empty());
  std::map<std::string, BasicEventPtr>::const_iterator it_m;
  for (it_m = members_.begin(); it_m != members_.end(); ++it_m) {
    GatePtr new_gate(new Gate(it_m->first, "or"));
    gates_.insert(std::make_pair(new_gate->id(), new_gate));
    it_m->second->ccf_gate(new_gate);
  }

  int max_level = factors_.back().first;  // Assumes that factors are
                                          // sequential.
  assert(factors_.size() == max_level - 1);

  std::vector<ExpressionPtr> probabilities;  // The level is position + 1.
  for (int i = 0; i < max_level; ++i) {
    int num_members = members_.size();
    ExpressionPtr k(
        new ConstantExpression(CcfGroup::Factorial(num_members - i) *
                               CcfGroup::Factorial(i) /
                               CcfGroup::Factorial(num_members - 1)));
    std::vector<ExpressionPtr> args;
    args.push_back(k);
    for (int j = 0; j < i; ++j) {
      args.push_back(factors_[j].second);
    }
    if (i < max_level - 1) {
      ExpressionPtr one(new ConstantExpression(1.0));
      std::vector<ExpressionPtr> sub_args;
      sub_args.push_back(one);
      sub_args.push_back(factors_[i].second);
      ExpressionPtr last_factor(new Sub(sub_args));
      args.push_back(last_factor);
    }
    args.push_back(distribution_);
    ExpressionPtr prob(new Mul(args));
    probabilities.push_back(prob);
  }
  assert(probabilities.size() == max_level);

  // Mapping of new basic events and their parents.
  std::map<BasicEventPtr, std::set<std::string> > new_events;
  CcfGroup::ConstructCcfBasicEvents(max_level, &new_events);
  assert(!new_events.empty());
  std::map<BasicEventPtr, std::set<std::string> >::iterator it;
  for (it = new_events.begin(); it != new_events.end(); ++it) {
    it->first->expression(probabilities[it->second.size() - 1]);
    // Add this basic event to the parent gates.
    std::set<std::string>::iterator it_l;
    for (it_l = it->second.begin(); it_l != it->second.end(); ++it_l) {
      gates_.find(*it_l)->second->AddChild(it->first);
    }
  }
}

void AlphaFactorModel::ApplyModel() {
  assert(gates_.empty());
  std::map<std::string, BasicEventPtr>::const_iterator it_m;
  for (it_m = members_.begin(); it_m != members_.end(); ++it_m) {
    GatePtr new_gate(new Gate(it_m->first, "or"));
    gates_.insert(std::make_pair(new_gate->id(), new_gate));
    it_m->second->ccf_gate(new_gate);
  }

  int max_level = factors_.back().first;  // Assumes that factors are
                                          // sequential.
  assert(factors_.size() == max_level);
  std::vector<ExpressionPtr> sum_args;
  std::vector<std::pair<int, ExpressionPtr> >::iterator it_f;
  for (it_f = factors_.begin(); it_f != factors_.end(); ++it_f) {
    sum_args.push_back(it_f->second);
  }
  ExpressionPtr sum(new Add(sum_args));

  std::vector<ExpressionPtr> probabilities;  // The level is position + 1.
  for (int i = 0; i < max_level; ++i) {
    int num_members = members_.size();
    ExpressionPtr k(
        new ConstantExpression(CcfGroup::Factorial(num_members - i) *
                               CcfGroup::Factorial(i) /
                               CcfGroup::Factorial(num_members - 1)));
    std::vector<ExpressionPtr> args;
    args.push_back(k);
    std::vector<ExpressionPtr> div_args;
    div_args.push_back(factors_[i].second);
    div_args.push_back(sum);
    ExpressionPtr fraction(new Div(div_args));
    args.push_back(fraction);
    args.push_back(distribution_);
    ExpressionPtr prob(new Mul(args));
    probabilities.push_back(prob);
  }
  assert(probabilities.size() == max_level);

  // Mapping of new basic events and their parents.
  std::map<BasicEventPtr, std::set<std::string> > new_events;
  CcfGroup::ConstructCcfBasicEvents(max_level, &new_events);
  assert(!new_events.empty());
  std::map<BasicEventPtr, std::set<std::string> >::iterator it;
  for (it = new_events.begin(); it != new_events.end(); ++it) {
    it->first->expression(probabilities[it->second.size() - 1]);
    // Add this basic event to the parent gates.
    std::set<std::string>::iterator it_l;
    for (it_l = it->second.begin(); it_l != it->second.end(); ++it_l) {
      gates_.find(*it_l)->second->AddChild(it->first);
    }
  }
}

void PhiFactorModel::Validate() {
  CcfGroup::Validate();
  double sum = 0;
  double sum_min = 0;
  double sum_max = 0;
  /// @todo How to assure that the sum will be 1 in sampling.
  ///       Is it allowed to have a factor sampling for Uncertainty analysis.
  std::vector<std::pair<int, ExpressionPtr> >::iterator it;
  for (it = CcfGroup::factors_.begin(); it != CcfGroup::factors_.end(); ++it) {
    sum += it->second->Mean();
    sum_min += it->second->Min();
    sum_max += it->second->Max();
  }
  /// @todo Problems with floating point number comparison.
  double epsilon = 1e-4;
  double diff = std::abs(sum - 1);
  double diff_min = std::abs(sum_min - 1);
  double diff_max = std::abs(sum_max - 1);
  if (diff_min > epsilon || diff > epsilon || diff_max > epsilon) {
    throw ValidationError("The factors for Phi model " + CcfGroup::name() +
                          " CCF group must sum to 1.");
  }
}

void PhiFactorModel::ApplyModel() {
  // Construct replacement gates for member basic events.
  // Create new basic events representing CCF.
  // Calculate CCF probabilities from the given factor.
  // Assign the CCF probabilities to the corresponding new basic events.
  // Add the new basic events to the replacement gates. These children
  // basic events must contain the parent gate id in its id.
  assert(gates_.empty());
  std::map<std::string, BasicEventPtr>::const_iterator it_m;
  for (it_m = members_.begin(); it_m != members_.end(); ++it_m) {
    GatePtr new_gate(new Gate(it_m->first, "or"));
    gates_.insert(std::make_pair(new_gate->id(), new_gate));
    it_m->second->ccf_gate(new_gate);
  }

  std::vector<ExpressionPtr> probabilities;  // The level is position + 1.
  std::vector<std::pair<int, ExpressionPtr> >::iterator it_f;
  for (it_f = factors_.begin(); it_f != factors_.end(); ++it_f) {
    std::vector<ExpressionPtr> args;
    args.push_back(it_f->second);
    args.push_back(distribution_);
    ExpressionPtr prob(new Mul(args));
    probabilities.push_back(prob);
  }

  int max_level = factors_.back().first;  // Assumes that factors are
                                          // sequential.
  // Mapping of new basic events and their parents.
  std::map<BasicEventPtr, std::set<std::string> > new_events;
  CcfGroup::ConstructCcfBasicEvents(max_level, &new_events);
  std::map<BasicEventPtr, std::set<std::string> >::iterator it;
  for (it = new_events.begin(); it != new_events.end(); ++it) {
    it->first->expression(probabilities[it->second.size() - 1]);
    // Add this basic event to the parent gates.
    std::set<std::string>::iterator it_l;
    for (it_l = it->second.begin(); it_l != it->second.end(); ++it_l) {
      gates_.find(*it_l)->second->AddChild(it->first);
    }
  }
}

}  // namespace scram
