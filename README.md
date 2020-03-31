# Utopia

__Utopia__ (_gr.,_ no-place), a non-existent society described in considerable detail. [Wikipedia, 2016]

Utopia is a comprehensive modelling framework for complex and evolving environmental systems.
It aims to provide the tools to conveniently implement a model, configure and perform simulation runs, and evaluate the resulting data.
<!-- TODO Write more here ... -->

Utopia is free software and licensed under the [GNU Lesser General Public License Version 3][LGPL] or any later version.
For the copyright notice and the list of copyright holders, see [`COPYING.md`](COPYING.md).

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
The following instructions will install Utopia into a development environment
on your machine.

If you simply want to _run_ Utopia, you can do so via a [ready-to-use docker image][Utopia-docker]; see [below](#utopia-via-docker) for more information.


### Step-by-step Instructions
These instructions are intended for 'clean' __macOS__ or __Ubuntu__ (19.10) setups.

_Note:_ Utopia is always tested against a recent Ubuntu release.
However, you may also use Utopia with an earlier release, as long as the [dependencies](#dependencies) can be fulfilled.

⚠️ If you encounter difficulties, have a look at the [**troubleshooting section**](#troubleshooting).
If this does not resolve your installation problems, [please file an issue in the GitLab project][Utopia-new-issue].


#### 1 — Clone Utopia
First, create a `Utopia` directory at a place of your choice.
This is where the Utopia repository will be cloned to.
When working with or developing for Utopia, auxilary data will have a place
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

##### On Ubuntu (19.10)
```bash
apt update
apt install cmake doxygen gcc g++ gfortran git libarmadillo-dev \
            libboost-dev libboost-test-dev libhdf5-dev libspdlog-dev \
            libyaml-cpp-dev pkg-config python3-dev python3-pip python3-venv
```

Further, we recommend to install the following optional packages:

```bash
apt update
apt install ffmpeg graphviz doxygen libtbb-dev
```

You will _probably_ need administrator rights. ([`sudo`, anyone?](https://xkcd.com/149/))

##### On macOS
First, install the Apple Command Line Tools:

```bash
xcode-select --install
```

There are two popular package managers on macOS, [Homebrew][Homebrew] and [MacPorts][MacPorts].
We recommend you use Homebrew. Here are the installation instructions for both:

* **Homebrew**:

    Install the required packages:

    ```bash
    brew update
    brew install armadillo boost cmake hdf5 pkg-config python3 spdlog yaml-cpp
    ```

    Further, we recommend to install the following optional packages:

    ```bash
    brew update
    brew install ffmpeg graphviz doxygen tbb
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
    port install doxygen ffmpeg graphviz tbb
    ```

#### 3 — Configure and build
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

**Note:** If you use **MacPorts**, append the location of your Python installation to the CMake command:

```bash
cmake -DPython_ROOT_DIR=/opt/local ..
```

The terminal output will show the configuration steps, which includes the installation of further Python dependencies and the creation of a virtual environment.

After this, you can build a specific or all Utopia models using:
```bash
make dummy     # builds only the dummy model
make -j4 all   # builds all models, using 4 CPUs
```

#### 4 — Run a model 🎉
You should now be able to run a Utopia model.
Being in the `build` directory, call:

```bash
source ./activate
```

to enter the virtual environment, where the Utopia Command Line Interface (CLI) is available.
(If you later want to exit the virtual environment, call the `deactivate` command.)

The `utopia` command is now available and gives you control over running and evaluating model simulations:

```bash
utopia run dummy
```

For more information on how to use the command line interface, see the [information for users](#how-to-run-a-model) below and the [documentation](#utopia-documentation).


#### 5 — Make sure everything works
_This step is optional, but recommended._

To make sure that Utopia works as expected on your machine, [build and carry out all tests](#testing).


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
To conveniently work with the frontend, you should thus enter the virtual environment:

```bash
source ./build/activate
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
from that directory, as some of the tests rely on auxilary files which are
located relative to the executable.

For invoking individual Python tests, there are no targets specified.
However, [pytest][pytest-usage] gives you control over which tests are invoked:

```bash
cd python
python -m pytest -v model_tests/<model_name>             # all tests
python -m pytest -v model_tests/<model_name>/my_test.py  # specific test file
python -m pytest -v utopya/test/<some_glob>              # selected via glob
```

_Note:_ Make sure you entered the virtual environment and the required
executables are built. See `pytest --help` for more information regarding the
CLI.

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
python -m pytest -v utopya/test/test_multiverse.py --cov=utopya.multiverse --cov-report=term-missing
```


##### C++ code coverage

1. Compile the source code with code coverage flags. Utopia provides the CMake
    configuration option `CPP_COVERAGE` for that purpose.

    When calling CMake, append `CPP_COVERAGE=On` to add the flags to all tests
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

| Software             | Version    | Comments                    |
| -------------------- | ---------- | --------------------------- |
| GCC                  | >= 9       | Full C++17 support required |
| _or:_ clang          | >= 9       | Full C++17 support required |
| _or:_ Apple LLVM     | >= 9       | Full C++17 support required |
| [CMake][cmake]       | >= 3.13    | ‡ |
| pkg-config           | >= 0.29    | |
| [HDF5][HDF5]         | >= 1.10.4  | ‡ |
| [Boost][Boost]       | >= 1.67    | ‡ |
| [Armadillo][arma]    | >= 9.600   | ‡ |
| [yaml-cpp][yamlcpp]  | >= 0.6.2   | ‡ |
| [spdlog][spdlog]     | >= 1.3     | ‡ |
| [Python3][Python3]   | >= 3.7     | ‡ |

Utopia aims to allow rapid development, and is thus being tested only against the more recent releases of its dependencies.
The version numbers noted above are those made available by the Ubuntu release that Utopia is tested against.
Only version requirements that are marked with a `‡` are _enforced_ by the Utopia build system.

If you encounter difficulties with any of these dependencies, please [file an issue in the GitLab project][Utopia-new-issue].

To get Utopia running on a system with an earlier Ubuntu version, the above dependencies still need to be fulfilled.
You can use the [Ubuntu Package Search][Ubuntu-packages] to find the versions available on your system.
If a required version is not available, private package repositories may help to install a more recent version of a dependency.

If you cannot achieve these dependencies on your current installation, there is the option to use the Utopia [support branches](#support-branches), which provide compatibility with earlier versions of selected dependencies.



#### Python
Utopia's frontend, `utopya`, uses some additional python packages.

These packages and their dependencies are _automatically_ installed into a virtual environment when the `cmake ..` command is carried out during the [configuration step of the installation](#4-configure-and-build)).

| Software                 | Version    | Comments                        |
| ------------------------ | ---------- | ------------------------------- |
| [Sphinx][Sphinx]         | >= 2.0     | Builds the Utopia documentation |
| [paramspace][paramspace] | >= 2.4.1   | Makes parameter sweeps easy     |
| [dantro][dantro]         | ~= 0.13    | Handle, transform, and visualize hierarchically organized data |


#### Recommended
The following depencies are _recommended_ to be installed, but are not strictly required by Utopia:

| Software              | Version | Comments                          |
| --------------------- | ------- | --------------------------------- |
| [doxygen][doxygen]    | >= 1.8  | Builds the C++ code documentation |
| [graphviz][graphviz]  | >= 2.40 | Used by doxygen to create dependency diagrams |
| [ffmpeg][ffmpeg]      | >= 4.1  | Used for creating videos |
| [TBB][tbb]            | >= 2018.5  | Intel parallelization library |


### Support Branches
To retain compatibility with older releases of the dependencies, we maintain a selected set of [support branches](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/-/branches/all?utf8=✓&search=support%2F).
These branches all start with the `support/*` prefix and are associated with a specific Ubuntu version.
They are maintained with features from the `master` branch as long as it is feasible to do so.

To use a support branch, simply checkout the respective branch locally and rebuild Utopia:

```bash
# Checkout the branch
git checkout support/bionic
git pull

# Use a new build directory, then reconfigure and build Utopia
rm -rf build
mkdir build && cd build
cmake ..
make all -j4
```



<!-- ###################################################################### -->

## Troubleshooting
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

[paramspace]: https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace
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
