![Utopia Logo](doc/_static/images/logo_blue_full.svg)

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

The [`utopya` Python package][utopya] constitutes Utopia's frontend.
It configures and performs the simulation runs and embeds the model into a data processing pipeline, such that the simulation data can directly be analyzed and visualized.
All parts of the frontend make use of a hierarchic, recursively-updated YAML-based configuration structure.
Using the [paramspace package][paramspace], this allows to easily define parameter sweeps, which can then be carried out as simultaneous simulations on massively parallel, distributed machines.
The [dantro][dantro]-based data processing pipeline automates visualization, thereby coupling the model implementation and its analysis closer together.

Several models are readily included in the framework, among them Conway's Game of Life, one CA- and one agent-based contagious disease model, and a network-based model of social opinion dynamics.
Investigating these models by performing simulation runs with a few varying parameters, or sensitivity analysis over a large parameter space, requires little to no programming skills.

For introductory guides, feature lists, FAQs, and API references refer to the online [user manual at docs.utopia-project.org][Utopia-docs]

Utopia development happens in the [`utopia-project` group on GitLab.com](https://gitlab.com/utopia-project).
Additionally, you can retrieve the code and get in contact via a mirrored repository on [GitHub][Utopia-Github].
Note that we *used* to host our own GitLab instance at `ts-gitlab.iup.uni-heidelberg.de`; traces of that past development platform may still be around this project somewhere ... 👻

### How to cite Utopia
Utopia was reviewed and published in the [Journal of Open Source Software (JOSS)](https://joss.theoj.org/).
Please cite at least the following publication if you use Utopia (or a modified version thereof) for your own work:

> Lukas Riedel, Benjamin Herdeanu, Harald Mack, Yunus Sevinchan, and Julian Weninger. 2020. **“Utopia: A Comprehensive and Collaborative Modeling Framework for Complex and Evolving Systems.”** *Journal of Open Source Software* 5 (53): 2165. DOI: [10.21105/joss.02165](https://doi.org/10.21105/joss.02165).

The [`CITATION.cff`](CITATION.cff) file in this repository follows the [citation file format](https://citation-file-format.github.io/) and contains additional metadata to reference this software, its authors, and associated publications.
See the [user manual](https://docs.utopia-project.org/html/cite.html) for more information and BibTeX data.

### Contents of this README
* [How to install](#how-to-install)
    * [On your machine](#step-by-step-instructions)
    * [Alternative: docker image](#utopia-via-docker)
* [Quickstart](#quickstart)
* [Documentation and Guides](#utopia-documentation)
* [Information for Developers](#information-for-developers)
    * [Testing](#testing)
    * [Setting up a separate models repository](#setting-up-a-separate-repository-for-models)
* [Dependencies](#dependencies)
* [Troubleshooting](#troubleshooting)

---


<!-- ###################################################################### -->
<!-- marker-installation-instructions -->

## How to install
The following instructions will install Utopia into a development environment on your machine.
If you simply want to _run_ Utopia, you can do so via a [ready-to-use docker image][Utopia-docker]; see [below](#utopia-via-docker) for more information.


### Step-by-step Instructions
These instructions are intended for 'clean' __macOS__ (both Intel and Apple Silicon) or recent __Ubuntu__ setups.
Since __Windows__ supports the installation of Ubuntu via [Windows Subsystem for Linux](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux), Utopia can also be used on Windows.
Follow the [WSL Installation Guide](https://docs.microsoft.com/en-us/windows/wsl/install-win10) to install Ubuntu, then follow the instructions for Ubuntu in this README.

**Note:** Utopia is always tested against a recent Ubuntu release, currently [Ubuntu 22.04](https://releases.ubuntu.com/22.04/).
However, you can use Utopia with any earlier release, as long as the [dependencies](#dependencies) can be fulfilled.

⚠️ If you encounter difficulties, have a look at the [**troubleshooting section**](#troubleshooting).
If this does not resolve your installation problems, [please file an issue in the GitLab project][Utopia-new-issue].


#### 1 — Clone Utopia
First, create a `Utopia` directory at a place of your choice.
This is where the Utopia repository will be cloned to.
When working with or developing for Utopia, auxiliary data will have a place there as well.

In your terminal, enter the `Utopia` directory you just created and invoke the clone command:

```bash
git clone https://gitlab.com/utopia-project/utopia.git
```

Alternatively, you can clone via SSH, using the address from the "Clone" button on the [project page][Utopia].

After cloning, there will be a new `utopia` directory (mirroring this repository) inside your top-level `Utopia` directory.



#### 2 — Install dependencies
Install the third-party dependencies using a package manager.

**Note:** If you have [Anaconda][Anaconda] installed, you already have a working Python installation on your system, and you can omit installing the `python` packages below.
However, notice that there might be issues during [the configuration step](#4-configure-and-build).
Have a look at the [troubleshooting](#troubleshooting) section to see how to address them.

##### On Ubuntu (22.04)
```bash
apt update
apt install cmake gcc g++ gfortran git libarmadillo-dev libboost-all-dev \
            libhdf5-dev libspdlog-dev libyaml-cpp-dev pkg-config \
            python3-dev python3-pip python3-venv
```

Further, we recommend installing the following optional packages:

```bash
apt update
apt install ffmpeg graphviz doxygen
```

You will _probably_ need administrator rights.

**Note:** For simplicity, we suggest installing the meta-package `libboost-all-dev` which includes the whole [Boost][Boost] library.
If you want a minimal installation (only the [strictly required components](#dependencies)), use the following packages instead:
`libboost-dev libboost-test-dev libboost-graph-dev libboost-regex-dev`.


##### On macOS
First, install the Apple Command Line Tools:

```bash
xcode-select --install
```

There are two popular package managers on macOS, [Homebrew][Homebrew] and [MacPorts][MacPorts]; we recommend to use Homebrew.
Here are the installation instructions for both:

* **Homebrew**:

    Install the required packages:

    ```bash
    brew update
    brew install armadillo boost cmake hdf5 pkg-config python3 spdlog yaml-cpp
    ```

    Further, we recommend installing the following optional packages:

    ```bash
    brew update
    brew install ffmpeg graphviz doxygen
    ```

* **MacPorts**:

    Please be aware that `port` commands typically require administrator rights (`sudo`).

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

    Further, we recommend installing the following optional packages:

    ```bash
    port install ffmpeg graphviz doxygen
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

**Note:** If you are using **MacPorts**, append the location of your Python installation to the CMake command (this is only required when calling CMake on a clean `build` directory):

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

**Note:** If you are using `csh` or `fish` shells, use the respective `activate` scripts located in `build/utopia-env/bin/` (see [below](#how-to-run-a-model)).

The `utopia` command is now available and gives you control over running and evaluating model simulations:

```bash
utopia run dummy
```

The model output will be written into `~/utopia_output/dummy/<timestamp>`.
For more information on how to use the command line interface, see the [information for users](#how-to-run-a-model) below and the [documentation][Utopia-docs].


#### 5 — Make sure everything works
_This step is optional, but recommended._

To make sure that Utopia works as expected on your machine, [build and carry out](https://docs.utopia-project.org/html/usage/implement/running-tests.html) all tests.

### Optional Installation Steps

The following instructions will enable additional, *optional* features of Utopia.

#### Enable Multithreading in Utopia Models

1. Install the optional dependencies for multithreading.

    * On **Ubuntu**, we recommend using the GCC compiler with Intel TBB:

        ```bash
        apt update && apt install libtbb-dev
        ```

        Alternatively, one may install the Intel oneAPI base toolkit following these [installation instructions](https://www.intel.com/content/www/us/en/develop/documentation/installation-guide-for-intel-oneapi-toolkits-linux/top/installation/install-using-package-managers/apt.html).
        The only required package is `intel-basekit`.
        It includes the oneDPL library we use for parallelization.

    * On **macOS**, with **Homebrew** (please mind the note below!):

        ```bash
        brew update && brew install onedpl
        ```

    * On **macOS** with **MacPorts**, we are [currently unsure](https://gitlab.com/utopia-project/utopia/-/issues/254) whether multithreading is workable.

1. Enter the Utopia build directory, and call CMake again. This time, enable the use of multithreading with the `MULTITHREADING` option:

    ```bash
    cd build
    cmake -DMULTITHREADING=On ..
    ```

    At the end of its output, CMake should now report that the "Multithreading" feature has been enabled.
    If the requirements for multithreading are not met, however, the build will fail.

1. Re-compile the models. Inside the build directory, call

    ```bash
    make all
    ```

1. Depending on the algorithms used inside the respective model, it will automatically exploit the multithreading capabilities of your system when executed! 🎉


### Utopia via Docker
[Docker][docker] is a free OS-level virtualization software.
It allows running any application in a closed environment container.

The Utopia docker image is a way to run Utopia models and evaluate them without having to go through the installation procedure.
It is also suitable for framework and model development.

The image and instructions on how to create a container from it can be found on the [`ccees/utopia` docker hub page][Utopia-docker].




<!-- ###################################################################### -->

## Quickstart
This section gives a glimpse into working with Utopia.
It's not more than a glimpse; after playing around with this, [consult the documentation][Utopia-docs] to gain access to more information material, and especially: the [**Utopia Tutorial**][Utopia-tutorial].


### How to run a model?
The Utopia command line interface (CLI) is, by default, only available in a Python virtual environment, in which `utopya` (the Utopia frontend) and its dependencies are installed.
To conveniently work with the frontend, you should thus enter the virtual environment.
Execute *one* of the commands below, depending on which type of shell you're using:

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

**Note:**
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

The best place to continue from here is the [tutorial][Utopia-tutorial].




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

To do that, follow the linked instructions to [generate a key pair](https://docs.gitlab.com/ee/user/ssh.html#generate-an-ssh-key-pair)
and to [add a key to your GitLab account](https://docs.gitlab.com/ee/user/ssh.html#add-an-ssh-key-to-your-gitlab-account).

### Building the documentation locally
To build the documentation locally, first make sure that all submodules are downloaded, because they are needed for building the documentation:

```bash
git submodule update --init --recursive
```

Then navigate to the `build` directory and run

```bash
make doc
```

In case you also want to generate (some of) the figures that are embedded into the documentation, set the `UTOPIA_DOC_GENERATE_FIGURES` environment variable:

```bash
UTOPIA_DOC_GENERATE_FIGURES=True make doc
```

After building, carrying out a link check and running some documentation tests is advisable:

```bash
make check_docs
```

The [Sphinx][Sphinx]-built user documentation will then be located at `build/doc/html/`.
The C++ [doxygen][doxygen]-documentation can be found at `build/doc/doxygen/html/`.
Open the respective `index.html` files to browse the documentation.

### Choosing a different compiler
CMake will inspect your system paths and use the default compiler.
You can use the `CC` and `CXX` environment variables to select a specific C and C++ compiler, respectively.
As compiler paths are cached by CMake, changing the compiler requires you to delete the `build` directory and re-run CMake.

Ubuntu 22.04 LTS (Jammy Jellyfish), for example, provides [GCC versions 10 to 12][Ubuntu-packages].
To use GCC 12, for example, first install it via APT:

```bash
apt update && apt install gcc-12 g++-12
```

Then create the build directory, enter it, and set the `CC` and `CXX` environment variables when calling CMake:

```bash
mkdir build && cd build
CC=/usr/bin/gcc-12 CXX=/usr/bin/g++-12 cmake ..
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
Utopia contains unit tests to ensure consistency by checking whether class members
and functions are working correctly. This is done both for the C++ and the
Python code.
The tests are integrated into the GitLab Continuous Integration pipeline,
meaning that tests are run upon every push to the project and failing tests
can be easily detected.
Tests can also be executed locally, to test a (possibly altered) version of
Utopia *before* committing and pushing changes to the GitLab.

See [the user manual](https://docs.utopia-project.org/html/usage/implement/running-tests.html) for more details on how to run tests.

### Setting up a separate repository for models
Working inside a clone or a fork of this repository is generally *not* a good idea: it makes updating harder, prohibits efficient version control on the models, and makes it more difficult to include additional dependencies or code.

To make setting up such a project repository as easy as possible, we provide a template repository, which can be used to start a new Utopia project!
Follow the instructions in [the `models_template` project][models_template] for more information.

Make sure to call the following command to export package information to the CMake project registry, such that a separate repository can find Utopia:

```bash
cmake -DCMAKE_EXPORT_PACKAGE_REGISTRY=On ..
```




<!-- ###################################################################### -->

## Dependencies

| Software                                       | Required Version | Tested Version  | Comments     |
| ---------------------------------------------- | ---------------- | --------------- | ------------ |
| GCC                                            | >= 9             | 11.2            | Full C++17 support required |
| _or:_ clang                                    | >= 9             | 14.0            | Full C++17 support required |
| _or:_ Apple LLVM                               | >= 9             |                 | Full C++17 support required |
| [CMake](https://cmake.org/)                    | >= 3.16          | 3.22            | |
| pkg-config                                     | >= 0.29          | 0.29            | |
| [HDF5](https://www.hdfgroup.org/solutions/hdf5/)  | >= 1.10.4     | 1.10.7          | |
| [Boost](http://www.boost.org/)                 | >= 1.67          | 1.74            | required components: `graph`, `regex` and `unit_test_framework` |
| [Armadillo](http://arma.sourceforge.net/)      | >= 9.600         | 10.8.2          | |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | >= 0.6.2         | 0.7.0           | |
| [spdlog](https://github.com/gabime/spdlog)     | >= 1.3           | 1.9.2           | |
| [Python3](https://www.python.org/downloads/)   | >= 3.6           | 3.10.4          | |

Utopia aims to allow rapid development, and is thus being tested only against the more recent releases of its dependencies.
Currently, Utopia is tested against the packages provided by [**Ubuntu 22.04**][Ubuntu-packages].
However the above version _requirements_ (i.e., those _enforced_ by the build system) can be fulfilled also with Ubuntu 19.10.

To get Utopia running on a system with an earlier Ubuntu version, the above dependencies still need to be fulfilled.
You can use the [Ubuntu Package Search][Ubuntu-packages] to find the versions available on your system.
If a required version is not available, private package repositories may help to install a more recent version of a dependency.

If you encounter difficulties with the installation procedure for any of these dependencies, please [file an issue in the GitLab project][Utopia-new-issue].


### Python
Utopia also has some Python dependencies, which are _automatically_ installed during the [configuration step of the installation](#3-configure-and-build).

The following table includes the most important Python dependencies:

| Software                  | Version  | Comments                        |
| ------------------------- | -------- | ------------------------------- |
| [utopya](https://gitlab.com/utopia-project/utopya)   | >= 1.1   | The (outsourced) Utopia frontend package |
| [dantro](https://gitlab.com/utopia-project/dantro)   | >= 0.18  | Handle, transform, and visualize hierarchically organized data |
| [paramspace](https://gitlab.com/blsqr/paramspace)    | >= 2.5   | Makes parameter sweeps easy |

In addition, the following packages are _optionally_ used for development of the framework or its models.

| Software                  | Version   | Comments                        |
| ------------------------- | --------- | ------------------------------- |
| [pytest](https://docs.pytest.org/)    |          | For model tests |
| [pre-commit](https://pre-commit.com)  | >= 2.18  | For pre-commit hooks |
| [black](https://github.com/psf/black) | >= 22.6  | For formatting python code |
| [Sphinx](https://www.sphinx-doc.org/) | == 4.5.* | Builds the Utopia documentation |

These requirements are defined in the `.utopia-env-requirements.txt` file; in case installation fails, a warning will be emitted during [configuration](#3-configure-and-build).


### Recommended
The following depencies are _recommended_ to be installed, but are not strictly required by Utopia:

| Software                  | Version  | Comments                        |
| ------------------------- | -------- | ------------------------------- |
| [doxygen](http://www.doxygen.nl)                      | >= 1.8  | Builds the C++ code documentation |
| [graphviz](https://graphviz.gitlab.io)                | >= 2.40 | Used by doxygen to create dependency diagrams |
| [ffmpeg](https://www.ffmpeg.org)                      | >= 4.1  | Used for creating videos |
| [Intel oneDPL](https://www.intel.com/content/www/us/en/developer/tools/oneapi/dpc-library.html) |  | Intel parallelization library |
| [TBB](https://github.com/oneapi-src/oneTBB)           | >= 2018.5  | Intel multithreading library |


<!-- ###################################################################### -->

## Troubleshooting

* If you have a previous installation and the **build fails inexplicably**,
    removing the `build` directory completely and starting anew from the
    [configuration step](#3-configure-and-build) should help.  
    In cases where the installation _used_ to work but at some point _stopped_ working, this should be a general remedy.
    If the problem does *not* seem to be related to the Python environment, deleting only `build/CMakeCache.txt` may already suffice and save some configuration time.

* If you have trouble with more recent **HDF5** versions on macOS, one workaround is to use an older version:
    
    ```bash
    brew install hdf5@1.10
    brew link hdf5@1.10 --overwrite
    ```

* **Anaconda** ships its own version of the HDF5 library which is _not_
    compatible with Utopia. To tell CMake where to find the correct version of
    the library, add the following argument (without the comments!) to the
    `cmake ..` command during [configuration](#3-configure-and-build):

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

[Utopia]: https://gitlab.com/utopia-project/utopia
[Utopia-Gitlab]: https://gitlab.com/utopia-project/utopia
[Utopia-Github]: https://github.com/utopia-foss/utopia
[models_template]: https://gitlab.com/utopia-project/models_template

[Utopia-issues]: https://gitlab.com/utopia-project/utopia/issues
[Utopia-new-issue]: https://gitlab.com/utopia-project/utopia/issues/new?issue

[Utopia-docs]: https://docs.utopia-project.org/html/index.html
[Utopia-tutorial]: https://docs.utopia-project.org/html/getting_started/tutorial.html
[Utopia-docker]: https://hub.docker.com/r/ccees/utopia

[utopya]: https://gitlab.com/utopia-project/utopya
[paramspace]: https://gitlab.com/blsqr/paramspace
[dantro]: https://gitlab.com/utopia-project/dantro

[LGPL]: https://www.gnu.org/licenses/lgpl-3.0.en.html
[Ubuntu-packages]: https://packages.ubuntu.com
[docker]: https://www.docker.com/
[Homebrew]: https://brew.sh/
[MacPorts]: https://www.macports.org/
[Anaconda]: https://www.anaconda.com/

<!-- These shortlinks are used in the text but cannot be used in the table (due to this file being embedded via sphinx) -->
<!-- TODO Check if this is still the case -->
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
[pytest-usage]: https://docs.pytest.org/en/stable/usage.html
[pytest-cov]: https://github.com/pytest-dev/pytest-cov
[tbb]: https://github.com/oneapi-src/oneTBB
[pstl]: https://github.com/oneapi-src/oneDPL
