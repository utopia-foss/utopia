# Utopia

__Utopia__ (gr. no-place), a non-existent society described in considerable detail. [Wikipedia, 2016]

Powered by [DUNE](https://dune-project.org/)

[![pipeline status](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/badges/master/pipeline.svg)](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/commits/master)

## Installation
Utopia is a DUNE module and thus relies on the [DUNE Buildsystem](https://www.dune-project.org/doc/installation/) for installation. If all requirements are met, Utopia is configured and built with the `dunecontrol` script:

    ./dune-common/bin/dunecontrol --module=utopia all

### Dependencies
| Software | Version | Comments |
| ---------| ------- | -------- |
| GCC | >= 7 | Full C++17 compiler support required |
| [CMake](https://cmake.org/) | >= 3.1 | |
| pkg-config | | |
| [dune-common](https://gitlab.dune-project.org/core/dune-common) | master | |
| [dune-geometry](https://gitlab.dune-project.org/core/dune-geometry) | master | |
| [dune-grid](https://gitlab.dune-project.org/core/dune-grid) | master | |
| [dune-uggrid](https://gitlab.dune-project.org/staging/dune-uggrid) | master | |

### Recommended
| Software | Version | Purpose |
| ---------| ------- | ------- |
| [doxygen](http://www.stack.nl/~dimitri/doxygen/) | | Builds the code documentation upon installation |

### Optional Packages
| Software | Version | Purpose |
| -------- | ------- | ------- |
| [PSGraf](https://zwackelmann.iup.uni-heidelberg.de:10443/tools/psgraf)| master | Data visualization on the fly |

Install PSGraf according to its installation manual. When executing `dunecontrol`, append `CMAKE_FLAGS="-DPSGRAF_ROOT=<path/to/psgraf/build>"`. If PSGraf is found, the preprocessor macro `HAVE_PSGRAF` is set.

### Building the Documentation
Utopia builds a Doxygen documentation from its source files. Use `dunecontrol` to execute the appropriate command:

    ./dune-common/bin/dunecontrol --only=utopia make doc

You will find the files inside the build directory `utopia/build-cmake/doc`.

### Unit Tests
**The unit testing is still incomplete!**

Utopia contains unit tests to ensure consistency by checking if class members and functions are working correctly. The tests are integrated into the GitLab Continuous Integration build process, meaning that failing tests cause an entire build to fail.

Tests can also be executed locally, to test a (possibly altered) version of Utopia *before* committing changes. To build them, execute

    ./dune-common/bin/dunecontrol --only=utopia make build_tests

and perform the tests by calling

    ./dune-common/bin/dunecontrol --only=utopia make test

If the test executables are not built before executing `make test`, the corresponding tests will inevitably fail.
