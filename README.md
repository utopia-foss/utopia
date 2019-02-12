# Utopia

__Utopia__ (_gr.,_ no-place), a non-existent society described in considerable detail. [Wikipedia, 2016]

Powered by [DUNE](https://dune-project.org/)

| Testing System | Python Test Coverage |
| :------------: | :------------------: |
| [![pipeline status](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/badges/master/pipeline.svg)](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/commits/master) | [![coverage report](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/badges/master/coverage.svg)](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/commits/master) |

#### Contents of this Readme
* [Installation](#installation)
* [Documentation and Guides](#utopia-documentation)
* [Quickstart](#quickstart)
* [Information for Developers](#information-for-developers)

---

## Installation

### Step-by-step Instructions
These instructions are intended for 'clean' __Ubuntu__ (18.04) or __macOS__ setups.


##### 1 — Setup
First, create a `Utopia` directory at a place of your choice. This is where not only the Utopia repository but also some of the dependencies will be stored in further subdirectories.  
_Note:_ All steps below should be carried out from within _this_ directory.

##### 2 — Cloning Utopia
For cloning the Utopia repository, you will need to use the `--recurse-submodules` option. (Utopia uses [Git Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules); this command automatically
clones the submodule repositories.)

    git clone --recurse-submodules https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia.git

##### 3 — DUNE dependencies
Utopia is a DUNE module and thus relies on the [DUNE Buildsystem](https://www.dune-project.org/doc/installation/) for installation. To fulfil that depenceny, [`git clone`](https://git-scm.com/docs/git-clone) all the DUNE dependencies from the [table](#dependencies) below.

    git clone https://gitlab.dune-project.org/core/dune-common.git
    git clone https://gitlab.dune-project.org/core/dune-geometry.git
    git clone https://gitlab.dune-project.org/core/dune-grid.git
    git clone https://gitlab.dune-project.org/staging/dune-uggrid

##### 4 — _(macOS only)_ Apple Command Line Tools
Install the Apple Command Line Tools by executing

    xcode-select --install

##### 5 — Install dependencies
Install third-party packages using a package manager.

__macOS:__ On macOS, we recommend [Homebrew](https://brew.sh/). (If you prefer to use [MacPorts](https://www.macports.org/), notice that some packages might need to be installed differently.)

    brew update
    brew install armadillo boost cmake doxygen fftw hdf5 gcc pkg-config \
                 python3 yaml-cpp

__Ubuntu:__

    apt update
    apt install cmake doxygen gcc g++ gfortran git libarmadillo-dev \
                libboost-dev libhdf5-dev libyaml-cpp-dev libfftw3-dev \
                pkg-config python3-dev python3-pip

_Note:_ You will _probably_ need administrator rights on Ubuntu. ([`sudo`, anyone?](https://xkcd.com/149/))

##### 6 — Configure and build
Configure and build DUNE and Utopia by executing the `dunecontrol` script:

    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DDUNE_PYTHON_VIRTUALENV_SETUP=True -DDUNE_PYTHON_ALLOW_GET_PIP=True" ./dune-common/bin/dunecontrol --module=utopia all

Afterwards, reconfiguring and rebuilding can also be done locally, instead of calling `dunecontrol`: Enter the `utopia/build-cmake` directory, and call `cmake ..` or `make` directly.

##### 7 — Run a model :tada:
Now, you should be able to run a utopia model:

    ./build-cmake/run-in-dune-env utopia run dummy

For more information on how to use the command line interface (and a prettier command :speak_no_evil:), see the [information for users](#how-to-run-a-model).


### Troubleshooting
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

* If you have a previous installation and the build failed, try removing *all* the `build-cmake` directories, either manually or using

        rm -r ./*/build-cmake/

* If at some point the `spdlog` and `yaml-cpp` dependencies are updated, you might need to update the git submodules. To get the current version, call:

        git submodule update

    This will perform a `git checkout` of the specified commit in all submodules. To fetch data of submodules that were not previously available on your branch, call the following command:

        git submodule update --init --recursive


### Dependencies

| Software | Version | Comments |
| ---------| ------- | -------- |
| GCC | >= 7 | Full C++17 compiler support required |
| _or:_ clang | >= 6 | Full C++17 compiler support required |
| _or:_ Apple LLVM | >= 9 | Full C++17 compiler support required |
| [CMake](https://cmake.org/) | >= 3.10 | |
| pkg-config | | |
| [HDF5](https://www.hdfgroup.org/solutions/hdf5/) | >= 1.10. | |
| [Boost](http://www.boost.org/) | >= 1.65 | |
| [Armadillo](http://arma.sourceforge.net/) | >= 8.400 | |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | 0.6.2 | Included as submodule |
| [spdlog](https://github.com/gabime/spdlog) | >= 0.17.0 | Included as submodule |
| [FFTW](http://www.fftw.org) | >= 3.3 | For fast fourier transformations |
| [dune-common](https://gitlab.dune-project.org/core/dune-common) | master | |
| [dune-geometry](https://gitlab.dune-project.org/core/dune-geometry) | master | |
| [dune-grid](https://gitlab.dune-project.org/core/dune-grid) | master | |
| [dune-uggrid](https://gitlab.dune-project.org/staging/dune-uggrid) | master | |
| [Python3](https://www.python.org/downloads/) | >= 3.6 | < 3.6 _might_ work, but is not tested |

#### Python
Utopia's frontend, `utopya`, uses some custom packages.

These packages and their dependencies are _automatically_ installed into a virtual environment when the `cmake ..` command is carried out (as done via `dunecontrol` in [step 6 of the installation](#6-configure-and-build)).
Check out the troubleshooting section there if this fails.

| Software | Version | Purpose |
| ---------| ------- | ------- |
| [Sphinx](https://www.sphinx-doc.org/en/master/) | 1.8 | Documentation generator |
| [paramspace](https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace) | >= 2.0 | Makes parameter sweeps easy |
| [dantro](https://ts-gitlab.iup.uni-heidelberg.de/utopia/dantro) | >= 0.6 | A data loading and plotting framework |

#### Recommended

| Software | Version | Purpose |
| ---------| ------- | ------- |
| [doxygen](http://www.stack.nl/~dimitri/doxygen/) | >= 1.8.14 | Builds the C++ code documentation |
| [ffmpeg](https://www.ffmpeg.org) | >= 4.0 | Used for creating videos |


## Utopia Documentation
This Readme only covers the basics. For all further documentation, tutorials, guides, model descriptions, and more, have a look into the documentation. It is _highly_ recommended to do this.

Utopia builds a documentation with [Sphinx](http://www.sphinx-doc.org/en/master/index.html), including all relevant information for the every-day user as well as developers.
Additionally, a C++ code documentation is built by Doxygen. To build the docs locally, navigate to the `utopia/build-cmake` directory and execute

    make doc

The user documentation will then be located at `utopia/build-cmake/doc/html/`, and the Doxygen documentation at `utopia/build-cmake/doc/doxygen/html/`. Open the respective `index.html` files to browse the documentation.

For first-time users, it is recommended to check out the tutorial there.


## Quickstart
This section gives a glimpse into working with Utopia. It's not more than a glimpse; after playing around with this, [build the documentation](#utopia-documentation) to have access to more information material.

### How to run a model?
The Utopia command line interface (CLI) is, by default, only available in a Python virtual environment, in which `utopya` (the Utopia frontend) and its dependencies are installed.
To conveniently work with the frontend, you should thus enter the virtual environment:

    source ./utopia/build-cmake/activate

Now, your shell should be prefixed with `(dune-env)`. All the following should take place inside this virtual environment.

The basic command to run the `SomeModel` model is 

    utopia run SomeModel

where `SomeModel` needs to be replaced with a valid model name; available models can be found under [`dune/utopia/models`](dune/utopia/models).
This command carries out a pre-configured simulation for that model, loads the data, and performs automated plots.

_Note:_
* The script will always run through, even if there were errors in the individual parts. Thus, you should check the terminal output for errors and warnings.
* By default, you can find the simulation output in `~/utopia_output/SomeModel/<timestamp>`. It contains the written data, all the used configuration files, and the automated plots.

For further information, e.g. on how you can pass a custom configuration, check the CLI help:

    utopia --help
    utopia run --help


## Information for Developers
### New to Utopia? How do I build a model?
Aside exploring the already existing models, you should check out the Beginner's Guide in the documentation (see above on [how to build it](#building-the-documentation)) which will guide you through the first steps of building your own, fancy, new Utopia model. :tada:

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
Utopia contains unit tests to ensure consistency by checking if class members and functions are working correctly. This is done both for the C++ and the Python code.
The tests are integrated into the GitLab Continuous Integration build process, meaning that failing tests can be easily detected.

Tests can also be executed locally, to test a (possibly altered) version of Utopia *before* committing changes. To build them, set build flags to `Debug` as described [above](#build-types) and then execute

    make build_tests -j4

where the `-j4` argument builds four test targets in parallel. To perform all tests, call

    ARGS="--output-on-failure" make test

If the test executables are not built before executing `make test`, the corresponding tests will inevitably fail.

#### Grouped Unit Tests
We grouped the tests to receive more granular information from the CI system.
You can choose to perform only tests from a specific group by calling

    make test_<group>

Replace `<group>` by the appropriate testing group identifier. Note that the `ARGS` environment variable is not needed here.

Available testing groups:

| Group | Info |
| ----- | ---- |
| `core` | Backend functions for models |
| `dataio` | Backend functions for reading config files and writing data |
| `utopya` | Frontend package for performing simulations, reading and analyzing data |
| `models` | Models tests (implemented in `python/model_tests`, not the C++ ones!) |
| `python` | All python tests |
