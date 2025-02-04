Welcome to MQT Debugger's documentation!
========================================

MQT Debugger is a comprehensive framework for debugging and analyzing the behaviour of quantum programs.  It is developed by the `Chair for Design Automation <https://www.cda.cit.tum.de/>`_ at the `Technical University of Munich <https://www.tum.de>`_ as part of the :doc:`Munich Quantum Toolkit <mqt:index>` (*MQT*).

The framework provides tools for running quantum programs in a simulated environment, stepping through the code instruction-by-instruction, and displaying the quantum state at each step.
It also allows developers to include assertions in their code, testing the correctness of provided programs, and, when an assertion fails, suggesting possible causes of the failure.

MQT Debugger is accessible as a stand-alone application, as a C++ and Python library to include in custom programs, and as a DAP server that can be accessed by popular IDEs such as Visual Studio Code and CLion.

We recommend you to start with the :doc:`installation instructions <Installation>`.
Then proceed to the :doc:`quickstart guide <Quickstart>` and read the :doc:`reference documentation <library/Library>`.
If you are interested in the theory behind MQT Debugger, have a look at the publications in the :doc:`publication list <Publications>`.

We appreciate any feedback and contributions to the project. If you want to contribute, you can find more information in the :doc:`Contribution <Contributing>` guide. If you are having trouble with the installation or the usage of MQT Debugger, please let us know at our :doc:`Support <Support>` page.

----

 .. toctree::
    :hidden:

    self

 .. toctree::
    :maxdepth: 2
    :caption: User Guide
    :glob:

    Installation
    Quickstart
    Debugging
    Assertions
    Diagnosis
    AssertionRefinement
    Publications

 .. toctree::
    :maxdepth: 2
    :caption: Developers
    :glob:

    Contributing
    DevelopmentGuide
    Support

 .. toctree::
    :maxdepth: 6
    :caption: API Reference
    :glob:

    library/Library
