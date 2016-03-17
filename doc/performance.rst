######################
SCRAM Performance Data
######################

Performance data is for optimization and regression-detection purposes.
Only cases that help detect bottlenecks are recorded in this file.
The case/record is removed
if the performance becomes good enough
to make the parameter negligible or noisy
(time < 0.5s, memory consumption < 10 MiB, etc.).

There are two main performance records in this file:
run-time (in seconds) and memory utilization (in MiB).
The record can also include (specialize on) performance proxies
specific to the subject.

- Running times are taken from the logs and reports.
  The minimum of samples is recorded.
- Peak consumption is recorded for memory utilization with the ``top`` program.
- Records are for the total program or specific analysis facilities.
- Performance for the debug build is recorded
  if there's a strong correlation with the release build.
  These debug-build records help the development process
  by decreasing the need to recompile the code for performance testing.
- The build types do not affect the memory as much as the speed.
- Do not confuse these memory records
  with the actual allocation of the memory by the code.
  The actual memory allocation must be analyzed
  with memory(heap) profilers like Valgrind with Massif.

==============   ===================
System Specs     Version
==============   ===================
Processor         Core i7-2820QM
OS                Ubuntu 14.04 64bit
GCC version       4.8.4
Boost version     1.55
Libxml2           2.9.1
TCMalloc          2.1
==============   ===================

.. note:: If the current performance is not listed, then the best is the current.


Analysis
========

The records for analysis.

Input name [flags] (results)
============================

Build Type:
-----------

- Record        Best Achieved | Current Performance (in seconds)


Probability/Importance Analysis
===============================


Fault Tree Analysis (MOCUS)
===========================

Baobab1 -l 7 (mcs = 17432)
--------------------------

Debug build:  2.1

Release build:  0.30

- # of ZBDD nodes created: 34966
- # of SetNodes in ZBDD: 3377


Baobab1 (mcs = 46188)
---------------------

Debug build:  2.2

Release build: 0.30

- # of ZBDD nodes created: 34862
- # of SetNodes in ZBDD: 3333

- Memory:   23


Baobab2 (mcs = 4805)
--------------------

- # of ZBDD nodes created: 70169
- # of SetNodes in ZBDD: 167

- Memory:   15


Fault Tree Analysis (ZBDD)
==========================

Baobab1 (mcs = 46188)
---------------------

Debug build:
~~~~~~~~~~~~

- Total ZBDD time:  1.7

    * # of ZBDD nodes created: 79264  |  86228
    * # of SetNodes in ZBDD: 3338

- Memory:   48


Baobab2 (mcs = 4805)
--------------------

- # of ZBDD nodes created: 33297
- # of SetNodes in ZBDD: 168


Fault Tree Analysis (BDD)
=========================

Baobab1 (mcs = 46188)
---------------------

- # of BDD vertices created: 8289  |  8296
- # of ITE in BDD: 3349

- Memory:   23


CEA9601 -l 4 (mcs = 54436)
==========================

Debug build:
------------

- BDD Time: 11
- ZBDD Time: 1.0

Release build:
--------------

- BDD Time: 3.3
- ZBDD Time: 0.30

- # of BDD vertices created: 3013946  |  3048123
- # of ITE in BDD: 1175468

- Memory:   380


CEA9601 -l 5 (mcs = 1615876)
============================

Release build:
--------------

- BDD Time: 3.3
- ZBDD Time: 4.0  |  4.5

- Analysis total (includes release of memory, cleanup, etc.): 8.4
- Reporting (/dev/null): 5.2

- # of BDD vertices created: 3048746
- # of ITE in BDD: 1175468
- # of ZBDD vertices created: 43593
- ZBDD Cut set extraction time: 3.5

- Memory:   580


Uncertainty Analysis
====================


SCRAM Model Validation
======================

Fault tree generator flags to get the model.

-b 10000 -a 3 --common-g 0.1 --common-b 0.1
-------------------------------------------

- Memory:   70


-b 300000 -a 3 --common-g 0.1 --common-b 0.1
--------------------------------------------

Debug build:
~~~~~~~~~~~~

- Initialization and Validation    37  |  40


Release build:
~~~~~~~~~~~~~~

- Initialization and Validation    21  | 24

- Memory:   1130


Fault Tree Generator Script
===========================

-b 100000 -a 3 --common-g 0.1 --common-b 0.1
--------------------------------------------

- Generation Time  8.7  |  15
