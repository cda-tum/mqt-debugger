C Interface
===========

This section documents the interface that must be implemented for a debugger to be compatible with this framework.

The interface is written in C and can be implemented with any compatible language. We provide an :doc:`example implementation <../dd/Dd>` in C++
based on the DD simulation backend from `MQT Core <https://github.com/cda-tum/mqt-core>`_.

 .. toctree::
    :maxdepth: 4
    :caption: C-Interfaces
    :glob:

    Common
    Debug
    Diagnostics
