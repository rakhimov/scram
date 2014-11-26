#include "ccf_group.h"

#include <boost/shared_ptr.hpp>
#include <gtest/gtest.h>

#include "error.h"
#include "event.h"
#include "expression.h"

using namespace scram;

typedef boost::shared_ptr<BasicEvent> BasicEventPtr;
typedef boost::shared_ptr<Expression> ExpressionPtr;

TEST(CcfGroupTest, AddMemberRepeated) {
  CcfGroup* ccf_group = new BetaFactorModel("general");
  BasicEventPtr member(new BasicEvent("id"));
  ASSERT_NO_THROW(ccf_group->AddMember(member));
  EXPECT_THROW(ccf_group->AddMember(member), LogicError);
  delete ccf_group;
}

TEST(CcfGroupTest, AddMemberAfterDistribution) {
  CcfGroup* ccf_group = new BetaFactorModel("general");

  BasicEventPtr member(new BasicEvent("id"));
  ASSERT_NO_THROW(ccf_group->AddMember(member));

  ExpressionPtr distr(new ConstantExpression(1));
  ASSERT_NO_THROW(ccf_group->AddDistribution(distr));

  BasicEventPtr member_two(new BasicEvent("two"));
  EXPECT_THROW(ccf_group->AddMember(member), IllegalOperation);
  delete ccf_group;
}
