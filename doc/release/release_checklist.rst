#################
Release Checklist
#################

.. note::
    This checklist is for release actions.
    A copy of this file must be used for actual marking
    without submitting it to the version control system.

.. note:: '[-]' is used to indicate currently failing or unachieveable step.


Pre-Release
===========

- [ ] Review compiler warnings
- [ ] Cyclomatic complexity check (target: 15) (tools: Lizard)
- [ ] Additional tests (coverage target: 95%):

    * [ ] Unit tests
    * [ ] Input tests
    * [ ] Integration tests
    * [ ] Benchmarking tests
    * [ ] Performance tests

- [ ] Fuzz testing preprocessor and analysis code (tools: fuzz_tester.py)
- [ ] CodeCov (C++/Python) >= 95%
- [ ] Coverity defects < 0.35 per 1000 SLOC (target: 0)
- [ ] Static code analysis with CppCheck
- [ ] Valgrind check for memory leaks

    * [ ] Run all *fast* tests under Valgrind memcheck

- [ ] Check compatibility with Python2 and Python3 (tools: pylint)
- [ ] Update documentation (RST files)

    * [ ] Grammar check the documentation (optional)

- [ ] Documentation coverage of the code (coverage target: full) (tools: Doxygen)
- [ ] Cleanup the code

    * [ ] Unused ``#include`` and improper forward declarations (CppClean)
    * [ ] Style check of C++ code (CppLint)
    * [ ] Style check of Python code (PyLint, Prospector)
    * [ ] Unused ``import``
    * [ ] Spell check the code (optional)


Release
=======

- [ ] Bump the version

    * [ ] Add release notes into ``doc/release``

    * [ ] Update version numbers

        + [ ] CMakeLists.txt
        + [ ] scram.man

    * Commit the release with *SCRAM x.y.z* (x, y, z being version numbers)

- [ ] Release with GitHub automatic tagging with the release notes

    * [ ] Close the milestone on GitHub
    * [ ] Fetch the tags

- [ ] Rebase Master on Develop (Avoid merging)

- [ ] Update the website

    * [ ] Lizard CCN report
    * [ ] cppdep report
    * [ ] Sitemap

- [ ] Publish the release notes on the scram-announce mailing list


Distribution
============

- [ ] Update Alioth/PPA (Debian/Ubuntu)
- [ ] Installation package (NSIS/Windows)
- [ ] Homebrew
