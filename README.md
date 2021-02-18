# Utopia

Utopia is a comprehensive modelling framework for complex and evolving systems.
It aims to provide the tools to conveniently implement a model, configure and perform simulation runs, and evaluate the resulting data.

Utopia is free software and licensed under the [GNU Lesser General Public License Version 3][LGPL] or any later version.
For the copyright notice and the list of copyright holders, see [`COPYING.md`](COPYING.md).

## Overview

Complex and evolving systems are investigated with a wide array of different models types.
Among the most popular are cellular automata (CA), agent-based models (ABMs), and network models.
Utopia provides a library to build these types of models in modern C++.
Its `core` template library contains functions for implementing model dynamics and scaffoldings for building new models.
The Utopia `dataIO` library is capable of storing hierarchical simulation data efficiently in the H5 file format.
Within Utopia, and when using Utopia as a dependency, models are integrated into a simulation infrastructure for conveniently executing and analyzing them.

The `utopya` Python package constitutes Utopia's frontend.
It configures and performs the simulation runs and embeds the model into a data processing pipeline, such that the simulation data can directly be analyzed and visualized.
All parts of the frontend make use of a hierarchic, recursively-updated YAML-based configuration structure.
Using the [paramspace package][paramspace], this allows to easily define parameter sweeps, which can then be carried out as simultaneous simulations on massively parallel, distributed machines.
The [dantro][dantro]-based data processing pipeline automates visualization, thereby coupling the model implementation and its analysis closer together.

Several models are readily included in the framework, among them Conway's Game of Life, as well as one CA- and one agent-based contagious disease model.
Investigating these models by performing simulation runs with a few varying parameters, or sensitivity analysis over a large parameter space, requires little to no programming skills.

For introductory guides, feature lists, FAQs, and API references refer to the online [user manual](https://hermes.iup.uni-heidelberg.de/utopia_doc/master/html/index.html).

### How to cite Utopia

Utopia was reviewed and published in the [Journal of Open Source Software (JOSS)](https://joss.theoj.org/).
Please cite at least the following publication if you use Utopia (or a modified version thereof) for your own work:

> Lukas Riedel, Benjamin Herdeanu, Harald Mack, Yunus Sevinchan, and Julian Weninger. 2020. **“Utopia: A Comprehensive and Collaborative Modeling Framework for Complex and Evolving Systems.”** *Journal of Open Source Software* 5 (53): 2165. DOI: [10.21105/joss.02165](https://doi.org/10.21105/joss.02165).

The [`CITATION.cff`](CITATION.cff) file in this repository follows the [citation file format](https://citation-file-format.github.io/) and contains additional metadata to reference this software, its authors, and associated publications.

### Contents of this README
* [How to install](#how-to-install)
    * [On your machine](#step-by-step-instructions)
    * [Alternative: docker image](#utopia-via-docker)
* [Quickstart](#quickstart)
* [Documentation and Guides](#utopia-documentation)
* [Information for Developers](#information-for-developers)
* [Dependencies](#dependencies)
* [Troubleshooting](#troubleshooting)

---


<!-- ###################################################################### -->

## How to install
The following instructions will install Utopia into a development environment
on your machine.

If you simply want to _run_ Utopia, you can do so via a [ready-to-use docker image][Utopia-docker]; see [below](#utopia-via-docker) for more information.


### Step-by-step Instructions
These instructions are intended for 'clean' __macOS__ or __Ubuntu__ (20.04 or newer) setups.

Since __Windows__ supports the installation of Ubuntu via [Windows Subsystem for Linux](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux), Utopia can also be used on Windows.
Follow the [WSL Installation Guide](https://docs.microsoft.com/en-us/windows/wsl/install-win10) to install Ubuntu, then follow the instructions for Ubuntu in this README.

_Note:_ Utopia is always tested against a recent Ubuntu release.
However, you may also use Utopia with an earlier release, as long as the [dependencies](#dependencies) can be fulfilled.

⚠️ If you encounter difficulties, have a look at the [**troubleshooting section**](#troubleshooting).
If this does not resolve your installation problems, [please file an issue in the GitLab project][Utopia-new-issue].


#### 1 — Clone Utopia
First, create a `Utopia` directory at a place of your choice.
This is where the Utopia repository will be cloned to.
When working with or developing for Utopia, auxiliary data will have a place
there as well.

In your terminal, enter the `Utopia` directory you just created.

Now, get a clone URL via the _Clone_ button in the top right-hand corner of the [Utopia project page][Utopia].  
If you are a developer or CCEES group member, you should [get an SSH key registered](#ssh-repository-access) with the GitLab and use the SSH address.
Otherwise, use the HTTPS address.

To clone the repository, use the following command and add the chosen clone URL at the indicated place:

```bash
git clone UTOPIA-CLONE-URL
```

After cloning, there will be a new `utopia`
directory (mirroring this repository) inside your top-level `Utopia` directory.



#### 2 — Install dependencies
Install the third-party dependencies using a package manager.

_Note:_ If you have [Anaconda][Anaconda] installed, you already have a working Python installation on your system, and you can omit installing the `python` packages below.
However, notice that there might be issues during [the configuration step](#4-configure-and-build).
Have a look at the [troubleshooting](#troubleshooting) section to see how to address them.

##### On Ubuntu (20.04)
```bash
apt update
apt install cmake doxygen gcc g++ gfortran git libarmadillo-dev \
            libboost-dev libboost-test-dev libhdf5-dev libspdlog-dev \
            libyaml-cpp-dev pkg-config python3-dev python3-pip python3-venv
```

Further, we recommend to install the following optional packages:

```bash
apt update
apt install ffmpeg graphviz doxygen
```

You will _probably_ need administrator rights. ([`sudo`, anyone?](https://xkcd.com/149/))

##### On macOS
First, install the Apple Command Line Tools:

```bash
xcode-select --install
```

There are two popular package managers on macOS, [Homebrew][Homebrew] and [MacPorts][MacPorts].
We recommend you use Homebrew.
Here are the installation instructions for both:

* **Homebrew**:

    Install the required packages:

    ```bash
    brew update
    brew install armadillo boost cmake hdf5 pkg-config python3 spdlog yaml-cpp
    ```

    Further, we recommend to install the following optional packages:

    ```bash
    brew update
    brew install ffmpeg graphviz doxygen
    ```

* **MacPorts**:

    Notice that `port` commands typically require administrator rights (`sudo`).

    Install the required packages:

    ```bash
    port install armadillo boost cmake hdf5 python37 py37-pip spdlog yaml-cpp
    ```

    Select the installed versions of Python and Pip:

    ```bash
    port select --set python python37
    port select --set python3 python37
    port select --set pip pip37
    port select --set pip3 pip37
    ```

    Further, we recommend to install the following optional packages:

    ```bash
    port install doxygen ffmpeg graphviz
    ```


#### 3 — Configure and build
Enter the repository and create your desired build directory:

```bash
cd utopia
mkdir build
```

Now, enter the build directory and invoke CMake (and mind the caveats below):

```bash
cd build
cmake ..
```

**Note:** If you use **MacPorts**, append the location of your Python installation to the CMake command (this is only required when calling CMake on a clean `build` directory):

```bash
cmake -DPython_ROOT_DIR=/opt/local ..
```

The terminal output will show the configuration steps, which includes the installation of further Python dependencies and the creation of a virtual environment.

After this, you can build a specific or all Utopia models using:

```bash
make dummy     # builds only the dummy model
make -j4 all   # builds all models, using 4 CPUs
```

_Note:_ On macOS ≥ 11, you might encounter errors at this point; please refer to [the troubleshooting section](#troubleshooting).


#### 4 — Run a model 🎉
You should now be able to run a Utopia model.
Being in the `build` directory, call:

```bash
source ./activate
```

to enter the virtual environment, where the Utopia Command Line Interface (CLI) is available.
(If you later want to exit the virtual environment, call the `deactivate` command.)

*Note:* If you are using `csh` or `fish` shells, use the respective `activate` scripts located in `build/utopia-env/bin/` (see [below](#how-to-run-a-model)).

The `utopia` command is now available and gives you control over running and evaluating model simulations:

```bash
utopia run dummy
```

The model output will be written into `~/utopia_output/dummy/<timestamp>`.
For more information on how to use the command line interface, see the [information for users](#how-to-run-a-model) below and the [documentation](#utopia-documentation).


#### 5 — Make sure everything works
_This step is optional, but recommended._

To make sure that Utopia works as expected on your machine, [build and carry out all tests](#testing).

### Optional Installation Steps

The following instructions will enable additional, *optional* features of Utopia.

#### Enable Multithreading in Utopia Models

1. Install the optional dependencies for multithreading.

    * On **Ubuntu**:

        ```bash
        apt update && apt install libtbb-dev
        ```

    * On **macOS**, with **Homebrew** (please mind the note below!):

        ```bash
        brew update && brew install parallelstl
        ```

1. Enter the Utopia build directory, and call CMake again.

    ```bash
    cd build
    cmake ..
    ```

    **Note:** If you installed ParallelSTL on **macOS** via **Homebrew**, specify the path to the installation in `/usr/local/Cellar/`:

    ```bash
    cmake -DParallelSTL_ROOT=/usr/local/Cellar/parallelstl/<version>/ ..
    ```

    Make sure to replace `<version>` with the version installed on your machine; use `brew list parallelstl` to find the correct path.

    You will only have to add this path to the CMake call **once**.
    It will be stored until you delete the contents of the build directory.

    At the end of its output, CMake should now report that the "Multithreading" feature has been enabled.

1. Re-compile the models. Inside the build directory, call

    ```bash
    make all
    ```

1. Depending on the algorithms used inside the respective model, it will automatically exploit the multithreading capabilities of your system when executed! 🎉


### Utopia via Docker
[Docker][docker] is a free OS-level virtualization software.
It allows to run any application in a closed environment container.

The Utopia docker image is a way to run Utopia models and evaluate them without having to go through the installation procedure.
It is also suitable for framework and model development.

The image and instructions on how to create a container from it can be found on the [`ccees/utopia` docker hub page][Utopia-docker].




<!-- ###################################################################### -->

## Quickstart
This section gives a glimpse into working with Utopia.
It's not more than a glimpse; after playing around with this, [consult the documentation](#utopia-documentation) to gain access to more information material, and especially: the [**Utopia Tutorial**][Utopia-tutorial].


### How to run a model?
The Utopia command line interface (CLI) is, by default, only available in a Python virtual environment, in which `utopya` (the Utopia frontend) and its dependencies are installed.
To conveniently work with the frontend, you should thus enter the virtual environment.
Execute *one* of the commands below depending on which type of shell you use:

```bash
source ./build/activate                  # For bash, zsh, or similar
source ./build/utopia-env/activate.csh   # For csh
source ./build/utopia-env/activate.fish  # For fish
```

Now, your shell should be prefixed with `(utopia-env)`.
(To exit the virtual environment, simply call `deactivate`.)
All the following should take place inside this virtual environment.

As you have already done with the `dummy` model, the basic command to run a model named `SomeModel` is:

```bash
utopia run SomeModel
```

where `SomeModel` needs to be replaced with a valid model name.
To get a list of available models, run the `utopia models ls` command.
Alternatively, have a look at the [`src/utopia/models`](../src/utopia/models) directory, where they are implemented.

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

Best place to continue from here is the [tutorial][Utopia-tutorial].




<!-- ###################################################################### -->

## Utopia Documentation
This Readme only covers the very basics.
For all further documentation, tutorials, guides, model descriptions, frequently asked questions and more, you should consult the actual Utopia documentation.

The latest build (corresponding to the latest commit to `master`) is available
[online][Utopia-docs].

See [below](#building-the-documentation-locally) on how to build the documentation locally.



<!-- ###################################################################### -->

## Information for Developers
### SSH Repository Access
To work comfortably with Utopia, access to the GitLab needs to be easy.
The best way to achieve that is by registering a so-called SSH key with your
GitLab account.

To do that, follow the linked instructions to [generate a key pair](https://docs.gitlab.com/ce/ssh/#generating-a-new-ssh-key-pair)
and to [add a key to your GitLab account](https://docs.gitlab.com/ce/ssh/#adding-an-ssh-key-to-your-gitlab-account).

### New to Utopia? How do I implement a model?
Aside from exploring the already existing models, you should check out the guides in the [documentation](#utopia-documentation)) which will get you started for implementing your own, fancy, new Utopia model. 🎉

**For members of the CCEES group,** there is the internal [models repository][Models]; this is the home of your model.

### Building the documentation locally
To build the documentation locally, navigate to the `build` directory and run

```bash
make doc
```

The [Sphinx][Sphinx]-built user documentation will then be located at `build/doc/html/`.
The C++ [doxygen][doxygen]-documentation can be found at `build/doc/doxygen/html/`.
Open the respective `index.html` files to browse the documentation.

### Choosing a different compiler
CMake will inspect your system paths and use the default compiler.
You can use the `CC` and `CXX` environment variables to select a specific C and C++ compiler, respectively.
As compiler paths are cached by CMake, changing the compiler requires you to delete the `build` directory and re-run CMake.

Ubuntu 20.04 LTS (Focal Fossa), for example, provides GCC 9 and GCC 10.
To use GCC 10, first install it via APT:

```bash
apt update && apt install gcc-10 g++-10
```

Then create the build directory, enter it, and set the `CC` and `CXX` environment variables when calling CMake:

```bash
mkdir build && cd build
CC=/usr/bin/gcc-10 CXX=/usr/bin/g++-10 cmake ..
```

Alternatively, you can `export` these variables in separate bash commands before executing CMake.

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

to rebuild executables.
CMake handles the required compiler flags automatically.

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

To build all tests, run the following commands:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4 build_tests_all
```
This first sets the `Debug` build type as described [above](#build-types) and
then builds all tests.
The `-j4` argument specifies that four test targets can be built in parallel.
It makes sense to adjust this to the number of processors you want to engage
in this task.

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
* Identifiers marked with `†` require all models to be built (by invoking
  `make all` before running these tests).
* Identifiers marked with `‡` do _not_ have a corresponding `build_tests_*`
  target.
* The `build_tests_` targets give you more control in scenarios where you want
  to test _only_ building.


#### Running Individual Test Executables
Each _individual_ test also has an individual build target, the names of which
you see in the output of the `make build_tests_*` command.
For invoking the individual test executable, you need to go to the
corresponding build directory, e.g. `build/tests/core/`, and run the executable
from that directory, as some of the tests rely on auxiliary files which are
located relative to the executable.

For invoking individual *Python* tests, there are no targets specified.
However, [pytest][pytest-usage] gives you control over which tests are invoked:

```bash
cd python
python -m pytest -v model_tests/{model_name}             # all tests
python -m pytest -v model_tests/{model_name}/my_test.py  # specific test file
```

Tests for individual `utopya` modules need to be run through the `python/utopya/run_test.py` executable rather than through `pytest` directly:

```bash
cd python/utopya
python run_test.py -v test/{some_glob}                   # selected via glob
```

_Note:_ For all of the above, make sure you entered the virtual environment and the required executables are all built; call `make all` to make sure.
See `pytest --help` for more information regarding the CLI.


#### Evaluating Test Code Coverage
Code coverage is useful information when writing and evaluating tests.
The coverage percentage is reported via the GitLab CI pipeline.
It is the mean of the test coverage for the Python code and the C++ code.

For retrieving this information on your machine, follow the instructions below.

##### Python code coverage
The Python code coverage is only relevant for the `utopya` package.
It can be analyzed using the [`pytest-cov`][pytest-cov]
extension for `pytest`, which is installed into the `utopia-env` alongside the
other dependencies.  
When running `make test_utopya`, the code coverage is tracked by default and
shows a table of utopya files and the lines within them that were _not_
covered by the tests.

If you would like to test for the coverage of some specific part of `utopya`,
adjust the test command accordingly to show the coverage report only for one
module, for example `utopya.multiverse`:

```bash
(utopia-env) $ cd python/utopya
(utopia-env) $ python run_test.py -v test/test_multiverse.py --cov=utopya.multiverse --cov-report=term-missing
```


##### C++ code coverage

1. Compile the source code with code coverage flags. Utopia provides the CMake
    configuration option `CPP_COVERAGE` for that purpose,

    ```bash
    cmake -DCPP_COVERAGE=On ..
    ```

    This will add the appropriate compiler flags to all tests
    and models within the Utopia framework. Notice that code coverage is
    disabled for `Release` builds because aggressive compiler optimization
    produces unreliable coverage results.

2. Execute the tests. Simply call the test commands listed in the previous
    sections.

3. Run the coverage result utility [`gcovr`][gcovr].

    First, install it via `pip`. If you want your host system to be unaffected,
    enter the Utopia virtual environment first.

        pip3 install gcovr

    `gcovr` takes several arguments. The easiest way of using it is moving to
    the build directory and executing

        gcovr --root ../

    This will display the source files and their respective coverage summary
    into the terminal. You can narrow down the report to certain source code
    paths using the `--filter` option and exclude others with the `--exclude`
    option.

    `gcovr` can give you a detailed HTML summary containing the color coded
    source code. We recommend reserving a separate directory in the build
    directory for that matter:

        mkdir coverage
        gcovr --root ../ --html --html-details -o coverage/report.html

Note that the C++ code coverage can also be evaluated when using the Python
test framework to run the tests, because the information is gathered directly
from the executable.  
This makes sense especially for the model tests, where it is sometimes more
convenient to test the results of a model run rather than some individual part
of it.



<!-- ###################################################################### -->

## Dependencies

| Software             | Required Version    | Tested Version  | Comments                    |
| -------------------- | ------------------- | --------------- | --------------------------- |
| GCC                  | >= 9                | 10              | Full C++17 support required |
| _or:_ clang          | >= 9                | 10.0            | Full C++17 support required |
| _or:_ Apple LLVM     | >= 9                |                 | Full C++17 support required |
| [CMake][cmake]       | >= 3.13             | 3.16            | |
| pkg-config           | >= 0.29             | 0.29            | |
| [HDF5][HDF5]         | >= 1.10.4           | 1.10.4          | |
| [Boost][Boost]       | >= 1.67             | 1.71            | |
| [Armadillo][arma]    | >= 9.600            | 9.800           | |
| [yaml-cpp][yaml-cpp] | >= 0.6.2            | 0.6.2           | |
| [spdlog][spdlog]     | >= 1.3              | 1.5.0           | |
| [Python3][Python3]   | >= 3.6              | 3.8.2           | |

Utopia aims to allow rapid development, and is thus being tested only against the more recent releases of its dependencies.
Currently, Utopia is tested against the packages provided by **Ubuntu 20.04**.
However the above version _requirements_ (i.e., those _enforced_ by the build system) can be fulfilled also with Ubuntu 19.10.

To get Utopia running on a system with an earlier Ubuntu version, the above dependencies still need to be fulfilled.
You can use the [Ubuntu Package Search][Ubuntu-packages] to find the versions available on your system.
If a required version is not available, private package repositories may help to install a more recent version of a dependency.

If you encounter difficulties with the installation procedure for any of these dependencies, please [file an issue in the GitLab project][Utopia-new-issue].


#### Python
Utopia's frontend, `utopya`, uses some additional python packages.

These packages and their dependencies are _automatically_ installed into a virtual environment when the `cmake ..` command is carried out during the [configuration step of the installation](#4-configure-and-build)).

| Software                 | Version    | Comments                        |
| ------------------------ | ---------- | ------------------------------- |
| [Sphinx][Sphinx]         | >= 2.0     | Builds the Utopia documentation |
| [paramspace][paramspace] | >= 2.5.4   | Makes parameter sweeps easy     |
| [dantro][dantro]         | >= 0.16    | Handle, transform, and visualize hierarchically organized data |


#### Recommended
The following depencies are _recommended_ to be installed, but are not strictly required by Utopia:

| Software              | Version | Comments                          |
| --------------------- | ------- | --------------------------------- |
| [doxygen][doxygen]    | >= 1.8  | Builds the C++ code documentation |
| [graphviz][graphviz]  | >= 2.40 | Used by doxygen to create dependency diagrams |
| [ffmpeg][ffmpeg]      | >= 4.1  | Used for creating videos |
| [ParallelSTL][pstl]   |         | Parallelization of STL algorithms (included in GCC >= 9) |
| [TBB][tbb]            | >= 2018.5  | Intel parallelization library required by ParallelSTL |



<!-- ###################################################################### -->

## Troubleshooting
* On **macOS Big Sur**, you *might* encounter an error during building of models, claiming that certain SDKs in `/Library/Developer/CommandLineTools/SDKs/` are missing.
    A remedy can be to add a [symlink](https://apple.stackexchange.com/a/115647/208830) `MacOSX11.0.sdk -> MacOSX.sdk`, depending on the given error message:

    ```bash
    $ ln -s /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk /Library/Developer/CommandLineTools/SDKs/MacOSX11.0.sdk
    ```

    This command might require `sudo`.

* If you have a previous installation and the **build fails inexplicably**,
    removing the `build` directory completely and starting anew from the
    [configuration step](#3-configure-and-build) should help.  
    In cases where the installation _used_ to work but at some point _stopped_ working, this should be a general remedy.

* In cases where you encounter errors with the **model registry**, it helps to
    remove the registry entries of all models and regenerate them:

    ```bash
    utopia models rm --all
    cd build
    cmake ..
    ```

* **Anaconda** ships its own version of the HDF5 library which is _not_
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


<!-- Links ################################################################ -->

[Utopia]: https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia
[Models]: https://ts-gitlab.iup.uni-heidelberg.de/utopia/models

[Utopia-issues]: https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues
[Utopia-new-issue]: https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues/new?issue

[Utopia-docs]: https://hermes.iup.uni-heidelberg.de/utopia_doc/latest/html/
[Utopia-tutorial]: https://hermes.iup.uni-heidelberg.de/utopia_doc/latest/html/guides/tutorial.html
[Utopia-docker]: https://hub.docker.com/r/ccees/utopia

[paramspace]: https://gitlab.com/blsqr/paramspace
[dantro]: https://ts-gitlab.iup.uni-heidelberg.de/utopia/dantro

[LGPL]: https://www.gnu.org/licenses/lgpl-3.0.en.html
[Ubuntu-packages]: https://packages.ubuntu.com
[docker]: https://www.docker.com/
[Homebrew]: https://brew.sh/
[MacPorts]: https://www.macports.org/
[Anaconda]: https://www.anaconda.com/

[CMake]: https://cmake.org/
[HDF5]: https://www.hdfgroup.org/solutions/hdf5/
[Boost]: http://www.boost.org/
[arma]: http://arma.sourceforge.net/
[yaml-cpp]: https://github.com/jbeder/yaml-cpp
[spdlog]: https://github.com/gabime/spdlog
[Python3]: https://www.python.org/downloads/
[Sphinx]: https://www.sphinx-doc.org/en/master/
[doxygen]: http://www.doxygen.nl
[graphviz]: https://graphviz.gitlab.io
[ffmpeg]: https://www.ffmpeg.org
[gcovr]: https://gcovr.com/en/stable/
[pytest-usage]: https://docs.pytest.org/en/latest/usage.html
[pytest-cov]: https://github.com/pytest-dev/pytest-cov
[tbb]: https://github.com/oneapi-src/oneTBB
[pstl]: https://github.com/oneapi-src/oneDPL
