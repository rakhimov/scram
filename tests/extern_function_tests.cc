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

#include <gtest/gtest.h>

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

TEST(ExternTest, ExternLibraryLoad) {
  const std::string cwd_dir = boost::filesystem::current_path().string();
  EXPECT_THROW(ExternLibrary("dummy", kLibName, "", false, false), DLError);
  EXPECT_THROW(ExternLibrary("dummy", kLibName, "", false, true), DLError);
  EXPECT_THROW(ExternLibrary("dummy", kLibName, "", true, true), DLError);
  EXPECT_THROW(ExternLibrary("dummy", kLibRelPath, cwd_dir, false, false),
               DLError);
  EXPECT_NO_THROW(ExternLibrary("dummy", kLibRelPath, cwd_dir, false, true));
  EXPECT_NO_THROW(ExternLibrary("dummy", kLibRelPath, cwd_dir, true, true));

  EXPECT_THROW(ExternLibrary("d", "", "", false, false), ValidityError);
  EXPECT_THROW(ExternLibrary("d", ".", "", false, false), ValidityError);
  EXPECT_THROW(ExternLibrary("d", "/", "", false, false), ValidityError);
  EXPECT_THROW(ExternLibrary("d", "//", "", false, false), ValidityError);
  EXPECT_THROW(ExternLibrary("d", "..", "", false, false), ValidityError);
  EXPECT_THROW(ExternLibrary("d", "./", "", false, false), ValidityError);
  EXPECT_THROW(ExternLibrary("d", "lib/", "", false, false), ValidityError);
  EXPECT_THROW(ExternLibrary("d", "lib:", "", false, false), ValidityError);

#if BOOST_OS_LINUX
  // The system search with LD_LIBRARY_PATH must be tested outside.
  EXPECT_NO_THROW(
      ExternLibrary("dummy", kLibRelPathLinux, cwd_dir, false, false));
#endif
}

TEST(ExternTest, ExternLibraryGet) {
  const std::string cwd_dir = boost::filesystem::current_path().string();

  std::unique_ptr<ExternLibrary> library;
  ASSERT_NO_THROW(library = std::make_unique<ExternLibrary>(
                      "dummy", kLibRelPath, cwd_dir, false, true));

  EXPECT_NO_THROW(library->get<int()>("foo"));
  EXPECT_NO_THROW(library->get<double()>("bar"));
  EXPECT_NO_THROW(library->get<float()>("baz"));
  EXPECT_THROW(library->get<int()>("foobar"), UndefinedElement);

  EXPECT_EQ(42, library->get<int()>("foo")());
  EXPECT_EQ(42, library->get<double()>("bar")());
  EXPECT_EQ(42, library->get<float()>("baz")());
}

TEST(ExternTest, ExternFunction) {
  const std::string cwd_dir = boost::filesystem::current_path().string();
  ExternLibrary library("dummy", kLibRelPath, cwd_dir, false, true);

  EXPECT_NO_THROW(ExternFunction<int>("extern", "foo", library));
  EXPECT_NO_THROW(ExternFunction<double>("extern", "bar", library));
  EXPECT_NO_THROW(ExternFunction<float>("extern", "baz", library));
  EXPECT_THROW(ExternFunction<int>("extern", "foobar", library),
               UndefinedElement);

  EXPECT_EQ(42, ExternFunction<int>("extern", "foo", library)());
}

TEST(ExternTest, ExternExpression) {
  const std::string cwd_dir = boost::filesystem::current_path().string();
  ExternLibrary library("dummy", kLibRelPath, cwd_dir, false, true);
  ExternFunction<int> foo("dummy_foo", "foo", library);
  ExternFunction<double, double> identity("dummy_id", "identity", library);
  ConstantExpression arg_one(12);

  EXPECT_NO_THROW(ExternExpression<int>(&foo, {}));
  EXPECT_THROW(ExternExpression<int>(&foo, {&arg_one}), ValidityError);

  EXPECT_EQ(42, ExternExpression<int>(&foo, {}).value());
  EXPECT_EQ(42, ExternExpression<int>(&foo, {}).Sample());
  EXPECT_FALSE(ExternExpression<int>(&foo, {}).IsDeviate());

  EXPECT_NO_THROW((ExternExpression<double, double>(&identity, {&arg_one})));
  EXPECT_THROW((ExternExpression<double, double>(&identity, {})),
               ValidityError);
  EXPECT_EQ(arg_one.value(),
            (ExternExpression<double, double>(&identity, {&arg_one})).value());
}

TEST(ExternTest, ExternFunctionApply) {
  const std::string cwd_dir = boost::filesystem::current_path().string();
  ExternLibrary library("dummy", kLibRelPath, cwd_dir, false, true);
  ExternFunctionPtr foo(new ExternFunction<int>("dummy_foo", "foo", library));
  ExternFunctionPtr identity(
      new ExternFunction<double, double>("dummy_id", "identity", library));
  ConstantExpression arg_one(12);

  EXPECT_NO_THROW(foo->apply({}));
  EXPECT_EQ(42, foo->apply({})->value());

  EXPECT_THROW(identity->apply({}), ValidityError);
  EXPECT_NO_THROW(identity->apply({&arg_one}));
  EXPECT_EQ(arg_one.value(), identity->apply({&arg_one})->value());
}

}  // namespace scram::mef::test
