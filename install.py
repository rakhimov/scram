#!/usr/bin/env python
"""install.py

An installation script for SCRAM. This script automates the building and
installation processes for various purposes like release and debug.
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

    root_dir = os.path.split(__file__)[0]
    makefile = os.path.join(args.build_dir, "Makefile")

    if not os.path.exists(makefile):
        rtn = subprocess.call(["which", "cmake"], shell=(os.name == "nt"))
        if rtn != 0:
            sys.exit("CMake could not be found, "
                     "please install CMake before developing SCRAM.")
        cmake_cmd = ["cmake", os.path.abspath(root_dir)]
        if args.prefix:
            cmake_cmd += ["-DCMAKE_INSTALL_PREFIX=" +
                          absexpanduser(args.prefix)]

        if args.build_type:
            cmake_cmd += ['-DCMAKE_BUILD_TYPE=' + args.build_type]
        elif args.release:
            cmake_cmd += ["-DCMAKE_BUILD_TYPE=Release"]
        elif args.debug:
            cmake_cmd += ["-DCMAKE_BUILD_TYPE=Debug"]
        elif args.profile:
            cmake_cmd += ["-DCMAKE_BUILD_TYPE=Debug"]
            cmake_cmd += ["-DCMAKE_C_FLAGS=-pg"]
            cmake_cmd += ["-DCMAKE_CXX_FLAGS=-pg"]
            cmake_cmd += ["-DCMAKE_CXX_FLAGS='-Wall -fprofile-arcs "
                          "-ftest-coverage'"]
        else:
            cmake_cmd += ["-DCMAKE_BUILD_TYPE=Debug"]  # default build type

        subprocess.check_call(cmake_cmd, cwd=args.build_dir,
                              shell=(os.name == 'nt'))

    make_cmd = ["make"]
    if args.threads:
        make_cmd += ["-j" + str(args.threads)]

    subprocess.check_call(make_cmd, cwd=args.build_dir, shell=(os.name == "nt"))

    if args.test:
        make_cmd += ["test"]
    elif not args.build_only:
        make_cmd += ["install"]

    subprocess.check_call(make_cmd, cwd=args.build_dir,
                          shell=(os.name == "nt"))

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
    localdir = absexpanduser("~/.local")

    description = "A SCRAM installation helper script. " +\
                  "For more information, please see " +\
                  "rakhimov.github.io/scram"
    parser = ap.ArgumentParser(description=description)

    build_dir = "where to place the build directory"
    parser.add_argument("--build_dir", help=build_dir, default="build")

    uninst = "uninstall"
    parser.add_argument("--uninstall", action="store_true", help=uninst,
                        default=False)

    clean = "attempt to remove the build directory before building"
    parser.add_argument("--clean-build", action="store_true", help=clean)

    threads = "the number of threads to use in the make step"
    parser.add_argument("-j", "--threads", type=int, help=threads)

    prefix = "the relative path to the installation directory"
    parser.add_argument("--prefix", help=prefix, default=localdir)

    build_only = "only build the package, do not install"
    parser.add_argument("--build-only", action="store_true", help=build_only)

    test = "run tests after building"
    parser.add_argument("--test", action="store_true", help=test)

    build_type = "the CMAKE_BUILD_TYPE"
    parser.add_argument('--build-type', help=build_type)

    debug = "build for debugging"
    parser.add_argument("-d", "--debug", help=debug, action="store_true",
                        default=False)

    profile = "build for profiling"
    parser.add_argument("-p", "--profile", help=profile, action="store_true",
                        default=False)

    release = "build for release with optimizations"
    parser.add_argument("-r", "--release", help=release, action="store_true",
                        default=False)

    args = parser.parse_args()
    if args.uninstall:
        uninstall_scram(args)
    else:
        install_scram(args)

if __name__ == "__main__":
    main()
