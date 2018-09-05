/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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

#include "expression/extern.h"

#include <memory>

#include <catch2/catch.hpp>

#include <boost/predef.h>

#include <boost/filesystem.hpp>

#if BOOST_OS_LINUX
#include <stdlib.h>
#endif

#include "expression/constant.h"

namespace scram::mef::test {

const char kLibName[] = "scram_dummy_extern";
const char kLibRelPath[] = "build/lib/scram/scram_dummy_extern";

#if BOOST_OS_LINUX
const char kLibRelPathLinux[] = "build/lib/scram/libscram_dummy_extern.so";
#endif

TEST_CASE("ExternTest.ExternLibraryLoad", "[mef::extern_function]") {
  const std::string cwd_dir = boost::filesystem::current_path().string();
  CHECK_THROWS_AS(ExternLibrary("dummy", kLibName, "", false, false), DLError);
  CHECK_THROWS_AS(ExternLibrary("dummy", kLibName, "", false, true), DLError);
  CHECK_THROWS_AS(ExternLibrary("dummy", kLibName, "", true, true), DLError);
  CHECK_THROWS_AS(ExternLibrary("dummy", kLibRelPath, cwd_dir, false, false),
                  DLError);
  CHECK_NOTHROW(ExternLibrary("dummy", kLibRelPath, cwd_dir, false, true));
  CHECK_NOTHROW(ExternLibrary("dummy", kLibRelPath, cwd_dir, true, true));

  CHECK_THROWS_AS(ExternLibrary("d", "", "", false, false), ValidityError);
  CHECK_THROWS_AS(ExternLibrary("d", ".", "", false, false), ValidityError);
  CHECK_THROWS_AS(ExternLibrary("d", "/", "", false, false), ValidityError);
  CHECK_THROWS_AS(ExternLibrary("d", "//", "", false, false), ValidityError);
  CHECK_THROWS_AS(ExternLibrary("d", "..", "", false, false), ValidityError);
  CHECK_THROWS_AS(ExternLibrary("d", "./", "", false, false), ValidityError);
  CHECK_THROWS_AS(ExternLibrary("d", "lib/", "", false, false), ValidityError);
  CHECK_THROWS_AS(ExternLibrary("d", "lib:", "", false, false), ValidityError);

#if BOOST_OS_LINUX
  // The system search with LD_LIBRARY_PATH must be tested outside.
  CHECK_NOTHROW(
      ExternLibrary("dummy", kLibRelPathLinux, cwd_dir, false, false));
#endif
}

TEST_CASE("ExternTest.ExternLibraryGet", "[mef::extern_function]") {
  const std::string cwd_dir = boost::filesystem::current_path().string();

  std::unique_ptr<ExternLibrary> library;
  REQUIRE_NOTHROW(library = std::make_unique<ExternLibrary>(
                      "dummy", kLibRelPath, cwd_dir, false, true));

  CHECK_NOTHROW(library->get<int()>("foo"));
  CHECK_NOTHROW(library->get<double()>("bar"));
  CHECK_NOTHROW(library->get<float()>("baz"));
  CHECK_THROWS_AS(library->get<int()>("foobar"), DLError);

  CHECK(library->get<int()>("foo")() == 42);
  CHECK(library->get<double()>("bar")() == 42);
  CHECK(library->get<float()>("baz")() == 42);
}

TEST_CASE("ExternTest.ExternFunction", "[mef::extern_function]") {
  const std::string cwd_dir = boost::filesystem::current_path().string();
  ExternLibrary library("dummy", kLibRelPath, cwd_dir, false, true);

  CHECK_NOTHROW(ExternFunction<int>("extern", "foo", library));
  CHECK_NOTHROW(ExternFunction<double>("extern", "bar", library));
  CHECK_NOTHROW(ExternFunction<float>("extern", "baz", library));
  CHECK_THROWS_AS(ExternFunction<int>("extern", "foobar", library), DLError);

  CHECK(ExternFunction<int>("extern", "foo", library)() == 42);
}

TEST_CASE("ExternTest.ExternExpression", "[mef::extern_function]") {
  const std::string cwd_dir = boost::filesystem::current_path().string();
  ExternLibrary library("dummy", kLibRelPath, cwd_dir, false, true);
  ConstantExpression arg_one(42);
  ConstantExpression arg_two(13);
  ConstantExpression arg_three(-1);

  SECTION("foo") {
    ExternFunction<int> foo("dummy_foo", "foo", library);
    CHECK_THROWS_AS(ExternExpression(&foo, {&arg_one}), ValidityError);
    ExternExpression expr(&foo, {});
    CHECK(expr.value() == 42);
    CHECK(expr.Sample() == 42);
    CHECK_FALSE(expr.IsDeviate());
  }

  SECTION("identity") {
    ExternFunction<double, double> identity("dummy_id", "identity", library);
    CHECK_THROWS_AS(ExternExpression(&identity, {}), ValidityError);
    ExternExpression expr(&identity, {&arg_one});
    CHECK(expr.value() == arg_one.value());
    CHECK(expr.Sample() == arg_one.Sample());
    CHECK_FALSE(expr.IsDeviate());
  }

  SECTION("sum") {
    ExternFunction<double, double, double, double> sum("dummy_id", "sum",
                                                       library);
    CHECK_THROWS_AS(ExternExpression(&sum, {}), ValidityError);
    CHECK_THROWS_AS(ExternExpression(&sum, {&arg_one}), ValidityError);
    CHECK_THROWS_AS(ExternExpression(&sum, {&arg_one, &arg_two}),
                    ValidityError);
    ExternExpression expr(&sum, {&arg_one, &arg_two, &arg_three});
    CHECK(expr.value() == 54);
    CHECK(expr.Sample() == 54);
    CHECK_FALSE(expr.IsDeviate());
  }

  SECTION("div") {
    ExternFunction<double, double, double> div("dummy_id", "div", library);
    CHECK_THROWS_AS(ExternExpression(&div, {&arg_one}), ValidityError);
    ExternExpression expr(&div, {&arg_one, &arg_two});
    double result = arg_one.value() / arg_two.value();
    CHECK(expr.value() == result);
    CHECK(expr.Sample() == result);
  }

  SECTION("subtract") {
    ExternFunction<double, double, double> sub("dummy_id", "sub", library);
    CHECK_THROWS_AS(ExternExpression(&sub, {&arg_one}), ValidityError);
    ExternExpression expr(&sub, {&arg_one, &arg_two});
    double result = arg_one.value() - arg_two.value();
    CHECK(expr.value() == result);
    CHECK(expr.Sample() == result);
  }
}

TEST_CASE("ExternTest.ExternFunctionApply", "[mef::extern_function]") {
  const std::string cwd_dir = boost::filesystem::current_path().string();
  ExternLibrary library("dummy", kLibRelPath, cwd_dir, false, true);
  ExternFunctionPtr foo(new ExternFunction<int>("dummy_foo", "foo", library));
  ExternFunctionPtr identity(
      new ExternFunction<double, double>("dummy_id", "identity", library));
  ConstantExpression arg_one(12);

  CHECK_NOTHROW(foo->apply({}));
  CHECK(foo->apply({})->value() == 42);

  CHECK_THROWS_AS(identity->apply({}), ValidityError);
  CHECK_NOTHROW(identity->apply({&arg_one}));
  CHECK(identity->apply({&arg_one})->value() == arg_one.value());
}

}  // namespace scram::mef::test
