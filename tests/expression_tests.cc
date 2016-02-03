/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "expression.h"

#include <gtest/gtest.h>

#include "error.h"

namespace scram {
namespace test {

// This mock class is used to specify
// return values and samples in a hard coded way.
class OpenExpression : public Expression {
 public:
  explicit OpenExpression(double m = 1, double s = 1, double mn = 0,
                          double mx = 0)
      : Expression::Expression({}),
        mean(m),
        sample(s),
        min(mn),
        max(mx) {}
  double mean;
  double sample;
  double min;  // This value is used only if explicitly set non-zero.
  double max;  // This value is used only if explicitly set non-zero.
  double Mean() noexcept { return mean; }
  double GetSample() noexcept { return sample; }
  double Max() noexcept { return max ? max : sample; }
  double Min() noexcept { return min ? min : sample; }
  bool IsConstant() noexcept { return true; }
};

using OpenExpressionPtr = std::shared_ptr<OpenExpression>;

TEST(ExpressionTest, Parameter) {
  OpenExpressionPtr expr(new OpenExpression(10, 8));
  ParameterPtr param;
  ASSERT_NO_THROW(param = ParameterPtr(new Parameter("param")));
  ASSERT_NO_THROW(param->expression(expr));
  ASSERT_THROW(param->expression(expr), LogicError);
}

TEST(ExpressionTest, Exponential) {
  OpenExpressionPtr lambda(new OpenExpression(10, 8));
  OpenExpressionPtr time(new OpenExpression(5, 4));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new ExponentialExpression(lambda, time)));
  EXPECT_DOUBLE_EQ(1 - std::exp(-50), dev->Mean());

  lambda->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  lambda->mean = 10;
  ASSERT_NO_THROW(dev->Validate());

  time->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  time->mean = 5;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  lambda->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  lambda->sample = 10;
  ASSERT_NO_THROW(dev->Validate());

  time->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  time->sample = 5;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Resampling without resetting.
  ASSERT_TRUE(dev->IsConstant());
}

TEST(ExpressionTest, GLM) {
  OpenExpressionPtr gamma(new OpenExpression(0.10, 0.8));
  OpenExpressionPtr lambda(new OpenExpression(10, 8));
  OpenExpressionPtr mu(new OpenExpression(100, 80));
  OpenExpressionPtr time(new OpenExpression(5, 4));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new GlmExpression(gamma, lambda,
                                                        mu, time)));
  EXPECT_DOUBLE_EQ((10 - (10 - 0.10 * 110) * std::exp(-110 * 5)) / 110,
                   dev->Mean());


  gamma->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  gamma->mean = 10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  gamma->mean = 0.10;
  ASSERT_NO_THROW(dev->Validate());

  lambda->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  lambda->mean = 10;
  ASSERT_NO_THROW(dev->Validate());

  mu->mean = -10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  mu->mean = 100;
  ASSERT_NO_THROW(dev->Validate());

  time->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  time->mean = 5;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  gamma->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  gamma->sample = 10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  gamma->sample = 0.10;
  ASSERT_NO_THROW(dev->Validate());

  lambda->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  lambda->sample = 10;
  ASSERT_NO_THROW(dev->Validate());

  mu->sample = -10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  mu->sample = 100;
  ASSERT_NO_THROW(dev->Validate());

  time->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  time->sample = 5;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_TRUE(dev->IsConstant());
}

TEST(ExpressionTest, Weibull) {
  OpenExpressionPtr alpha(new OpenExpression(0.10, 0.8));
  OpenExpressionPtr beta(new OpenExpression(10, 8));
  OpenExpressionPtr t0(new OpenExpression(10, 8));
  OpenExpressionPtr time(new OpenExpression(50, 40));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new WeibullExpression(alpha, beta,
                                                            t0, time)));
  EXPECT_DOUBLE_EQ(1 - std::exp(-std::pow(40 / 0.1, 10)),
                   dev->Mean());


  alpha->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  alpha->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  alpha->mean = 0.10;
  ASSERT_NO_THROW(dev->Validate());

  beta->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  beta->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  beta->mean = 10;
  ASSERT_NO_THROW(dev->Validate());

  t0->mean = -10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  t0->mean = 100;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  t0->mean = 10;
  ASSERT_NO_THROW(dev->Validate());

  time->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  time->mean = 50;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  alpha->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  alpha->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  alpha->sample = 0.10;
  ASSERT_NO_THROW(dev->Validate());

  beta->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  beta->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  beta->sample = 10;
  ASSERT_NO_THROW(dev->Validate());

  t0->sample = -10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  t0->sample = 100;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  t0->sample = 10;
  ASSERT_NO_THROW(dev->Validate());

  time->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  time->sample = 50;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsConstant());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Resampling without resetting.
}

// Uniform deviate test for invalid minimum and maximum values.
TEST(ExpressionTest, UniformDeviate) {
  OpenExpressionPtr min(new OpenExpression(1, 2));
  OpenExpressionPtr max(new OpenExpression(5, 4));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new UniformDeviate(min, max)));
  EXPECT_DOUBLE_EQ(3, dev->Mean());

  min->mean = 10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  min->mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  min->sample = 10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);  // min > max
  min->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_FALSE(dev->IsConstant());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Normal deviate test for invalid standard deviation.
TEST(ExpressionTest, NormalDeviate) {
  OpenExpressionPtr mean(new OpenExpression(10, 1));
  OpenExpressionPtr sigma(new OpenExpression(5, 4));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new NormalDeviate(mean, sigma)));

  sigma->mean = -5;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  sigma->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  sigma->mean = 5;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  sigma->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);  // sigma < 0
  sigma->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);  // sigma = 0
  sigma->sample = 1;
  EXPECT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_FALSE(dev->IsConstant());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Log-Normal deviate test for invalid mean, error factor, and level.
TEST(ExpressionTest, LogNormalDeviate) {
  OpenExpressionPtr mean(new OpenExpression(10, 5));
  OpenExpressionPtr ef(new OpenExpression(5, 3));
  OpenExpressionPtr level(new OpenExpression(0.95, 0.95, 0.6, 0.9));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new LogNormalDeviate(mean, ef, level)));

  ASSERT_NO_THROW(dev->Validate());
  level->mean = -0.5;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  level->mean = 2;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  level->mean = 0.95;
  ASSERT_NO_THROW(dev->Validate());

  mean->mean = -1;  // mean < 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  mean->mean = 0;  // mean = 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  mean->mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  ef->mean = -1;  // ef < 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef->mean = 1;  // ef = 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef->mean = 2;
  dev->Validate();
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  mean->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  mean->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  mean->sample = 5;
  ASSERT_NO_THROW(dev->Validate());
  ef->sample = 1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef->sample = 3;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_FALSE(dev->IsConstant());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Gamma deviate test for invalid arguments.
TEST(ExpressionTest, GammaDeviate) {
  OpenExpressionPtr k(new OpenExpression(3, 5));
  OpenExpressionPtr theta(new OpenExpression(7, 1));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new GammaDeviate(k, theta)));
  EXPECT_DOUBLE_EQ(21, dev->Mean());

  k->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  k->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  k->mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  theta->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  theta->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  theta->mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  k->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  k->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  k->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  theta->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  theta->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  theta->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_FALSE(dev->IsConstant());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Beta deviate test for invalid arguments.
TEST(ExpressionTest, BetaDeviate) {
  OpenExpressionPtr alpha(new OpenExpression(8, 5));
  OpenExpressionPtr beta(new OpenExpression(2, 1));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new BetaDeviate(alpha, beta)));
  EXPECT_DOUBLE_EQ(0.8, dev->Mean());

  alpha->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  alpha->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  alpha->mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  beta->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  beta->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  beta->mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  alpha->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  alpha->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  alpha->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  beta->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  beta->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  beta->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_FALSE(dev->IsConstant());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Test for histogram distribution arguments and sampling.
TEST(ExpressionTest, Histogram) {
  std::vector<ExpressionPtr> boundaries;
  std::vector<ExpressionPtr> weights;
  OpenExpressionPtr b1(new OpenExpression(1, 1));
  OpenExpressionPtr b2(new OpenExpression(3, 3));
  boundaries.push_back(b1);
  boundaries.push_back(b2);
  OpenExpressionPtr w1(new OpenExpression(2, 2));
  OpenExpressionPtr w2(new OpenExpression(4, 4));
  weights.push_back(w1);
  weights.push_back(w2);

  // Size mismatch.
  weights.push_back(OpenExpressionPtr(new OpenExpression()));
  EXPECT_THROW(Histogram(boundaries, weights), InvalidArgument);
  weights.pop_back();
  ASSERT_NO_THROW(Histogram(boundaries, weights));

  ExpressionPtr dev;
  EXPECT_NO_THROW(dev = ExpressionPtr(new Histogram(boundaries, weights)));
  EXPECT_DOUBLE_EQ(10.0 / 18, dev->Mean());

  b1->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1->mean = b2->mean;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1->mean = b2->mean + 1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1->mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  w1->mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  w1->mean = 2;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  b1->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1->sample = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1->sample = b2->sample;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1->sample = b2->sample + 1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  w1->sample = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  w1->sample = 2;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_FALSE(dev->IsConstant());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Test for negation of an expression.
TEST(ExpressionTest, Neg) {
  OpenExpressionPtr expression(new OpenExpression(10, 8));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new Neg(expression)));
  EXPECT_DOUBLE_EQ(-10, dev->Mean());
  EXPECT_DOUBLE_EQ(-8, dev->Sample());
  expression->max = 100;
  expression->min = 1;
  EXPECT_DOUBLE_EQ(-1, dev->Max());
  EXPECT_DOUBLE_EQ(-100, dev->Min());
}

// Test for addition of expressions.
TEST(ExpressionTest, Add) {
  std::vector<ExpressionPtr> arguments;
  arguments.push_back(OpenExpressionPtr(new OpenExpression(10, 20)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(30, 40)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(50, 60)));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new Add(arguments)));
  EXPECT_DOUBLE_EQ(90, dev->Mean());
  EXPECT_DOUBLE_EQ(120, dev->Sample());
  EXPECT_DOUBLE_EQ(120, dev->Max());
  EXPECT_DOUBLE_EQ(120, dev->Min());
}

// Test for subtraction of expressions.
TEST(ExpressionTest, Sub) {
  std::vector<ExpressionPtr> arguments;
  arguments.push_back(OpenExpressionPtr(new OpenExpression(10, 20)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(30, 40)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(50, 60)));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new Sub(arguments)));
  EXPECT_DOUBLE_EQ(-70, dev->Mean());
  EXPECT_DOUBLE_EQ(-80, dev->Sample());
  EXPECT_DOUBLE_EQ(-80, dev->Max());
  EXPECT_DOUBLE_EQ(-80, dev->Min());
}

// Test for multiplication of expressions.
TEST(ExpressionTest, Mul) {
  std::vector<ExpressionPtr> arguments;
  arguments.push_back(OpenExpressionPtr(new OpenExpression(1, 2, 0.1, 10)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(3, 4, 1, 5)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(5, 6, 2, 6)));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new Mul(arguments)));
  EXPECT_DOUBLE_EQ(15, dev->Mean());
  EXPECT_DOUBLE_EQ(48, dev->Sample());
  EXPECT_DOUBLE_EQ(0.2, dev->Min());
  EXPECT_DOUBLE_EQ(300, dev->Max());
}

// Test for the special case of finding maximum and minimum multiplication.
TEST(ExpressionTest, MultiplicationMaxAndMin) {
  std::vector<ExpressionPtr> arguments;
  arguments.push_back(OpenExpressionPtr(new OpenExpression(1, 2, -1, 2)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(3, 4, -7, -4)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(5, 6, 1, 5)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(4, 3, -2, 4)));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new Mul(arguments)));
  EXPECT_DOUBLE_EQ(60, dev->Mean());
  EXPECT_DOUBLE_EQ(144, dev->Sample());
  EXPECT_DOUBLE_EQ(2 * -7 * 5 * 4, dev->Min());
  EXPECT_DOUBLE_EQ(2 * -7 * 5 * -2, dev->Max());  // Sign matters.
}

// Test for division of expressions.
TEST(ExpressionTest, Div) {
  std::vector<ExpressionPtr> arguments;
  arguments.push_back(OpenExpressionPtr(new OpenExpression(1, 2, 0.1, 10)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(3, 4, 1, 5)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(5, 6, 2, 6)));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new Div(arguments)));
  EXPECT_DOUBLE_EQ(1.0 / 3 / 5, dev->Mean());
  EXPECT_DOUBLE_EQ(2.0 / 4 / 6, dev->Sample());
  EXPECT_DOUBLE_EQ(0.1 / 5 / 6, dev->Min());
  EXPECT_DOUBLE_EQ(10.0 / 1 / 2, dev->Max());

  arguments.push_back(OpenExpressionPtr(new OpenExpression(0, 1, 1, 1)));
  ASSERT_NO_THROW(dev = ExpressionPtr(new Div(arguments)));
  EXPECT_THROW(dev->Validate(), InvalidArgument);  // Division by 0.
}

// Test for the special case of finding maximum and minimum division.
TEST(ExpressionTest, DivisionMaxAndMin) {
  std::vector<ExpressionPtr> arguments;
  arguments.push_back(OpenExpressionPtr(new OpenExpression(1, 2, -1, 2)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(3, 4, -7, -4)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(5, 6, 1, 5)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(4, 3, -2, 4)));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new Div(arguments)));
  EXPECT_DOUBLE_EQ(1.0 / 3 / 5 / 4, dev->Mean());
  EXPECT_DOUBLE_EQ(2.0 / 4 / 6 / 3, dev->Sample());
  EXPECT_DOUBLE_EQ(-1.0 / -4 / 1 / -2, dev->Min());
  EXPECT_DOUBLE_EQ(2.0 / -4 / 1 / -2, dev->Max());
}

}  // namespace test
}  // namespace scram
