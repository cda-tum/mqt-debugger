[![PyPI](https://img.shields.io/pypi/v/mqt.debugger?logo=pypi&style=flat-square)](https://pypi.org/project/mqt.debugger/)
![OS](https://img.shields.io/badge/os-linux%20%7C%20macos%20%7C%20windows-blue?style=flat-square)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square)](https://opensource.org/licenses/MIT)
[![CI](https://img.shields.io/github/actions/workflow/status/cda-tum/mqt-debugger/ci.yml?branch=main&style=flat-square&logo=github&label=ci)](https://github.com/cda-tum/mqt-debugger/actions/workflows/ci.yml)
[![CD](https://img.shields.io/github/actions/workflow/status/cda-tum/mqt-debugger/cd.yml?style=flat-square&logo=github&label=cd)](https://github.com/cda-tum/mqt-debugger/actions/workflows/cd.yml)
[![Documentation](https://img.shields.io/readthedocs/debugger?logo=readthedocs&style=flat-square)](https://mqt.readthedocs.io/projects/debugger)
[![codecov](https://img.shields.io/codecov/c/github/cda-tum/mqt-debugger?style=flat-square&logo=codecov)](https://codecov.io/gh/cda-tum/mqt-debugger)

<p align="center">
  <a href="https://mqt.readthedocs.io">
   <picture>
     <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/cda-tum/mqt/main/docs/_static/mqt_light.png" width="60%">
     <img src="https://raw.githubusercontent.com/cda-tum/mqt/main/docs/_static/mqt_dark.png" width="60%">
   </picture>
  </a>
</p>

# MQT Debugger - A Quantum Circuit Debugging Tool

A tool for debugging quantum circuits developed as part of the [_Munich Quantum Toolkit (MQT)_](https://mqt.readthedocs.io) by the [Chair for Design Automation](https://www.cda.cit.tum.de/) at the [Technical University of Munich](https://www.tum.de/).
It proposes an interface for the simulation of circuits and diagnosis of errors and provides a base implementation built upon [MQT Core](https://github.com/cda-tum/mqt-core), which forms the backbone of the MQT.
It also provides a Debugger Adapter Protocol (DAP) server that can be used to integrate the debugger into IDEs.

<p align="center">
  <a href="https://mqt.readthedocs.io/projects/debugger">
  <img width=30% src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=read%20the%20docs" alt="Documentation" />
  </a>
</p>

If you have any questions, feel free to contact us via [quantum.cda@xcit.tum.de](mailto:quantum.cda@xcit.tum.de) or by creating an issue on [GitHub](https://github.com/cda-tum/mqt-debugger/issues).

## Getting Started

MQT Debugger is available via [PyPI](https://pypi.org/project/mqt.debugger/) for Linux, macOS, and Windows and supports Python 3.8 to 3.12.

```console
(venv) $ pip install mqt.debugger
```

The following code gives an example on the usage:

```python3
from mqt import debugger

state = debugger.create_ddsim_simulation_state()
with open("code.qasm", "r") as f:
    state.load_code(f.read())
f.run_simulation()
print(f.get_state_vector_full())
```

**Detailed documentation on all available methods, options, and input formats is available at [ReadTheDocs](https://mqt.readthedocs.io/projects/debugger).**

## System Requirements and Building

The implementation is compatible with any C++20 compiler, a minimum CMake version of 3.19, and Python 3.8+.
Please refer to the [documentation](https://mqt.readthedocs.io/projects/debugger) on how to build the project.

Building (and running) is continuously tested under Linux, macOS, and Windows using the [latest available system versions for GitHub Actions](https://github.com/actions/virtual-environments).

## References

MQT Debugger has been developed based on methods proposed in a scientific paper that has not been released yet.

## Acknowledgements

The Munich Quantum Toolkit has been supported by the European
Research Council (ERC) under the European Union's Horizon 2020 research and innovation program (grant agreement
No. 101001318), the Bavarian State Ministry for Science and Arts through the Distinguished Professorship Program, as well as the
Munich Quantum Valley, which is supported by the Bavarian state government with funds from the Hightech Agenda Bayern Plus.

<p align="center">
<picture>
<source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/cda-tum/mqt/main/docs/_static/tum_dark.svg" width="28%">
<img src="https://raw.githubusercontent.com/cda-tum/mqt/main/docs/_static/tum_light.svg" width="28%" alt="TUM Logo">
</picture>
<picture>
<img src="https://raw.githubusercontent.com/cda-tum/mqt/main/docs/_static/logo-bavaria.svg" width="16%" alt="Coat of Arms of Bavaria">
</picture>
<picture>
<source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/cda-tum/mqt/main/docs/_static/erc_dark.svg" width="24%">
<img src="https://raw.githubusercontent.com/cda-tum/mqt/main/docs/_static/erc_light.svg" width="24%" alt="ERC Logo">
</picture>
<picture>
<img src="https://raw.githubusercontent.com/cda-tum/mqt/main/docs/_static/logo-mqv.svg" width="28%" alt="MQV Logo">
</picture>
</p>
