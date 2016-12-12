#!/usr/bin/env python

"""Installation script for SCRAM.

This script automates the build and installation processes
for various purposes like release and debug.
"""

import os
import shutil
import subprocess
import sys

import argparse as ap


def absexpanduser(rel_path):
    """Expands a given relative path into an absolute one.

    Args:
        rel_path: A relative path.

    Returns:
        An expanded path.
    """
    return os.path.abspath(os.path.expanduser(rel_path))


def generate_make_files(args):
    """Generates make files by calling CMake.

    The process may exit abruptly with an error message.

    Args:
        args: Configurations for the installation process.
    """
    root_dir = os.path.split(__file__)[0]
    makefile = os.path.join(args.build_dir, "Makefile")
    if os.path.exists(makefile):
        return
    rtn = subprocess.call(["which", "cmake"], shell=(os.name == "nt"))
    if rtn != 0:
        sys.exit("CMake could not be found, "
                 "please install CMake before developing SCRAM.")
    cmake_cmd = ["cmake", os.path.abspath(root_dir)]
    cmake_cmd += ["-DBUILD_SHARED_LIBS=ON"]
    if args.prefix:
        cmake_cmd += ["-DCMAKE_INSTALL_PREFIX=" + absexpanduser(args.prefix)]

    if args.build_type:
        cmake_cmd += ["-DCMAKE_BUILD_TYPE=" + args.build_type]
    elif args.release:
        cmake_cmd += ["-DCMAKE_BUILD_TYPE=Release"]
    else:
        cmake_cmd += ["-DCMAKE_BUILD_TYPE=Debug"]
        cmake_cmd += ["-Wdev"]

    if args.profile:
        cmake_cmd += ["-DWITH_PROFILE=ON"]
    if args.coverage:
        cmake_cmd += ["-DWITH_COVERAGE=ON"]

    if args.D is not None:
        cmake_cmd += ["-D" + x for x in args.D]

    if args.mingw64:
        cmake_cmd += ["-G", "MSYS Makefiles"]

    subprocess.check_call(cmake_cmd, cwd=args.build_dir,
                          shell=(os.name == "nt"))


def install_scram(args):
    """Installs SCRAM with the specified configurations.

    The process may exit abruptly with an error message.

    Args:
        args: Configurations for the installation process.
    """
    if not os.path.exists(args.build_dir):
        os.mkdir(args.build_dir)
    elif args.clean_build:
        shutil.rmtree(args.build_dir)
        os.mkdir(args.build_dir)

    generate_make_files(args)

    make_cmd = ["make"]
    if args.threads:
        make_cmd += ["-j" + str(args.threads)]
    subprocess.check_call(make_cmd, cwd=args.build_dir, shell=(os.name == "nt"))

    if args.test:
        make_cmd += ["test"]
    elif not args.build_only:
        make_cmd += ["install"]
    subprocess.check_call(make_cmd, cwd=args.build_dir, shell=(os.name == "nt"))


def uninstall_scram(args):
    """Uninstalls SCRAM if it is installed.

    The process may exit abruptly with an error message.

    Args:
        args: Configurations used in the installation process.
    """
    makefile = os.path.join(args.build_dir, "Makefile")
    if not os.path.exists(args.build_dir) or not os.path.exists(makefile):
        sys.exit("May not uninstall scram since it has not yet been built.")
    subprocess.check_call(["make", "uninstall"], cwd=args.build_dir,
                          shell=(os.name == "nt"))


def main():
    """Initiates installation processes taking command-line arguments."""
    parser = ap.ArgumentParser(description="SCRAM installation script.")
    parser.add_argument("--build_dir", help="path for the build directory",
                        default="build")
    parser.add_argument("--uninstall", action="store_true",
                        help="remove the previous installation", default=False)
    clean = "attempt to remove the build directory before building"
    parser.add_argument("--clean-build", action="store_true", help=clean)
    parser.add_argument("-j", "--threads", type=int,
                        help="the number of threads to use in the make step")
    parser.add_argument("--prefix", help="path to the installation directory",
                        default=absexpanduser("~/.local"))
    parser.add_argument("--build-only", action="store_true",
                        help="only build the package, do not install")
    parser.add_argument("--test", action="store_true",
                        help="run tests after building")
    parser.add_argument("--build-type", help="the CMAKE_BUILD_TYPE")
    parser.add_argument("--debug", help="build for debugging (default)",
                        action="store_true", default=False)
    parser.add_argument("--release",
                        help="build for release with optimizations",
                        action="store_true", default=False)
    parser.add_argument("--profile", help="build with profiling",
                        action="store_true", default=False)
    parser.add_argument("--coverage", help="build with coverage",
                        action="store_true", default=False)
    parser.add_argument("-D", metavar="VAR", action="append",
                        help="pass environment variable(s) to CMake")
    parser.add_argument("--mingw64",
                        help="building on MSYS2 with mingw64-x86_64",
                        action="store_true", default=False)

    args = parser.parse_args()
    if args.uninstall:
        uninstall_scram(args)
    else:
        install_scram(args)

if __name__ == "__main__":
    main()
