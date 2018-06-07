FROM ubuntu:bionic
LABEL maintainer="Lukas Riedel <lriedel@iup.uni-heidelberg.de>, \
                  Yunus Sevinchan <ysevinch@iup.uni-heidelberg.de>"

# install dependencies
RUN apt-get update \
    && apt-get install -y \
        cmake \
        curl \
        doxygen \
        gcc \
        g++ \
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
RUN git clone https://gitlab.dune-project.org/core/dune-common.git
RUN git clone https://gitlab.dune-project.org/core/dune-geometry.git
RUN git clone https://gitlab.dune-project.org/core/dune-grid.git
RUN git clone https://gitlab.dune-project.org/staging/dune-uggrid.git

# build
RUN CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release \\
    -DDUNE_PYTHON_VIRTUALENV_SETUP=True \\
    -DDUNE_PYTHON_ALLOW_GET_PIP=True" \
    ./dune-common/bin/dunecontrol all
