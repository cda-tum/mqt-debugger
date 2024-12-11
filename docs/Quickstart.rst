Quickstart
==========

This documentation gives a quick overview on how to get started with MQT Debugger using:
- The Python library
- The DAP server
- The CLI app

Python Library
##############

The Python bindings are offered to give an easy start into using MQT Debugger.

Working with the Python library of MQT Debugger requires several preparation steps:

.. code-block:: python

    import mqt.debugger as dbg

All main functionalities are included in the module ``mqt.debugger``, no other modules need to be imported.

.. code-block:: python

    state = dbg.create_ddsim_simulation_state()

The first step is to create a simulation state. The current implementation of MQT Debugger implements a single simulation backend based on
decision diagrams from `MQT Core <https://github.com/cda-tum/mqt-core>`_. This backend is instantiated by calling :py:func:`create_ddsim_simulation_state <mqt.debugger.create_ddsim_simulation_state>`.

.. code-block:: python

    state.load_code(your_code)

Before running the debugger, a quantum program must be loaded into the state. This is done by calling :py:meth:`SimulationState.load_code <mqt.debugger.SimulationState.load_code>` with the quantum program as a string argument.
Currently, the supported quantum program format is `QASM 2.0 <https://arxiv.org/abs/1707.03429>`_.

After this setup is done, the debugging process can be started by stepping through the code one instruction at a time:

.. code-block:: python

    state.step_forward()

or by running the full simulation:

.. code-block:: python

    state.run_simulation()

In the second case, simulation will be run until the program ends, a failing assertion is encountered, or a breakpoint is reached.
Further details on how to step through the program can be found in the :doc:`reference documentation <library/Library>`.

Breakpoints can be set using

.. code-block:: python

    state.set_breakpoint(character_index)

where ``character_index`` is the index of the character in the original code's string, at which the breakpoint should be set.

Assertions can be added to the code following the :doc:`assertion syntax <Assertions>` and are automatically evaluated.

When an assertion fails, the diagnostics methods can be used to get more information about the failure. To this end, first access the diagnostics interface:

.. code-block:: python

    diagnostics = state.get_diagnostics()

Then, the potential error causes can be retrieved:

.. code-block:: python

    problems = diagnostics.potential_error_causes()
    print(problems)

DAP Server
##########

This library provides a DAP Server that can be connected to from existing IDEs like Visual Studio Code or CLion.

It can be started by calling

.. code-block:: console

    python3 -m mqt.debugger.dap.adapter

The server will then start on port 4711 and can accept one single connection from debugging clients.

.. note::
    Connecting to the server requires a client compatible with the `Debug Adapter Protocol <https://microsoft.github.io/debug-adapter-protocol//>`_.
    While most common IDEs already support it by default, some additional setup or extensions may be required to allow communication with arbitrary clients.

The DAP Server provides all simulation methods that are accessible via the Python library.
On assertion failures, the server will automatically pause the simulation and send a message to the client containing possible error causes.

CLI App
#######

The CLI app is a standalone application that can be used to debug quantum programs from the command line. It is mainly supposed to be used as a testing tool
and thus does not provide all the features of the framework or full accessibility through CLI parameters.

Instead, the CLI app will open a OpenQASM file with the name :code:`program.qasm` in the current working directory and start the debugging process for it.
