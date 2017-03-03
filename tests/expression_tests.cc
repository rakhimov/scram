/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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
#include "expression/arithmetic.h"
#include "expression/exponential.h"
#include "expression/random_deviate.h"
#include "parameter.h"

#include <gtest/gtest.h>

#include "error.h"

namespace scram {
namespace mef {
namespace test {

// This mock class is used to specify
// return values and samples in a hard coded way.
class OpenExpression : public Expression {
 public:
  explicit OpenExpression(double m = 1, double s = 1, double mn = 0,
                          double mx = 0)
      : Expression({}),
        mean(m),
        sample(s),
        min(mn),
        max(mx) {}
  double mean;
  double sample;
  double min;  // This value is used only if explicitly set non-zero.
  double max;  // This value is used only if explicitly set non-zero.
  double Mean() noexcept override { return mean; }
  double DoSample() noexcept override { return sample; }
  double Max() noexcept override { return max ? max : sample; }
  double Min() noexcept override { return min ? min : sample; }
  bool IsDeviate() noexcept override { return false; }
};

using OpenExpressionPtr = std::shared_ptr<OpenExpression>;

namespace {

void TestProbability(const ExpressionPtr& expr, const OpenExpressionPtr& arg,
                     bool sample = true) {
  ASSERT_NO_THROW(expr->Validate());
  double value = arg->mean;
  arg->mean = -1;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->mean = 0.0;
  EXPECT_NO_THROW(expr->Validate());
  arg->mean = 2;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->mean = value;
  ASSERT_NO_THROW(expr->Validate());

  if (!sample)
    return;

  double sample_value = arg->sample;
  arg->sample = -1;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->sample = 0.0;
  EXPECT_NO_THROW(expr->Validate());
  arg->sample = 2;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->sample = sample_value;
  ASSERT_NO_THROW(expr->Validate());
}

void TestNegative(const ExpressionPtr& expr, const OpenExpressionPtr& arg,
                  bool sample = true) {
  ASSERT_NO_THROW(expr->Validate());
  double value = arg->mean;
  arg->mean = -1;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->mean = 0.0;
  EXPECT_NO_THROW(expr->Validate());
  arg->mean = 100;
  EXPECT_NO_THROW(expr->Validate());
  arg->mean = value;
  ASSERT_NO_THROW(expr->Validate());

  if (!sample)
    return;

  double sample_value = arg->sample;
  arg->sample = -1;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->sample = 0.0;
  EXPECT_NO_THROW(expr->Validate());
  arg->sample = 100;
  EXPECT_NO_THROW(expr->Validate());
  arg->sample = sample_value;
  ASSERT_NO_THROW(expr->Validate());
}

void TestNonPositive(const ExpressionPtr& expr, const OpenExpressionPtr& arg,
                     bool sample = true) {
  ASSERT_NO_THROW(expr->Validate());
  double value = arg->mean;
  arg->mean = -1;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->mean = 0.0;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->mean = 100;
  EXPECT_NO_THROW(expr->Validate());
  arg->mean = value;
  ASSERT_NO_THROW(expr->Validate());

  if (!sample)
    return;

  double sample_value = arg->sample;
  arg->sample = -1;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->sample = 0.0;
  EXPECT_THROW(expr->Validate(), InvalidArgument);
  arg->sample = 100;
  EXPECT_NO_THROW(expr->Validate());
  arg->sample = sample_value;
  ASSERT_NO_THROW(expr->Validate());
}

}  // namespace

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

  TestNegative(dev, lambda);
  TestNegative(dev, time);

  double sampled_value = 0;
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Resampling without resetting.
  ASSERT_FALSE(dev->IsDeviate());
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

  TestProbability(dev, gamma);
  TestNonPositive(dev, lambda);
  TestNegative(dev, mu);
  TestNegative(dev, time);

  double sampled_value = 0;
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_FALSE(dev->IsDeviate());
}

TEST(ExpressionTest, Weibull) {
  OpenExpressionPtr alpha(new OpenExpression(0.10, 0.8));
  OpenExpressionPtr beta(new OpenExpression(10, 8));
  OpenExpressionPtr t0(new OpenExpression(10, 10));
  OpenExpressionPtr time(new OpenExpression(500, 500));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new WeibullExpression(alpha, beta,
                                                            t0, time)));
  EXPECT_DOUBLE_EQ(1 - std::exp(-std::pow(40 / 0.1, 10)),
                   dev->Mean());

  TestNonPositive(dev, alpha);
  TestNonPositive(dev, beta);
  TestNegative(dev, t0);
  TestNegative(dev, time);

  t0->mean = 1000;  // More than the mission time.
  EXPECT_NO_THROW(dev->Validate());
  t0->mean = 10;
  ASSERT_NO_THROW(dev->Validate());
  t0->sample = 1000;
  EXPECT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_FALSE(dev->IsDeviate());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Resampling without resetting.
}

TEST(ExpressionTest, PeriodicTest4) {
  OpenExpressionPtr lambda(new OpenExpression(0.10, 0.10));
  OpenExpressionPtr tau(new OpenExpression(1, 1));
  OpenExpressionPtr theta(new OpenExpression(2, 2));
  OpenExpressionPtr time(new OpenExpression(5, 5));
  ExpressionPtr dev;
  ASSERT_NO_THROW(
      dev = ExpressionPtr(new PeriodicTest(lambda, tau, theta, time)));
  EXPECT_DOUBLE_EQ(1 - std::exp(-0.10), dev->Mean());

  TestNonPositive(dev, lambda);
  TestNonPositive(dev, tau);
  TestNegative(dev, theta);
  TestNegative(dev, time);

  double sampled_value = 0;
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Resampling without resetting.
  ASSERT_FALSE(dev->IsDeviate());
}

TEST(ExpressionTest, PeriodicTest5) {
  OpenExpressionPtr lambda(new OpenExpression(7e-4, 7e-4));
  OpenExpressionPtr mu(new OpenExpression(4e-4, 4e-4));
  OpenExpressionPtr tau(new OpenExpression(4020, 4020));
  OpenExpressionPtr theta(new OpenExpression(4740, 4740));
  OpenExpressionPtr time(new OpenExpression(8760, 8760));
  ExpressionPtr dev;

  ASSERT_NO_THROW(
      dev = ExpressionPtr(new PeriodicTest(lambda, mu, tau, theta, time)));
  EXPECT_FALSE(dev->IsDeviate());
  TestNegative(dev, mu);

  EXPECT_EQ(dev->Mean(), dev->Sample());
  EXPECT_NEAR(0.817508, dev->Mean(), 1e-5);

  tau->mean = 2010;
  EXPECT_NEAR(0.736611, dev->Mean(), 1e-5);

  tau->mean = 120;
  EXPECT_NEAR(0.645377, dev->Mean(), 1e-5);

  TestNegative(dev, theta);
  mu->mean = lambda->mean;  // Special case when divisor cannot be 0.
  EXPECT_NEAR(0.511579, dev->Mean(), 1e-5);
  mu->mean = 1e300;  // The same value is expected as for 4 arg periodic-test.
  EXPECT_NEAR(PeriodicTest(lambda, tau, theta, time).Mean(), dev->Mean(), 1e-5);
  mu->mean = 0;  // No repair is performed.
  EXPECT_NEAR(0.997828, dev->Mean(), 1e-5);
}

TEST(ExpressionTest, PeriodicTest11) {
  OpenExpressionPtr lambda(new OpenExpression(7e-4, 7e-4));
  OpenExpressionPtr lambda_test(new OpenExpression(6e-4, 6e-4));
  OpenExpressionPtr mu(new OpenExpression(4e-4, 4e-4));
  OpenExpressionPtr tau(new OpenExpression(120, 120));
  OpenExpressionPtr theta(new OpenExpression(4740, 4740));
  OpenExpressionPtr gamma(new OpenExpression(0.01, 0.01));
  OpenExpressionPtr test_duration(new OpenExpression(20, 20));
  OpenExpressionPtr available_at_test(new OpenExpression(true, true));
  OpenExpressionPtr sigma(new OpenExpression(0.9, 0.9));
  OpenExpressionPtr omega(new OpenExpression(0.01, 0.01));
  OpenExpressionPtr time(new OpenExpression(8760, 8760));
  ExpressionPtr dev;

  ASSERT_NO_THROW(dev = ExpressionPtr(new PeriodicTest(
                      lambda, lambda_test, mu, tau, theta, gamma, test_duration,
                      available_at_test, sigma, omega, time)));
  EXPECT_FALSE(dev->IsDeviate());
  TestNegative(dev, lambda_test);
  TestNonPositive(dev, test_duration);
  TestProbability(dev, gamma);
  TestProbability(dev, sigma);
  TestProbability(dev, omega);

  EXPECT_NEAR(0.668316, dev->Mean(), 1e-5);
  available_at_test->mean = false;
  EXPECT_NEAR(0.668316, dev->Mean(), 1e-5);
  time->mean = 4750;
  EXPECT_EQ(1, dev->Mean());
  time->mean = 4870;
  EXPECT_NEAR(0.996715, dev->Mean(), 1e-5);
  time->mean = 8710;
  EXPECT_NEAR(0.997478, dev->Mean(), 1e-5);
  time->mean = 8760;
  available_at_test->mean = true;

  lambda_test->mean = mu->mean = lambda->mean;
  EXPECT_NEAR(0.543401, dev->Mean(), 1e-5);
  mu->mean = 4e-4;
  lambda_test->mean = 6e-4;

  test_duration->mean = 120;
  EXPECT_NEAR(0.6469, dev->Mean(), 1e-5);

  tau->mean = 4020;
  test_duration->mean = 0;
  omega->mean = 0;
  sigma->mean = 1;
  gamma->mean = 0;
  EXPECT_NEAR(0.817508, dev->Mean(), 1e-5);

  tau->mean = 120;
  EXPECT_NEAR(0.645377, dev->Mean(), 1e-5);
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
  EXPECT_NO_THROW(dev->Validate());
  min->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsDeviate());
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

  ASSERT_NO_THROW(dev->Validate());
  mean->mean = 2;
  EXPECT_NO_THROW(dev->Validate());
  mean->mean = 0;
  EXPECT_NO_THROW(dev->Validate());
  mean->mean = 10;
  ASSERT_NO_THROW(dev->Validate());

  TestNonPositive(dev, sigma, /*sample=*/false);

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsDeviate());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Log-Normal deviate test for invalid mean, error factor, and level.
TEST(ExpressionTest, LogNormalDeviateLogarithmic) {
  OpenExpressionPtr mean(new OpenExpression(10, 5));
  OpenExpressionPtr ef(new OpenExpression(5, 3));
  OpenExpressionPtr level(new OpenExpression(0.95, 0.95, 0.6, 0.9));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new LogNormalDeviate(mean, ef, level)));

  EXPECT_EQ(mean->Mean(), dev->Mean());
  EXPECT_EQ(0, dev->Min());

  level->mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  level->mean = 2;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  level->mean = 0.95;

  TestNonPositive(dev, mean, /*sample=*/false);

  ef->mean = -1;  // ef < 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef->mean = 1;  // ef = 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef->mean = 2;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  ef->sample = 1;
  EXPECT_NO_THROW(dev->Validate());
  ef->sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  ef->sample = 3;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsDeviate());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Log-Normal deviate with invalid normal mean and standard deviation.
TEST(ExpressionTest, LogNormalDeviateNormal) {
  OpenExpressionPtr mu(new OpenExpression(10, 1));
  OpenExpressionPtr sigma(new OpenExpression(5, 4));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new LogNormalDeviate(mu, sigma)));

  EXPECT_NEAR(5.9105e9, dev->Mean(), 1e6);
  EXPECT_EQ(0, dev->Min());

  ASSERT_NO_THROW(dev->Validate());
  mu->mean = 2;
  EXPECT_NO_THROW(dev->Validate());
  mu->mean = 0;
  EXPECT_NO_THROW(dev->Validate());
  mu->mean = 10;
  ASSERT_NO_THROW(dev->Validate());

  TestNonPositive(dev, sigma, /*sample=*/false);
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsDeviate());
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

  TestNonPositive(dev, k, /*sample=*/false);
  TestNonPositive(dev, theta, /*sample=*/false);

  ASSERT_NO_THROW(dev->Validate());
  k->sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  k->sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  k->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  theta->sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  theta->sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  theta->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsDeviate());
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

  TestNonPositive(dev, alpha, /*sample=*/false);
  TestNonPositive(dev, beta, /*sample=*/false);

  ASSERT_NO_THROW(dev->Validate());
  alpha->sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  alpha->sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  alpha->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  beta->sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  beta->sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  beta->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsDeviate());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Test for histogram distribution arguments and sampling.
TEST(ExpressionTest, Histogram) {
  std::vector<ExpressionPtr> boundaries;
  std::vector<ExpressionPtr> weights;
  OpenExpressionPtr b0(new OpenExpression(0, 0));
  OpenExpressionPtr b1(new OpenExpression(1, 1));
  OpenExpressionPtr b2(new OpenExpression(3, 3));
  boundaries.push_back(b0);
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

  ExpressionPtr dev(new Histogram(boundaries, weights));
  EXPECT_NO_THROW(dev->Validate());
  b0->mean = 0.5;
  EXPECT_NO_THROW(dev->Validate());
  b0->mean = 0;
  EXPECT_DOUBLE_EQ(1.5, dev->Mean());

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
  EXPECT_NO_THROW(dev->Validate());
  b1->sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  b1->sample = b2->sample;
  EXPECT_NO_THROW(dev->Validate());
  b1->sample = b2->sample + 1;
  EXPECT_NO_THROW(dev->Validate());
  b1->sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  w1->sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  w1->sample = 2;
  ASSERT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsDeviate());
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

// Test expression initialization with 2 or more arguments.
TEST(ExpressionTest, BinaryExpression) {
  std::vector<ExpressionPtr> arguments;
  ExpressionPtr dev;
  EXPECT_THROW(dev = ExpressionPtr(new Add(arguments)), InvalidArgument);
  arguments.push_back(OpenExpressionPtr(new OpenExpression(10, 20)));
  EXPECT_THROW(dev = ExpressionPtr(new Add(arguments)), InvalidArgument);
  arguments.push_back(OpenExpressionPtr(new OpenExpression(30, 40)));
  EXPECT_NO_THROW(dev = ExpressionPtr(new Add(arguments)));
  arguments.push_back(OpenExpressionPtr(new OpenExpression(30, 40)));
  EXPECT_NO_THROW(dev = ExpressionPtr(new Add(arguments)));
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
}  // namespace mef
}  // namespace scram
