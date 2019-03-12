# Utopia

__Utopia__ (_gr.,_ no-place), a non-existent society described in considerable
detail. [Wikipedia, 2016]

Utopia is a modelling framework

#### Contents of this Readme
* [Installation](#installation)
* [Documentation and Guides](#utopia-documentation)
* [Quickstart](#quickstart)
* [Information for Developers](#information-for-developers)

---

## Installation
### Step-by-step Instructions
These instructions are intended for 'clean' __Ubuntu__ (18.04) or __macOS__
setups.

#### 1 — Setup
First, create a `Utopia` directory at a place of your choice. This is where
the Utopia repository will be stored. When working with Utopia, auxilary data
will have a place here as well.


#### 2 — Cloning Utopia
In your terminal, enter the `Utopia` directory you just created.

With access to the Utopia GitLab project, you can clone the repository to that
directory using the following command:

```bash
git clone --recurse-submodules https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia.git
```

Inside your top level `Utopia` directory, there will now be a `utopia`
directory, which is the repository.

_Note:_ Utopia uses [Git Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules);
with the `--recurse-submodules` option, the submodule repositories are
automatically cloned alongside the `utopia` repository.


#### 3 — Install dependencies
Install the third-party dependencies using a package manager.

_Note:_ If you have [Anaconda](https://www.anaconda.com/) installed, you
already have a working Python installation on your system, and you can omit
installing the `python` packages below. However, notice that there might
be issues during [the configuration step](#4-configure-and-build). Have a look
at the [troubleshooting](#troubleshooting) section to see how to address them.

##### On Ubuntu 18.04

```bash
apt update
apt install cmake doxygen gcc g++ gfortran git libarmadillo-dev \
            libboost-dev libhdf5-dev libyaml-cpp-dev libfftw3-dev \
            pkg-config python3-dev python3-pip python3-venv
```

You will _probably_ need administrator rights. ([`sudo`, anyone?](https://xkcd.com/149/))

##### On macOS
First, install the Apple Command Line Tools:

```bash
xcode-select --install
```

As package manager on macOS, we recommend [Homebrew](https://brew.sh/).
(If you prefer to use [MacPorts](https://www.macports.org/), notice that some
packages might need to be installed differently.)

```bash
brew update
brew install armadillo boost cmake doxygen fftw hdf5 gcc pkg-config python3 yaml-cpp
```


#### 4 — Configure and build
Enter the repository and create your desired build directory:

```bash
cd utopia
mkdir build
```

Now, enter the build directory and invoke CMake:

```bash
cd build
cmake ..
```

The terminal output will show the configuration steps, which includes the
installation of further python dependencies and the creation of a virtual
environment.

After this, you can build a specific or all Utopia models using:

```bash
make dummy     # builds only the dummy model
make -j4 all   # builds all models, using 4 CPUs
```

#### 5 — Run a model :tada:
You should now be able to run a Utopia model.
Being in the `build` directory, call:

```bash
source ./activate
```

to enter the virtual environment, where the Utopia Command Line Interface (CLI)
is available. The `utopia` command gives you control over running and
evaluating model simulations:

```bash
utopia run dummy
```

For more information on how to use the command line interface, see the
[information for users](#how-to-run-a-model) below and the [documentation](#utopia-documentation).


### Troubleshooting
* If the `cmake ..` command failed during resolution of the Python
    dependencies it is probably due to the configuration routine attempting to
    load the packages via SSH and you not having access to the GitLab.
    To fix this, the most comfortable solution is to register your SSH key
    with the GitLab; you can follow [this](https://docs.gitlab.com/ce/ssh)
    instruction to do so.  
    Alternatively, you can manually install the Python dependencies into the
    virtual environment:

    ```bash
    cd build
    source ./activate
    pip install git+https://...
    ```

    The URLs to use for cloning can be found by following the links in the
    [dependency table](#dependencies) above. Note that deleting the build
    directories will also require to install the dependencies again.

* Anaconda ships its own version of the HDF5 library which is _not_
    compatible with Utopia. To tell CMake where to find the correct version of
    the library, add the following argument (without the comments!) to the
    `cmake ..` command during [configuration](#4-configure-and-build):

    ```bash
    -DHDF5_ROOT=/usr/           # on Ubuntu
    -DHDF5_ROOT=/usr/local/     # on macOS (Homebrew)
    ```

    For Ubuntu, the full command would then be:

    ```bash
    cd build
    cmake -DHDF5_ROOT=/usr/ ..
    ```

* If you have a previous installation and the build fails inexplicably,
    removing the `build` directory completely and starting anew from
    [configuration](#4-configure-and-build) often helps.

* If at some point the `spdlog` and `yaml-cpp` dependencies are updated, you
    might need to update the git submodules. To get the current version, call:

    ```bash
    git submodule update
    ```

    This will perform a `git checkout` of the specified commit in all
    submodules. To fetch data of submodules that were not previously available
    on your branch, call the following command:

    ```bash
    git submodule update --init --recursive
    ```


### Dependencies

| Software | Version | Comments |
| ---------| ------- | -------- |
| GCC | >= 7 | Full C++17 support required |
| _or:_ clang | >= 6 | Full C++17 support required |
| _or:_ Apple LLVM | >= 9 | Full C++17 support required |
| [CMake](https://cmake.org/) | >= 3.10 | |
| pkg-config | | |
| [HDF5](https://www.hdfgroup.org/solutions/hdf5/) | >= 1.10. | |
| [Boost](http://www.boost.org/) | >= 1.65 | |
| [Armadillo](http://arma.sourceforge.net/) | >= 8.400 | |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | 0.6.2 | Included as submodule |
| [spdlog](https://github.com/gabime/spdlog) | >= 0.17.0 | Included as submodule |
| [FFTW](http://www.fftw.org) | >= 3.3 | For fast fourier transformations |
| [Python3](https://www.python.org/downloads/) | >= 3.6 | < v3.6 _might_ work, but are not tested |

#### Python
Utopia's frontend, `utopya`, uses some custom packages that are not part of the
Python Package Index.

These packages and their dependencies are _automatically_ installed into a
virtual environment when the `cmake ..` command is carried out during the
[configuration step of the installation](#4-configure-and-build)).
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
This Readme only covers the basics. For all further documentation, tutorials,
guides, model descriptions, and more, have a look into the documentation. It
is _highly_ recommended to do this.

Utopia builds a documentation with [Sphinx](http://www.sphinx-doc.org/en/master/index.html),
including all relevant information for the every-day user as well as
developers. Additionally, a C++ code documentation is built by Doxygen. To
build the docs locally, navigate to the `build` directory and execute

```bash
make doc
```

The user documentation will then be located at `build/doc/html/`, and the
Doxygen documentation at `build/doc/doxygen/html/`. Open the respective
`index.html` files to browse the documentation.

For first-time users, it is recommended to check out the Utopia Tutorial there.


## Quickstart
This section gives a glimpse into working with Utopia. It's not more than a
glimpse; after playing around with this,
[build the documentation](#utopia-documentation) to have access to more
information material, and especially: the **Utopia Tutorial**.

### How to run a model?
The Utopia command line interface (CLI) is, by default, only available in a
Python virtual environment, in which `utopya` (the Utopia frontend) and its
dependencies are installed.
To conveniently work with the frontend, you should thus enter the virtual
environment:

```bash
source ./build/activate
```

Now, your shell should be prefixed with `(utopia-env)`. All the following
should take place inside this virtual environment.

As you have already done with the `dummy` model, the basic command to run a
model named `SomeModel` is:

```bash
utopia run SomeModel
```

where `SomeModel` needs to be replaced with a valid model name; available
models can be found under [`src/models`](src/models).
This command carries out a pre-configured simulation for that model, loads the
data, and performs automated plots.

_Note:_
* The script will always run through, even if there were errors in the
    individual parts. Thus, you should check the terminal output for errors
    and warnings (red or yellow, respectively). If you want the CLI to fail on
    errors, you can also set the `--debug` flag.
* By default, you can find the simulation output in
    `~/utopia_output/SomeModel/<timestamp>`. It contains the written data, all
    the used configuration files, and the automated plots.

For further information, e.g. on how you can pass a custom configuration,
check the CLI help:

```bash
utopia --help
utopia run --help
```

Best place to continue from here: The tutorial in the Utopia documentation.


## Information for Developers
### New to Utopia? How do I implement a model?
Aside from exploring the already existing models, you should check out the
guides in the documentation (see above on
[how to build it](#building-the-documentation)) which will get you started for
implementing your own, fancy, new Utopia model. :tada:


### Build Types
If you followed the instructions above, you have a `Release` build which is
optimized for maximum performance. If you need to debug, you should reconfigure
the entire project with CMake by navigating to the `build` folder and calling

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

Afterwards, call

```bash
make <something>
```

to rebuild executables. CMake handles the required compiler flags
automatically.

The build type (as most other CMake flags) persists until it is explicitly
changed by the user. To build optimized executables again, reconfigure with

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Unit Tests
Utopia contains unit tests to ensure consistency by checking if class members
and functions are working correctly. This is done both for the C++ and the
Python code.
The tests are integrated into the GitLab Continuous Integration build process,
meaning that failing tests can be easily detected.

Tests can also be executed locally, to test a (possibly altered) version of
Utopia *before* committing changes. To build them, set build flags to `Debug`
as described [above](#build-types) and then execute

```bash
make build_unit_tests -j4
```

where the `-j4` argument builds four test targets in parallel. To perform all
tests, call

```bash
ARGS="--output-on-failure" make test
```

If the test executables are not built before executing `make test`, the
corresponding tests will inevitably fail.

#### Grouped Unit Tests
We grouped the tests to receive more granular information from the CI system.
You can choose to perform only tests from a specific group by calling

```bash
make test_<group>
```

Replace `<group>` by the appropriate testing group identifier. Note that the
`ARGS` environment variable is not needed here.

Available testing groups:

| Group    | Info                                                           |
| -------- | -------------------------------------------------------------- |
| `core`   | Backend functions for models                                   |
| `dataio` | Backend functions for writing data and working with YAML       |
| `utopya` | Frontend package managing simulations and their evaluation     |
| `models` | The C++ and Python tests for _all_ models                      |
| `model_python` | All python models tests (from `python/model_tests`)      |
| `model_<name>` | The C++ model tests of model with name `<name>`          |
