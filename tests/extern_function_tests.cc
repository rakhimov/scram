/*
 * Copyright (C) 2017 Olzhas Rakhimov
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

#if BOOST_OS_LINUX
#include <stdlib.h>
#endif

#include "env.h"

namespace scram {
namespace mef {
namespace test {

const char kLibName[] = "scram_dummy_extern";
const char kLibRelPath[] = "../lib/scram/test/scram_dummy_extern";

#if BOOST_OS_LINUX
const char kLibRelPathLinux[] = "../lib/scram/test/libscram_dummy_extern.so";
#endif

TEST(ExternTest, ExternLibraryLoad) {
  const std::string bin_dir = Env::install_dir() + "/bin";
  EXPECT_THROW(ExternLibrary("dummy", kLibName, "", false, false), IOError);
  EXPECT_THROW(ExternLibrary("dummy", kLibName, "", false, true), IOError);
  EXPECT_THROW(ExternLibrary("dummy", kLibName, "", true, true), IOError);
  EXPECT_THROW(ExternLibrary("dummy", kLibRelPath, bin_dir, false, false),
               IOError);
  EXPECT_NO_THROW(ExternLibrary("dummy", kLibRelPath, bin_dir, false, true));
  EXPECT_NO_THROW(ExternLibrary("dummy", kLibRelPath, bin_dir, true, true));

#if BOOST_OS_LINUX
  EXPECT_NO_THROW(
      ExternLibrary("dummy", kLibRelPathLinux, bin_dir, false, false));
  // The system search with LD_LIBRARY_PATH must be tested outside.
#endif
}

TEST(ExternTest, ExternLibraryGet) {
  const std::string bin_dir = Env::install_dir() + "/bin";

  std::unique_ptr<ExternLibrary> library;
  ASSERT_NO_THROW(library = std::make_unique<ExternLibrary>(
                      "dummy", kLibRelPath, bin_dir, false, true));

  EXPECT_NO_THROW(library->get<int(*)()>("foo"));
  EXPECT_NO_THROW(library->get<double(*)()>("bar"));
  EXPECT_NO_THROW(library->get<float(*)()>("baz"));
  EXPECT_THROW(library->get<int(*)()>("foobar"), UndefinedElement);

  EXPECT_EQ(42, library->get<int(*)()>("foo")());
  EXPECT_EQ(42, library->get<double(*)()>("bar")());
  EXPECT_EQ(42, library->get<float(*)()>("baz")());
}

TEST(ExternTest, ExternFunction) {
  const std::string bin_dir = Env::install_dir() + "/bin";
  std::unique_ptr<ExternLibrary> library;
  ASSERT_NO_THROW(library = std::make_unique<ExternLibrary>(
                      "dummy", kLibRelPath, bin_dir, false, true));

  EXPECT_NO_THROW(ExternFunction<int>("extern", "foo", *library));
  EXPECT_NO_THROW(ExternFunction<double>("extern", "bar", *library));
  EXPECT_NO_THROW(ExternFunction<float>("extern", "baz", *library));
  EXPECT_THROW(ExternFunction<int>("extern", "foobar", *library),
               UndefinedElement);

  EXPECT_EQ(42, ExternFunction<int>("extern", "foo", *library)());
}

}  // namespace test
}  // namespace mef
}  // namespace scram
