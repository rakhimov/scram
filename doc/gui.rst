#############
GUI Front-End
#############

The GUI front-end is developed with `Qt5 and Qt Creator`_.

.. _Qt5 and Qt Creator: https://www.qt.io/developers/


GUI Concerns
============

- Construction and visualization of risk analysis models
  (fault trees, event trees, etc.).

- Configuration of the analysis.

- Delegation of the analysis to the core.

- Presentation of the analysis results.


Design Goals/Principles
=======================

- The GUI design relates to the user,
  not the system architecture.

- The GUI must be so intuitive, discoverable, and transparent
  that developers can get away with minimal or no documentation of the GUI.

- If the GUI tutorial is needed,
  the design is failure.

- If screen-shots are needed to discover a feature,
  the design is failure.


Design Constraints
==================

- Loose coupling with the SCRAM core

- Functionality

- Efficiency and Performance (Soft = 50ms)

- Continuous validation and verification.
  Guaranteed validity upon model construction
  so that there is no need for delayed validation.


Design Assumptions
==================

- Visualization and construction of analysis models
  are meant for human consumption and analysis,
  not for demonstration of how complex/complicated/large the system is.

- Risk analysis models (e.g. fault trees) of engineering systems
  have natural modularity;
  the system can be described by an interrelated/interconnected collection of components.

- A monolithic description of a component with a large number of elements (e.g. 100 or more)
  is practically useless for human consumption, unmaintainable,
  and error-prone, hiding the modeling errors.

- A myriad (10 or more) of failure scenarios describing the model
  is practically incomprehensible and useless for the analyst.

- Qualitative and quantitative metrics (e.g. total probability, # of scenarios, etc.)
  together with data analysis and visualization help the analyst
  filter out the most important/relevant information to focus and act upon.

- The analyst is computer-literate
  and knowledgeable of risk analysis tools and techniques.


GUI Features
============

- Full screen mode
- Dynamic/Interactive
- Scrolling/Dragging/Magnification
- Printing capability
- Find and replace
- Warning pane
- Status line
- Multi-level undo
- Hot keys/Shortcuts
- Auto-Save
- Separate editors for analysis components
- Convenient, effective view of analysis results
- Context menu
- Smart defaults
- Revision control


Human Interface Guidelines
==========================

- `KDE HIG <https://techbase.kde.org/Projects/Usability/HIG/>`_
- `GNOME HIG <https://developer.gnome.org/hig/stable/>`_
- `XFCE HIG <https://wiki.xfce.org/dev/hig/general>`_
- `The basic concepts of UI`_

.. _The basic concepts of UI: https://www.usability.gov/what-and-why/user-interface-design.html
