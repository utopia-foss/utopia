# Utopia

__Utopia__ (gr. no-place), a non-existent society described in considerable detail. [Wikipedia, 2016]

Powered by [DUNE](https://dune-project.org/)

[![pipeline status](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/badges/master/pipeline.svg)](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/commits/master)

## Installation
Utopia is a DUNE module and thus relies on the [DUNE Buildsystem](https://www.dune-project.org/doc/installation/) for installation. If all requirements are met, Utopia is configured and built with the `dunecontrol` script:

    ./dune-common/bin/dunecontrol --module=utopia all

Follow the Step-by-step instructions below for building Utopia from source.

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

### Step-by-step Instructions
These instructions are intended for 'clean' __Ubuntu__ or __macOS__ setups.
The main difference between the two systems are the package managers.
On Mac, we recommend [Homebrew](https://brew.sh/). If you prefer to use [MacPorts](https://www.macports.org/),
notice that some packages might need to be installed differently.
Ubuntu is shipped with APT.

1. Mac OS X users need to start by installing the Apple Command Line Tools by executing

        xcode-select --install

2. Install third-party packages with the package manager:

    __Ubuntu:__

        apt update
        apt install cmake doxygen gcc g++ gfortran git pkg-config
    
    __macOS:__

        brew update
        brew install cmake doxygen gcc-7 pkg-config

3. Use `[git clone](https://git-scm.com/docs/git-clone)` to clone the
    DUNE repositories listed above into a suitable folder on your machine.
    Make sure to `[git checkout](https://git-scm.com/docs/git-checkout)` the correct branches.

4. __macOS only:__ Make sure that CMake uses the Homebrew GCC instead of
    the command line tools Clang:

        export CC=gcc-7
        export CXX=g++-7
    
    Notice that these commands only last for your current terminal session.

5. Configure and build DUNE and Utopia by executing the `dunecontrol` script:

        ./dune-common/bin/dunecontrol --module=utopia all

    Afterwards, reconfiguring and rebuilding can now also be done locally,
    instead of calling `dunecontrol`. After entering the `utopia/build-cmake` directory,
    you can call `cmake ..` or `make` directly.


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
