#################
Release Checklist
#################

.. note::
    This checklist is for release actions.
    A copy of this file must be used for actual marking
    without submitting it to version control system.

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

- [ ] Fuzz testing preprocessor and analysis code (tools: fuzz_tester.py, fault_tree_generator.py)
- [ ] Coveralls >= 95%
- [ ] Coverity defects < 0.35 per 1000 SLOC (target: 0)
- [ ] Static code analysis with CppCheck
- [ ] Valgrind check for memory leaks

    * [ ] Run all *fast* tests under Valgrind memcheck

- [ ] Check compatibility with Python2 and Python3 (tools: Ninja-IDE)
- [ ] Update documentation (RST files)

    * [ ] Grammar check the documentation (optional)

- [ ] Documentation coverage of the code (coverage target: full) (tools: Doxygen)
- [ ] Cleanup the code

    * [ ] Unused ``#include`` and improper forward declarations (CppClean)
    * [ ] Style check of C++ code (CppLint)
    * [ ] Style check of Python code (PyLint, Prospector)
    * [ ] Unused ``import``
    * [ ] Spell check the code (optional)

- [ ] Update the Lizard report


Release
=======

- [ ] Update version number in code version.h
- [ ] Update version numbers on the gh-source branch (file: conf.py)
- [ ] Update the website

    * [ ] Sitemap

- [ ] Update release notes on GitHub
- [ ] Release with GitHub automatic tagging
- [ ] Rebase Master on Develop (Avoid merging)
- [ ] Close the milestone on GitHub


Distribution
============

- [ ] Update PPA (Ubuntu)
