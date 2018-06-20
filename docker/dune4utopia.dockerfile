FROM ubuntu:bionic

LABEL maintainer="Lukas Riedel <lriedel@iup.uni-heidelberg.de>, \
                  Yunus Sevinchan <ysevinch@iup.uni-heidelberg.de>"

# Define docker build arguments
# Number of cores for parallel builds
ARG PROCNUM=1

# Compilers to be used
ARG CC=gcc
ARG CXX=g++

# DUNE version to use
ARG DUNE_VERSION=2.6

# install dependencies
RUN apt-get update \
    && apt-get install -y \
        cmake \
        curl \
        doxygen \
        gcc \
        g++ \
        clang \
        gfortran \
        git \
        libboost-dev \
        libhdf5-dev \
        libyaml-cpp-dev \
        locales \
        pkg-config \
        python3-dev \
        python3-pip \
    && apt-get clean

# manage locales
RUN rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

# install custom CA of Heidelberg University to access our servers
RUN curl https://pki.pca.dfn.de/uni-heidelberg-ca/pub/cacert/chain.txt \
    -o /usr/local/share/ca-certificates/uniheidelberg.crt \
    && update-ca-certificates

# change working directory and clone DUNE dependencies
WORKDIR /opt/dune
RUN git clone https://gitlab.dune-project.org/core/dune-common.git \
    -b releases/${DUNE_VERSION}
RUN git clone https://gitlab.dune-project.org/core/dune-geometry.git \
    -b releases/${DUNE_VERSION}
RUN git clone https://gitlab.dune-project.org/core/dune-grid.git \
    -b releases/${DUNE_VERSION}
RUN git clone https://gitlab.dune-project.org/staging/dune-uggrid.git \
    -b releases/${DUNE_VERSION}

# build
RUN MAKE_FLAGS="-j ${PROCNUM}" \
    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DDUNE_PYTHON_VIRTUALENV_SETUP=True -DDUNE_PYTHON_ALLOW_GET_PIP=True" \
    ./dune-common/bin/dunecontrol all
