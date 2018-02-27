FROM ubuntu:rolling
MAINTAINER Lukas Riedel

# install dependencies
RUN apt-get update \
    && apt-get install -y \
        cmake \
        doxygen \
        gcc \
        g++ \
        gfortran \
        git \
        locales \
        pkg-config \
        python3-dev \
    && apt-get clean

RUN rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

WORKDIR /opt/dune
RUN git clone https://gitlab.dune-project.org/core/dune-common.git
RUN git clone https://gitlab.dune-project.org/core/dune-geometry.git
RUN git clone https://gitlab.dune-project.org/core/dune-grid.git
RUN git clone https://gitlab.dune-project.org/staging/dune-uggrid.git

RUN CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release \\
    -DDUNE_PYTHON_VIRTUALENV_SETUP=True \\
    -DDUNE_PYTHON_ALLOW_GET_PIP=True" \
    ./dune-common/bin/dunecontrol all