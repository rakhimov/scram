#include <gtest/gtest.h>

#include "expression.h"

#include "error.h"

using namespace scram;

// This mock class is used to specify return
// values and samples in a hard coded way.
class OpenExpression : public Expression {
 public:
  OpenExpression(double m = 1, double s = 1) : mean(m), sample(s) {}
  double mean;
  double sample;
  inline double Mean() { return mean; }
  inline double Sample() { return sample; }
  void Validate() {}
};

typedef boost::shared_ptr<OpenExpression> OpenExpressionPtr;

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

  ASSERT_NO_THROW(dev->Sample());
  lambda->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  lambda->sample = 10;
  ASSERT_NO_THROW(dev->Sample());

  time->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  time->sample = 5;
  ASSERT_NO_THROW(dev->Sample());
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

  ASSERT_NO_THROW(dev->Sample());
  gamma->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  gamma->sample = 10;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  gamma->sample = 0.10;
  ASSERT_NO_THROW(dev->Sample());

  lambda->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  lambda->sample = 10;
  ASSERT_NO_THROW(dev->Sample());

  mu->sample = -10;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  mu->sample = 100;
  ASSERT_NO_THROW(dev->Sample());

  time->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  time->sample = 5;
  ASSERT_NO_THROW(dev->Sample());
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

  ASSERT_NO_THROW(dev->Sample());
  alpha->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  alpha->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  alpha->sample = 0.10;
  ASSERT_NO_THROW(dev->Sample());

  beta->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  beta->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  beta->sample = 10;
  ASSERT_NO_THROW(dev->Sample());

  t0->sample = -10;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  t0->sample = 100;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  t0->sample = 10;
  ASSERT_NO_THROW(dev->Sample());

  time->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  time->sample = 50;
  ASSERT_NO_THROW(dev->Sample());
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

  ASSERT_NO_THROW(dev->Sample());
  min->sample = 10;
  EXPECT_THROW(dev->Sample(), InvalidArgument);  // min > max
  min->sample = 1;
  ASSERT_NO_THROW(dev->Sample());
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

  ASSERT_NO_THROW(dev->Sample());
  sigma->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);  // sigma < 0
  sigma->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);  // sigma = 0
  sigma->sample = 1;
  EXPECT_NO_THROW(dev->Sample());
}

// Log-Normal deviate test for invalid mean, error factor, and level.
TEST(ExpressionTest, LogNormalDeviate) {
  OpenExpressionPtr mean(new OpenExpression(10, 5));
  OpenExpressionPtr ef(new OpenExpression(5, 3));
  OpenExpressionPtr level(new OpenExpression(0.95));
  ExpressionPtr dev;
  ASSERT_NO_THROW(dev = ExpressionPtr(new LogNormalDeviate(mean, ef, level)));

  level->mean = 0.5; // Unsupported level.
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
  ef->mean = 0;  // ef = 0
  EXPECT_THROW(dev->Validate(), InvalidArgument);
  ef->mean = 1;
  ASSERT_NO_THROW(dev->Validate());

  ASSERT_NO_THROW(dev->Sample());
  mean->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  mean->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  mean->sample = 5;
  ASSERT_NO_THROW(dev->Sample());
  ef->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  ef->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  ef->sample = 3;
  ASSERT_NO_THROW(dev->Sample());
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

  ASSERT_NO_THROW(dev->Sample());
  k->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  k->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  k->sample = 1;
  ASSERT_NO_THROW(dev->Sample());

  theta->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  theta->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  theta->sample = 1;
  ASSERT_NO_THROW(dev->Sample());
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

  ASSERT_NO_THROW(dev->Sample());
  alpha->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  alpha->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  alpha->sample = 1;
  ASSERT_NO_THROW(dev->Sample());

  beta->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  beta->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  beta->sample = 1;
  ASSERT_NO_THROW(dev->Sample());
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

  ASSERT_NO_THROW(dev->Sample());
  b1->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  b1->sample = 0;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  b1->sample = b2->sample;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  b1->sample = b2->sample + 1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  b1->sample = 1;
  ASSERT_NO_THROW(dev->Sample());

  w1->sample = -1;
  EXPECT_THROW(dev->Sample(), InvalidArgument);
  w1->sample = 2;
  ASSERT_NO_THROW(dev->Sample());
}
