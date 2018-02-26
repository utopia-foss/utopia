# CITCAT

The __C__onveniently __I__ntegrated __C__ellular __A__utomaton __T__oolkit

[![build status](https://zwackelmann.iup.uni-heidelberg.de:10443/utopia/citcat/badges/master/build.svg)](https://zwackelmann.iup.uni-heidelberg.de:10443/utopia/citcat/commits/master)

Powered by [DUNE](https://dune-project.org/)

This module is the base library for the [iCAT](https://zwackelmann.iup.uni-heidelberg.de:10443/utopia/icat) package. It contains class and function templates, a documentation, and unit tests.

_CITCAT is not meant to be a stand-alone module. To build Cellular Automata models, use iCAT and place source files there._

## Installation
CITCAT is a DUNE module and thus relies on the [DUNE Buildsystem](https://www.dune-project.org/doc/installation/) for installation. Please refer to the iCAT [Installation Manual](INSTALL.md) for information on how to install CITCAT and iCAT on your machine. If all requirements are met, CITCAT is configured and built with the `dunecontrol` script:

    ./dune-common/bin/dunecontrol --module=citcat all

### Dependencies
| Software | Version | Comments |
| ---------| ------- | -------- |
| [CMake](https://cmake.org/) | >=3.0 |
| pkg-config | | |
| [dune-common](https://gitlab.dune-project.org/core/dune-common) | master |
| [dune-geometry](https://gitlab.dune-project.org/core/dune-geometry) | master |
| [dune-grid](https://gitlab.dune-project.org/core/dune-grid) | master |
| [dune-uggrid](https://gitlab.dune-project.org/staging/dune-uggrid) | master |

### Recommended
| Software | Version | Purpose |
| ---------| ------- | ------- |
| GCC | >= 5 | Supported compiler for CITCAT. If in doubt, use this one!
| [doxygen](http://www.stack.nl/~dimitri/doxygen/) | | Builds the code documentation upon installation

### Optional Packages
| Software | Version | Purpose |
| -------- | ------- | ------- |
| [PSGraf](https://zwackelmann.iup.uni-heidelberg.de:10443/tools/psgraf)| master | Data visualization on the fly |

Install PSGraf according to its installation manual. When executing `dunecontrol`, append `CMAKE_FLAGS="-DPSGRAF_ROOT=<path/to/psgraf/build>"`. If PSGraf is found, the preprocessor macro `HAVE_PSGRAF` is set.

### Building the Documentation
CITCAT builds a Doxygen documentation from its source files. Use `dunecontrol` to execute the appropriate command:

    ./dune-common/bin/dunecontrol --only=citcat make doc

You will find the files inside the build directory `citcat/build-cmake/doc`.

### DUNE Module
CITCAT can be set up as a dependency of, or a suggestion by your own DUNE Module. To do so, clone CITCAT into the directory containing the other DUNE modules. Call `duneproject` and add it to the list of dependencies (or suggestions). The entire code can be included into your program by including the CITCAT header file:

    #include <dune/citcat/citcat.hh>

### Unit Tests
**The unit testing is still incomplete!**

CITCAT contains unit tests to ensure consistency by checking if class members and functions are working correctly. The tests are integrated into the GitLab Continuous Integration build process, meaning that failing tests cause an entire build to fail.

Tests can also be executed locally, to test a (possibly altered) version of CITCAT *before* committing changes. To build them, execute

    ./dune-common/bin/dunecontrol --only=citcat make build_tests

and perform the tests by calling

    ./dune-common/bin/dunecontrol --only=citcat make test

If the test executables are not built before executing `make test`, the corresponding tests will inevitably fail.
