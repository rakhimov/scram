/*
 * Copyright (C) 2015 Olzhas Rakhimov
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

#include "boolean_graph.h"

#include <gtest/gtest.h>

#include "initializer.h"
#include "fault_tree.h"
#include "model.h"
#include "settings.h"

namespace scram {
namespace test {

TEST(BooleanGraphTest, Print) {
  Settings settings;
  Initializer* init = new Initializer(settings);
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/fta/correct_formulas.xml");
  EXPECT_NO_THROW(init->ProcessInputFiles(input_files));
  BooleanGraph* graph = new BooleanGraph(init->model()
                                         ->fault_trees().begin()->second
                                         ->top_events().front());
  graph->Print();
  delete init;
  delete graph;
}

}  // namespace test
}  // namespace scram
