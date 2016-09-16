# CITCAT

The __C__onveniently __I__ntegrated __C__ellular __A__utomaton __T__oolkit

Powered by [DUNE](https://dune-project.org/)

This module is the base library for the [iCAT](http://shangrila.iup.uni-heidelberg.de:30000/citcat/icat) package. It contains class and function templates, a documentation, and unit tests.

_CITCAT is not meant to be a stand-alone module. To build Cellular Automata models, use iCAT and place source files there._

## Installation
CITCAT is a DUNE module and thus relies on the [DUNE Buildsystem](https://www.dune-project.org/doc/installation/) for installation. Refer to the iCAT Installation Manual for information on how to install CITCAT on your machine.

Using [Docker](https://www.docker.com/), you can also download the ready-to-use [CITCAT DUNE Environment Image](https://hub.docker.com/r/citcat/dune-env/) from [DockerHub](https://hub.docker.com/). Clone CITCAT into the `/opt/dune/` directory and execute

    ./dune-common/bin/dunecontrol --only=citcat all

to build CITCAT.

Create the CITCAT documentation by entering the `build-cmake` directory and executing

    make doc
