# Utopia

__Utopia__ (gr., no-place), a non-existent society described in considerable detail. [Wikipedia, 2016]

Powered by [DUNE](https://dune-project.org/)

| Testing System | Python Test Coverage |
| :------------: | :------------------: |
| [![pipeline status](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/badges/master/pipeline.svg)](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/commits/master) | [![coverage report](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/badges/master/coverage.svg)](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/commits/master) |

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
| [HDF5](https://www.hdfgroup.org/solutions/hdf5/) | >= 1.10. | |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | >= 0.5.2 | |
| [Boost](http://www.boost.org/) | >= 1.62 | |
| [dune-common](https://gitlab.dune-project.org/core/dune-common) | master | |
| [dune-geometry](https://gitlab.dune-project.org/core/dune-geometry) | master | |
| [dune-grid](https://gitlab.dune-project.org/core/dune-grid) | master | |
| [dune-uggrid](https://gitlab.dune-project.org/staging/dune-uggrid) | master | |
| Python | >= 3.6 | Earlier Python3 versions _may_ work, but are not tested |
| [paramspace](https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace) | >= 1.0b | |
| [dantro](https://ts-gitlab.iup.uni-heidelberg.de/utopia/dantro) | >= 0.1b | (soon) |

### Recommended
| Software | Version | Purpose |
| ---------| ------- | ------- |
| [doxygen](http://www.stack.nl/~dimitri/doxygen/) | >= 1.8.14 | Builds the code documentation upon installation |

### Optional Packages
| Software | Version | Purpose |
| -------- | ------- | ------- |
| [PSGraf](https://ts-gitlab.iup.uni-heidelberg.de/tools/psgraf)| master | Data visualization on the fly |

Install PSGraf according to its installation manual. When executing `dunecontrol`, append `CMAKE_FLAGS="-DPSGRAF_ROOT=<path/to/psgraf/build>"`. If PSGraf is found, the preprocessor macro `HAVE_PSGRAF` is set.

### Step-by-step Instructions
These instructions are intended for 'clean' __Ubuntu__ or __macOS__ setups.
The main difference between the two systems are the package managers.
On Mac, we recommend [Homebrew](https://brew.sh/). If you prefer to use [MacPorts](https://www.macports.org/),
notice that some packages might need to be installed differently.
Ubuntu is shipped with APT.

1. __macOS__ users need to start by installing the Apple Command Line Tools by executing

        xcode-select --install

2. Install third-party packages with the package manager:

    __Ubuntu:__

        apt update
        apt install cmake doxygen gcc g++ gfortran git libboost-dev \
            libhdf5-dev libyaml-cpp-dev pkg-config python3-dev
    
    __macOS:__

        brew update
        brew install boost cmake doxygen gcc pkg-config python
        brew install yaml-cpp --cc=gcc-7
        brew install hdf5 --cc=gcc-7
    
    __Notice:__ If you had `hdf5` or `yaml-cpp` already installed by Homebrew,
    make sure to `brew uninstall` them and then run the respective commands above.

3. Use [`git clone`](https://git-scm.com/docs/git-clone) to clone the
    DUNE repositories listed above into a suitable folder on your machine.
    Make sure to [`git checkout`](https://git-scm.com/docs/git-checkout) the correct branches (see the dependency list above).

4. __macOS only:__ Make sure that CMake uses the Homebrew GCC instead of
    the command line tools Clang:

        export CC=gcc-7
        export CXX=g++-7
    
    Notice that these commands only last for your current terminal session.

5. Configure and build DUNE and Utopia by executing the `dunecontrol` script:

        CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release \
            -DDUNE_PYTHON_VIRTUALENV_SETUP=True \
            -DDUNE_PYTHON_ALLOW_GET_PIP=True" \
            ./dune-common/bin/dunecontrol --module=utopia all

    Afterwards, reconfiguring and rebuilding can now also be done locally,
    instead of calling `dunecontrol`. After entering the `utopia/build-cmake` directory,
    you can call `cmake ..` or `make` directly.


#### Troubleshooting
* If you have a previous installation and the build failed, try removing *all* the `build-cmake` directories, either manually or using

        rm -r ./*/build-cmake/

* If the `dunecontrol` command failed during resolution of the Python
    dependencies it is due to the configuration routine attempting to load the
    packages via SSH. To fix this, the most comfortable solution is to register
    your SSH key with the GitLab; follow [this](https://docs.gitlab.com/ce/ssh)
    instruction to do so.  
    Alternatively, you can manually install the Python dependencies into the
    virtual environment that DUNE creates:

        ./build-cmake/run-in-dune-env pip install git+https://...

    The clone URLs can be found by following the links in the
    [dependency table](#dependencies). Note that deleting the build
    directories will also require to install the dependencies again.


### Building the Documentation
Utopia builds a Doxygen documentation from its source files. Use `dunecontrol` to execute the appropriate command:

    ./dune-common/bin/dunecontrol --only=utopia make doc

You will find the files inside the build directory `utopia/build-cmake/doc`.

### Build Types
If you followed the instructions above, you have a `Release` build which is
optimized for maximum performance. If you need to debug, you should reconfigure
the entire project with CMake by navigating to the `utopia/build-cmake` folder
and calling

    cmake -DCMAKE_BUILD_TYPE=Debug ..

Afterwards, call

    make <something>

to rebuild executables. CMake handles the required compiler flags automatically.

The build type (as most other CMake flags) persists until it is explicitly
changed by the user. To build optimized executables again, reconfigure with

    cmake -DCMAKE_BUILD_TYPE=Release ..


### Unit Tests
Utopia contains unit tests to ensure consistency by checking if class members and functions are working correctly. The tests are integrated into the GitLab Continuous Integration build process, meaning that failing tests cause an entire build to fail.

Tests can also be executed locally, to test a (possibly altered) version of Utopia *before* committing changes. To build them, execute

    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Debug" \
        ./dune-common/bin/dunecontrol --only=utopia make build_tests

and perform all tests by calling

    ARGS="--output-on-failure" ./dune-common/bin/dunecontrol --only=utopia make test

If the test executables are not built before executing `make test`, the corresponding tests will inevitably fail.

#### Grouped Unit Tests
We grouped the tests to receive more granular information from the CI system.
You can choose to perform only tests from a specific group by calling

    make test_<group>

Replace `<group>` by the appropriate testing group identifier.

Available testing groups:

| Group | Info |
| ----- | ---- |
| `core` | Backend functions for models |
| `dataio` | Backend functions for reading config files and writing data |
| `python` | Frontend functions for managing simulations and analyzing data |
