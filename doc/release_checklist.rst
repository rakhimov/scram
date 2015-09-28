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

- [ ] Coveralls >= 95%
- [ ] Coverity defects < 0.35 per 1000 SLOC (target: 0)
- [ ] Valgrind check for memory leaks

    * [ ] Run all *fast* tests under Valgrind memcheck

- [ ] Check compatibility with Python2 and Python3 (tools: Ninja-IDE)
- [ ] Update documentation (RST files)

    * [ ] Grammar check the documentation (optional)

- [ ] Documentation coverage of the code (coverage target: full) (tools: Doxygen)
- [ ] Cleanup the code

    * [ ] Style check of C++ code (CppLint)
    * [ ] Style check of Python code (PyLint)
    * [ ] Spell check the code (optional)
    * [-] CppClean check (problem: fails with C++11)

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
