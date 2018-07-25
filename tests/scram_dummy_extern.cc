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

/// @file
/// Dummy library with extern functions to test dynamic loading in MEF extern.

#pragma GCC diagnostic ignored "-Wmissing-declarations"

extern "C" {
int foo() { return 42; }
double bar() { return 42; }
float baz() { return 42; }
double identity(double arg) { return arg; }
double div(double lhs, double rhs) { return lhs / rhs; }
double sum(double arg1, double arg2, double arg3) { return arg1 + arg2 + arg3; }
double sub(double lhs, double rhs) { return lhs - rhs; }
}
