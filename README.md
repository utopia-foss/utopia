# Utopia

__Utopia__ (_gr.,_ no-place), a non-existent society described in considerable detail. [Wikipedia, 2016]

Utopia is a comprehensive modelling framework for complex environmental systems.
It aims to provide the tools to conveniently implement a model, configure and perform simulation runs, and evaluate the resulting data.
<!-- TODO Write more here ... -->

Utopia is free software and licensed under the
[GNU Lesser General Public License Version 3](https://www.gnu.org/licenses/lgpl-3.0.en.html)
or any later version.
For the copyright notice and the list of copyright holders, see
[`COPYING.md`](COPYING.md).

#### Contents of this README
* [Installation](#installation)
    * [On your machine](#step-by-step-instructions)
    * [Alternative: docker image](#utopia-via-docker)
* [Quickstart](#quickstart)
* [Documentation and Guides](#utopia-documentation)
* [Information for Developers](#information-for-developers)
* [Dependencies](#dependencies)
* [Troubleshooting](#troubleshooting)

---



<!-- ###################################################################### -->

## Installation
The following instructions will install Utopia into a development environment on your machine.

If you simply want to _run_ Utopia, you can do so via a [ready-to-use docker image](https://hub.docker.com/r/ccees/utopia); see [below](#utopia-via-docker) for more information.


### Step-by-step Instructions
These instructions are intended for 'clean' __Ubuntu__ (18.04) or __macOS__
setups.

#### 1 — Setup
First, create a `Utopia` directory at a place of your choice.
This is where the Utopia repository will be stored.
When working with or developing for Utopia, auxilary data will have a place here as well.


#### 2 — Cloning Utopia
In your terminal, enter the `Utopia` directory you just created.

With access to the Utopia GitLab project, you can clone the repository to that
directory using the following command:

```bash
git clone --recurse-submodules https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia.git
```

Inside your top level `Utopia` directory, there will now be a `utopia` directory, which mirrors this repository.

_Note:_ Utopia uses [Git Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules); with the `--recurse-submodules` option, the submodule repositories are automatically cloned alongside the `utopia` repository.


#### 3 — Install dependencies
Install the third-party dependencies using a package manager.

_Note:_ If you have [Anaconda](https://www.anaconda.com/) installed, you already have a working Python installation on your system, and you can omit installing the `python` packages below.
However, notice that there might be issues during [the configuration step](#4-configure-and-build). Have a look at the [troubleshooting](#troubleshooting) section to see how to address them.

##### On Ubuntu 18.04
```bash
apt update
apt install cmake doxygen gcc g++ gfortran git libarmadillo-dev \
            libboost-dev libboost-test-dev libhdf5-dev \
            pkg-config python3-dev python3-pip python3-venv
```

You will _probably_ need administrator rights. ([`sudo`, anyone?](https://xkcd.com/149/))

##### On macOS
First, install the Apple Command Line Tools:

```bash
xcode-select --install
```

As package manager on macOS, we recommend [Homebrew](https://brew.sh/).
(If you prefer to use [MacPorts](https://www.macports.org/), notice that some packages might need to be installed differently.)

```bash
brew update
brew install armadillo boost cmake doxygen hdf5 gcc pkg-config python3
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

The terminal output will show the configuration steps, which includes the installation of further Python dependencies and the creation of a virtual environment.

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

to enter the virtual environment, where the Utopia Command Line Interface (CLI) is available.
The `utopia` command gives you control over running and evaluating model simulations:

```bash
utopia run dummy
```

For more information on how to use the command line interface, see the [information for users](#how-to-run-a-model) below and the [documentation](#utopia-documentation).


### Utopia via Docker
[Docker](https://www.docker.com/) is a free OS-level virtualization software.
It allows to run any application in a closed environment container.

If you do not intend to develop models with Utopia, the Utopia docker image is a way to _run_ Utopia models and evaluate them without having to go through the installation procedure.

The image and instructions on how to create a container from it can be found on the [`ccees/utopia` docker hub page](https://hub.docker.com/r/ccees/utopia).




<!-- ###################################################################### -->

## Quickstart
This section gives a glimpse into working with Utopia.
It's not more than a glimpse; after playing around with this, [consult the documentation](#utopia-documentation) to gain access to more information material, and especially: the [**Utopia Tutorial**](https://hermes.iup.uni-heidelberg.de/utopia_doc/latest/html/guides/tutorial.html).


### How to run a model?
The Utopia command line interface (CLI) is, by default, only available in a Python virtual environment, in which `utopya` (the Utopia frontend) and its dependencies are installed.
To conveniently work with the frontend, you should thus enter the virtual environment:

```bash
source ./build/activate
```

Now, your shell should be prefixed with `(utopia-env)`.
All the following should take place inside this virtual environment.

As you have already done with the `dummy` model, the basic command to run a model named `SomeModel` is:

```bash
utopia run SomeModel
```

where `SomeModel` needs to be replaced with a valid model name.
To get a list of available models, run the `utopia models ls` command.
Alternatively, have a look at the [`src/utopia/models`](src/utopia/models) directory, where they are implemented.

The `utopia run` command carries out a pre-configured simulation for that model, loads the data, and performs automated plots.

_Note:_
* The script will always run through, even if there were errors in the
    individual parts. Thus, you should check the terminal output for errors
    and warnings (red or yellow, respectively). If you want the CLI to fail on
    errors, you can also set the `--debug` flag.
* By default, you can find the simulation output in
    `~/utopia_output/SomeModel/<timestamp>`. It contains the written data, all
    the used configuration files, and the default plots.

For further information, e.g. on how you can pass a custom configuration, check the CLI help:

```bash
utopia --help
utopia run --help
```

Best place to continue from here is the [tutorial](https://hermes.iup.uni-heidelberg.de/utopia_doc/latest/html/guides/tutorial.html).




<!-- ###################################################################### -->

## Utopia Documentation
This Readme only covers the very basics.
For all further documentation, tutorials, guides, model descriptions, frequently asked questions and more, you should consult the actual Utopia documentation.

The latest build (corresponding to the latest commit to `master`) is available
[online](https://hermes.iup.uni-heidelberg.de/utopia_doc/latest/html/index.html).

See [below](#building-the-documentation-locally) on how to build the documentation locally.



<!-- ###################################################################### -->

## Information for Developers
### New to Utopia? How do I implement a model?
Aside from exploring the already existing models, you should check out the guides in the [documentation](#utopia-documentation)) which will get you started for implementing your own, fancy, new Utopia model. :tada:

**For members of the CCEES group,** there is the internal [models repository](https://ts-gitlab.iup.uni-heidelberg.de/utopia/models); this is the new home of your model.

### Building the documentation locally
To build the documentation locally, navigate to the `build` directory and run

```bash
make doc
```

The [Sphinx](http://www.sphinx-doc.org/en/master/index.html)-built user documentation will then be located at `build/doc/html/`.
The C++ [doxygen](http://www.stack.nl/~dimitri/doxygen/)-documentation can be found at `build/doc/doxygen/html/`.
Open the respective `index.html` files to browse the documentation.

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

### Testing
Utopia contains unit tests to ensure consistency by checking if class members
and functions are working correctly. This is done both for the C++ and the
Python code.
The tests are integrated into the GitLab Continuous Integration pipeline,
meaning that tests are run upon every push to the project and failing tests
can be easily detected.

Tests can also be executed locally, to test a (possibly altered) version of
Utopia *before* committing and pushing changes to the GitLab.
To build them, set build flags to `Debug` as described [above](#build-types)
and then execute

```bash
make -j4 build_tests_all
```

where the `-j4` argument builds four test targets in parallel. It makes sense
to adjust this to the number of processors you want to engage in this task.
To then carry out the tests, call the corresponding `test_`-prefixed target:

```bash
make -j4 test_all
```

#### Test Groups
Usually, changes which are to be tested are concentrated in a few files, which
makes running all tests with `test_all` time-consuming and thus inefficient.
We therefore provide grouped tests, which relate only to a subset of tests.
A group of tests can be built and invoked individually by calling:

```bash
make build_tests_<identifier>
make test_<identifier>
```

Replace `<identifier>` by the appropriate testing group identifier from the
table below.

| Identifier        | Test description   |
| ----------------- | ------------------ |
| `core`            | Model infrastructure, managers, convenience functions … |
| `dataio`          | Data input and output, e.g. HDF5, YAML, … |
| `backend`         | Combination of `core` and `dataio` |
| `model_<name>`    | The C++ model tests of model with name `<name>` |
| `models`†         | The C++ and Python tests for _all_ models |
| `models_python`†‡ | All python model tests (from `python/model_tests`) |
| `utopya`†‡        | Tests for `utopya` frontend package |
| `all`             | All of the above. (Go make yourself a hot beverage, when invoking this.) |

_Note:_
* Identifiers marked with `†` require all models to be built (by running
  `make all`).
* Identifiers marked with `‡` do _not_ have a corresponding `build_tests_*`
  target.
* The `build_tests_` targets give you more control in scenarios where you want
  to test _only_ building.

#### Running Individual Test Executables
Each _individual_ test also has an individual build target, the names of which
you see in the output of the `make build_tests_*` command.
For invoking the individual test executable, you need to go to the
corresponding build directory, e.g. `build/tests/core/`, and run the executable
from that directory, as some of the tests rely on auxilary files which are
located relative to the executable.

For invoking individual Python tests, there are no targets specified.
However, [pytest](https://docs.pytest.org/en/latest/usage.html) gives you
control over which tests are invoked:

```bash
cd python
python -m pytest -v model_tests/<model_name>             # all tests
python -m pytest -v model_tests/<model_name>/my_test.py  # specific test file
python -m pytest -v utopya/test/<some_glob>              # selected via glob
```

_Note:_ Make sure you entered the virtual environment and the required
executables are built. See `pytest --help` for more information regarding the
CLI.


<!-- ###################################################################### -->

## Dependencies
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
| [Python3](https://www.python.org/downloads/) | >= 3.6 | Prior versions _might_ work, but are not tested |


#### Python
Utopia's frontend, `utopya`, uses some custom packages that are not part of the Python Package Index yet.

These packages and their dependencies are _automatically_ installed into a virtual environment when the `cmake ..` command is carried out during the [configuration step of the installation](#4-configure-and-build)).
Check out the [troubleshooting section](#troubleshooting) if this fails.

| Software | Version | Purpose |
| ---------| ------- | ------- |
| [Sphinx](https://www.sphinx-doc.org/en/master/) | >=2.0 | Documentation generator |
| [paramspace](https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace) | >= 2.2 | Makes parameter sweeps easy |
| [dantro](https://ts-gitlab.iup.uni-heidelberg.de/utopia/dantro) | >= 0.9 | A data loading and plotting framework |


#### Recommended
| Software | Version | Purpose |
| ---------| ------- | ------- |
| [doxygen](http://www.stack.nl/~dimitri/doxygen/) | >= 1.8.14 | Builds the C++ code documentation |
| [graphviz](https://graphviz.gitlab.io) | >= 2.38 | Used by doxygen to create dependency diagrams |
| [ffmpeg](https://www.ffmpeg.org) | >= 4.0 | Used for creating videos |



<!-- ###################################################################### -->

## Troubleshooting
* If the `cmake ..` command failed during resolution of the Python
    dependencies it is probably due to the configuration routine attempting to
    load the packages via SSH and you not having access to the GitLab.
    To fix this, the most comfortable solution is to register your SSH key
    with the GitLab; you can follow [these](https://docs.gitlab.com/ce/ssh)
    instructions to do so.  
    Alternatively, you can manually install the Python dependencies into the
    virtual environment:

    ```bash
    cd build
    source ./activate
    pip install git+https://...
    ```

    The URLs to use for cloning can be found by following the links in the
    [dependency table](#dependencies); if these dependencies change, you will
    have to manually update them.
    Note that deleting the build directory will also require to install the
    dependencies anew.

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
    removing the `build` directory completely and starting anew from the
    [configuration step](#4-configure-and-build) should help.

* In cases where you encounter errors with the model registry, it helps to
    remove the registry entries of all models and regenerate them:

    ```bash
    utopia models rm --all
    cd build
    cmake ..
    ```

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
