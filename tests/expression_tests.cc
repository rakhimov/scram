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
      : mean(m),
        sample(s),
        min(mn),
        max(mx) {}
  double mean;
  double sample;
  double min;  // This value is used only if explicitly set non-zero.
  double max;  // This value is used only if explicitly set non-zero.
  double value() noexcept override { return mean; }
  double DoSample() noexcept override { return sample; }
  Interval interval() noexcept override {
    return Interval::closed(min ? min : sample, max ? max : sample);
  }
  bool IsDeviate() noexcept override { return false; }
};

namespace {

template <class T>
std::unique_ptr<T> MakeUnique(std::initializer_list<Expression*> args) {
  return std::make_unique<T>(args);
}

void TestProbability(Expression* expr, OpenExpression* arg,
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

void TestNegative(Expression* expr, OpenExpression* arg, bool sample = true) {
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

void TestNonPositive(Expression* expr, OpenExpression* arg,
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
  OpenExpression expr(10, 8);
  ParameterPtr param;
  ASSERT_NO_THROW(param = ParameterPtr(new Parameter("param")));
  ASSERT_NO_THROW(param->expression(&expr));
  ASSERT_THROW(param->expression(&expr), LogicError);
}

TEST(ExpressionTest, Exponential) {
  OpenExpression lambda(10, 8);
  OpenExpression time(5, 4);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(
      dev = std::make_unique<ExponentialExpression>(&lambda, &time));
  EXPECT_DOUBLE_EQ(1 - std::exp(-50), dev->value());

  TestNegative(dev.get(), &lambda);
  TestNegative(dev.get(), &time);

  double sampled_value = 0;
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Resampling without resetting.
  ASSERT_FALSE(dev->IsDeviate());
}

TEST(ExpressionTest, GLM) {
  OpenExpression gamma(0.10, 0.8);
  OpenExpression lambda(10, 8);
  OpenExpression mu(100, 80);
  OpenExpression time(5, 4);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(
      dev = std::make_unique<GlmExpression>(&gamma, &lambda, &mu, &time));
  EXPECT_DOUBLE_EQ((10 - (10 - 0.10 * 110) * std::exp(-110 * 5)) / 110,
                   dev->value());

  TestProbability(dev.get(), &gamma);
  TestNonPositive(dev.get(), &lambda);
  TestNegative(dev.get(), &mu);
  TestNegative(dev.get(), &time);

  double sampled_value = 0;
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_FALSE(dev->IsDeviate());
}

TEST(ExpressionTest, Weibull) {
  OpenExpression alpha(0.10, 0.8);
  OpenExpression beta(10, 8);
  OpenExpression t0(10, 10);
  OpenExpression time(500, 500);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(
      dev = std::make_unique<WeibullExpression>(&alpha, &beta, &t0, &time));
  EXPECT_DOUBLE_EQ(1 - std::exp(-std::pow(40 / 0.1, 10)),
                   dev->value());

  TestNonPositive(dev.get(), &alpha);
  TestNonPositive(dev.get(), &beta);
  TestNegative(dev.get(), &t0);
  TestNegative(dev.get(), &time);

  t0.mean = 1000;  // More than the mission time.
  EXPECT_NO_THROW(dev->Validate());
  t0.mean = 10;
  ASSERT_NO_THROW(dev->Validate());
  t0.sample = 1000;
  EXPECT_NO_THROW(dev->Validate());

  double sampled_value = 0;
  ASSERT_FALSE(dev->IsDeviate());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Resampling without resetting.
}

TEST(ExpressionTest, PeriodicTest4) {
  OpenExpression lambda(0.10, 0.10);
  OpenExpression tau(1, 1);
  OpenExpression theta(2, 2);
  OpenExpression time(5, 5);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(
      dev = std::make_unique<PeriodicTest>(&lambda, &tau, &theta, &time));
  EXPECT_DOUBLE_EQ(1 - std::exp(-0.10), dev->value());

  TestNonPositive(dev.get(), &lambda);
  TestNonPositive(dev.get(), &tau);
  TestNegative(dev.get(), &theta);
  TestNegative(dev.get(), &time);

  double sampled_value = 0;
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Resampling without resetting.
  ASSERT_FALSE(dev->IsDeviate());
}

TEST(ExpressionTest, PeriodicTest5) {
  OpenExpression lambda(7e-4, 7e-4);
  OpenExpression mu(4e-4, 4e-4);
  OpenExpression tau(4020, 4020);
  OpenExpression theta(4740, 4740);
  OpenExpression time(8760, 8760);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(
      dev = std::make_unique<PeriodicTest>(&lambda, &mu, &tau, &theta, &time));
  EXPECT_FALSE(dev->IsDeviate());
  TestNegative(dev.get(), &mu);

  EXPECT_EQ(dev->value(), dev->Sample());
  EXPECT_NEAR(0.817508, dev->value(), 1e-5);

  tau.mean = 2010;
  EXPECT_NEAR(0.736611, dev->value(), 1e-5);

  tau.mean = 120;
  EXPECT_NEAR(0.645377, dev->value(), 1e-5);

  TestNegative(dev.get(), &theta);
  mu.mean = lambda.mean;  // Special case when divisor cannot be 0.
  EXPECT_NEAR(0.511579, dev->value(), 1e-5);
  mu.mean = 1e300;  // The same value is expected as for 4 arg periodic-test.
  EXPECT_NEAR(PeriodicTest(&lambda, &tau, &theta, &time).value(), dev->value(),
              1e-5);
  mu.mean = 0;  // No repair is performed.
  EXPECT_NEAR(0.997828, dev->value(), 1e-5);
}

TEST(ExpressionTest, PeriodicTest11) {
  OpenExpression lambda(7e-4, 7e-4);
  OpenExpression lambda_test(6e-4, 6e-4);
  OpenExpression mu(4e-4, 4e-4);
  OpenExpression tau(120, 120);
  OpenExpression theta(4740, 4740);
  OpenExpression gamma(0.01, 0.01);
  OpenExpression test_duration(20, 20);
  OpenExpression available_at_test(true, true);
  OpenExpression sigma(0.9, 0.9);
  OpenExpression omega(0.01, 0.01);
  OpenExpression time(8760, 8760);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = std::make_unique<PeriodicTest>(
                      &lambda, &lambda_test, &mu, &tau, &theta, &gamma,
                      &test_duration, &available_at_test, &sigma, &omega,
                      &time));
  EXPECT_FALSE(dev->IsDeviate());
  TestNegative(dev.get(), &lambda_test);
  TestNonPositive(dev.get(), &test_duration);
  TestProbability(dev.get(), &gamma);
  TestProbability(dev.get(), &sigma);
  TestProbability(dev.get(), &omega);

  EXPECT_NEAR(0.668316, dev->value(), 1e-5);
  available_at_test.mean = false;
  EXPECT_NEAR(0.668316, dev->value(), 1e-5);
  time.mean = 4750;
  EXPECT_EQ(1, dev->value());
  time.mean = 4870;
  EXPECT_NEAR(0.996715, dev->value(), 1e-5);
  time.mean = 8710;
  EXPECT_NEAR(0.997478, dev->value(), 1e-5);
  time.mean = 8760;
  available_at_test.mean = true;

  lambda_test.mean = mu.mean = lambda.mean;
  EXPECT_NEAR(0.543401, dev->value(), 1e-5);
  mu.mean = 4e-4;
  lambda_test.mean = 6e-4;

  test_duration.mean = 120;
  EXPECT_NEAR(0.6469, dev->value(), 1e-5);

  tau.mean = 4020;
  test_duration.mean = 0;
  omega.mean = 0;
  sigma.mean = 1;
  gamma.mean = 0;
  EXPECT_NEAR(0.817508, dev->value(), 1e-5);

  tau.mean = 120;
  EXPECT_NEAR(0.645377, dev->value(), 1e-5);
}

// Uniform deviate test for invalid minimum and maximum values.
TEST(ExpressionTest, UniformDeviate) {
  OpenExpression min(1, 2);
  OpenExpression max(5, 4);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = std::make_unique<UniformDeviate>(&min, &max));
  EXPECT_DOUBLE_EQ(3, dev->value());

  min.mean = 10;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  min.mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  min.sample = 10;
  EXPECT_NO_THROW(dev->Validate());
  min.sample = 1;
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
  OpenExpression mean(10, 1);
  OpenExpression sigma(5, 4);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = std::make_unique<NormalDeviate>(&mean, &sigma));

  ASSERT_NO_THROW(dev->Validate());
  mean.mean = 2;
  EXPECT_NO_THROW(dev->Validate());
  mean.mean = 0;
  EXPECT_NO_THROW(dev->Validate());
  mean.mean = 10;
  ASSERT_NO_THROW(dev->Validate());

  TestNonPositive(dev.get(), &sigma, /*sample=*/false);

  double sampled_value = 0;
  ASSERT_TRUE(dev->IsDeviate());
  ASSERT_NO_THROW(sampled_value = dev->Sample());
  EXPECT_EQ(sampled_value, dev->Sample());  // Re-sampling without resetting.
  ASSERT_NO_THROW(dev->Reset());
  EXPECT_NE(sampled_value, dev->Sample());
}

// Log-Normal deviate test for invalid mean, error factor, and level.
TEST(ExpressionTest, LogNormalDeviateLogarithmic) {
  OpenExpression mean(10, 5);
  OpenExpression ef(5, 3);
  OpenExpression level(0.95, 0.95, 0.6, 0.9);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = std::make_unique<LogNormalDeviate>(&mean, &ef, &level));

  EXPECT_EQ(mean.value(), dev->value());
  EXPECT_EQ(0, dev->interval().lower());
  EXPECT_EQ(IntervalBounds::left_open(), dev->interval().bounds());

  level.mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  level.mean = 2;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  level.mean = 0.95;

  TestNonPositive(dev.get(), &mean, /*sample=*/false);

  ef.mean = -1;  // ef < 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef.mean = 1;  // ef = 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef.mean = 2;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  ef.sample = 1;
  EXPECT_NO_THROW(dev->Validate());
  ef.sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  ef.sample = 3;
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
  OpenExpression mu(10, 1);
  OpenExpression sigma(5, 4);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = std::make_unique<LogNormalDeviate>(&mu, &sigma));

  EXPECT_NEAR(5.9105e9, dev->value(), 1e6);
  EXPECT_EQ(0, dev->interval().lower());
  EXPECT_EQ(IntervalBounds::left_open(), dev->interval().bounds());

  ASSERT_NO_THROW(dev->Validate());
  mu.mean = 2;
  EXPECT_NO_THROW(dev->Validate());
  mu.mean = 0;
  EXPECT_NO_THROW(dev->Validate());
  mu.mean = 10;
  ASSERT_NO_THROW(dev->Validate());

  TestNonPositive(dev.get(), &sigma, /*sample=*/false);
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
  OpenExpression k(3, 5);
  OpenExpression theta(7, 1);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = std::make_unique<GammaDeviate>(&k, &theta));
  EXPECT_DOUBLE_EQ(21, dev->value());

  TestNonPositive(dev.get(), &k, /*sample=*/false);
  TestNonPositive(dev.get(), &theta, /*sample=*/false);

  ASSERT_NO_THROW(dev->Validate());
  k.sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  k.sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  k.sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  theta.sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  theta.sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  theta.sample = 1;
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
  OpenExpression alpha(8, 5);
  OpenExpression beta(2, 1);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = std::make_unique<BetaDeviate>(&alpha, &beta));
  EXPECT_DOUBLE_EQ(0.8, dev->value());

  TestNonPositive(dev.get(), &alpha, /*sample=*/false);
  TestNonPositive(dev.get(), &beta, /*sample=*/false);

  ASSERT_NO_THROW(dev->Validate());
  alpha.sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  alpha.sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  alpha.sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  beta.sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  beta.sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  beta.sample = 1;
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
  std::vector<Expression*> boundaries;
  std::vector<Expression*> weights;
  OpenExpression b0(0, 0);
  OpenExpression b1(1, 1);
  OpenExpression b2(3, 3);
  boundaries.push_back(&b0);
  boundaries.push_back(&b1);
  boundaries.push_back(&b2);
  OpenExpression w1(2, 2);
  OpenExpression w2(4, 4);
  OpenExpression w3(5, 5);
  weights.push_back(&w1);
  weights.push_back(&w2);

  // Size mismatch.
  weights.push_back(&w3);
  EXPECT_THROW(Histogram(boundaries, weights), InvalidArgument);
  weights.pop_back();
  ASSERT_NO_THROW(Histogram(boundaries, weights));

  auto dev = std::make_unique<Histogram>(boundaries, weights);
  EXPECT_NO_THROW(dev->Validate());
  b0.mean = 0.5;
  EXPECT_NO_THROW(dev->Validate());
  b0.mean = 0;
  EXPECT_DOUBLE_EQ(1.5, dev->value());

  b1.mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1.mean = 0;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1.mean = b2.mean;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1.mean = b2.mean + 1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  b1.mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  w1.mean = -1;
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  w1.mean = 2;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Validate());
  b1.sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  b1.sample = 0;
  EXPECT_NO_THROW(dev->Validate());
  b1.sample = b2.sample;
  EXPECT_NO_THROW(dev->Validate());
  b1.sample = b2.sample + 1;
  EXPECT_NO_THROW(dev->Validate());
  b1.sample = 1;
  ASSERT_NO_THROW(dev->Validate());

  w1.sample = -1;
  EXPECT_NO_THROW(dev->Validate());
  w1.sample = 2;
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
  OpenExpression expression(10, 8);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = std::make_unique<Neg>(&expression));
  EXPECT_DOUBLE_EQ(-10, dev->value());
  EXPECT_DOUBLE_EQ(-8, dev->Sample());
  expression.max = 100;
  expression.min = 1;
  EXPECT_TRUE(Interval::closed(-100, -1) == dev->interval()) << dev->interval();
}

// Test expression initialization with 2 or more arguments.
TEST(ExpressionTest, BinaryExpression) {
  std::vector<Expression*> arguments;
  EXPECT_THROW(Add{arguments}, InvalidArgument);
  OpenExpression arg_one(10, 20);
  arguments.push_back(&arg_one);
  EXPECT_THROW(Add{arguments}, InvalidArgument);

  OpenExpression arg_two(30, 40);
  arguments.push_back(&arg_two);
  EXPECT_NO_THROW(Add{arguments});
  arguments.push_back(&arg_two);
  EXPECT_NO_THROW(Add{arguments});
}

// Test for addition of expressions.
TEST(ExpressionTest, Add) {
  OpenExpression arg_one(10, 20);
  OpenExpression arg_two(30, 40);
  OpenExpression arg_three(50, 60);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = MakeUnique<Add>({&arg_one, &arg_two, &arg_three}));
  EXPECT_DOUBLE_EQ(90, dev->value());
  EXPECT_DOUBLE_EQ(120, dev->Sample());
  EXPECT_TRUE(Interval::closed(120, 120) == dev->interval()) << dev->interval();
}

// Test for subtraction of expressions.
TEST(ExpressionTest, Sub) {
  OpenExpression arg_one(10, 20);
  OpenExpression arg_two(30, 40);
  OpenExpression arg_three(50, 60);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = MakeUnique<Sub>({&arg_one, &arg_two, &arg_three}));
  EXPECT_DOUBLE_EQ(-70, dev->value());
  EXPECT_DOUBLE_EQ(-80, dev->Sample());
  EXPECT_TRUE(Interval::closed(-80, -80) == dev->interval()) << dev->interval();
}

// Test for multiplication of expressions.
TEST(ExpressionTest, Mul) {
  OpenExpression arg_one(1, 2, 0.1, 10);
  OpenExpression arg_two(3, 4, 1, 5);
  OpenExpression arg_three(5, 6, 2, 6);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = MakeUnique<Mul>({&arg_one, &arg_two, &arg_three}));
  EXPECT_DOUBLE_EQ(15, dev->value());
  EXPECT_DOUBLE_EQ(48, dev->Sample());
  EXPECT_TRUE(Interval::closed(0.2, 300) == dev->interval()) << dev->interval();
}

// Test for the special case of finding maximum and minimum multiplication.
TEST(ExpressionTest, MultiplicationMaxAndMin) {
  OpenExpression arg_one(1, 2, -1, 2);
  OpenExpression arg_two(3, 4, -7, -4);
  OpenExpression arg_three(5, 6, 1, 5);
  OpenExpression arg_four(4, 3, -2, 4);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(
      dev = MakeUnique<Mul>({&arg_one, &arg_two, &arg_three, &arg_four}));
  EXPECT_DOUBLE_EQ(60, dev->value());
  EXPECT_DOUBLE_EQ(144, dev->Sample());
  EXPECT_TRUE(Interval::closed(-280, 140) == dev->interval())
      << dev->interval();
}

// Test for division of expressions.
TEST(ExpressionTest, Div) {
  OpenExpression arg_one(1, 2, 0.1, 10);
  OpenExpression arg_two(3, 4, 1, 5);
  OpenExpression arg_three(5, 6, 2, 6);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(dev = MakeUnique<Div>({&arg_one, &arg_two, &arg_three}));
  EXPECT_DOUBLE_EQ(1.0 / 3 / 5, dev->value());
  EXPECT_DOUBLE_EQ(2.0 / 4 / 6, dev->Sample());
  EXPECT_TRUE(Interval::closed(0.1 / 5 / 6, 10.0 / 1 / 2) == dev->interval())
      << dev->interval();

  arg_two.mean = 0;  // Division by 0.
  EXPECT_THROW(dev->Validate(), InvalidArgument);
}

// Test for the special case of finding maximum and minimum division.
TEST(ExpressionTest, DivisionMaxAndMin) {
  OpenExpression arg_one(1, 2, -1, 2);
  OpenExpression arg_two(3, 4, -7, -4);
  OpenExpression arg_three(5, 6, 1, 5);
  OpenExpression arg_four(4, 3, -2, 4);
  std::unique_ptr<Expression> dev;
  ASSERT_NO_THROW(
      dev = MakeUnique<Div>({&arg_one, &arg_two, &arg_three, &arg_four}));
  EXPECT_DOUBLE_EQ(1.0 / 3 / 5 / 4, dev->value());
  EXPECT_DOUBLE_EQ(2.0 / 4 / 6 / 3, dev->Sample());
  EXPECT_TRUE(Interval::closed(-1.0 / -4 / 1 / -2, 2.0 / -4 / 1 / -2) ==
              dev->interval()) << dev->interval();
}

}  // namespace test
}  // namespace mef
}  // namespace scram
