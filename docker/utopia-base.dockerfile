# NOTE ------------------------------------------------------------------------
# IMPORTANT:
#     Do not forget to increment the UTOPIA_BASE_IMAGE_VERSION variable in the
#     docker/Makefile when making changes to this file!
#     The version is a simple counter that should be increment upon _any_
#     change in this file (+0.1 for small, +1.0 for breaking changes).
#     The version should be reset to 1 when a new Ubuntu version is used.
# -----------------------------------------------------------------------------

ARG UBUNTU_VERSION
FROM ubuntu:$UBUNTU_VERSION

LABEL maintainer="Utopia Developers <utopia-dev@iup.uni-heidelberg.de>"

ENV LANG en_US.utf8

RUN export DEBIAN_FRONTEND=noninteractive; \
    export DEBCONF_NONINTERACTIVE_SEEN=true; \
    echo 'tzdata tzdata/Areas select Etc' | debconf-set-selections; \
    echo 'tzdata tzdata/Zones/Etc select UTC' | debconf-set-selections; \
    apt-get update -y \
    && apt-get install -y --no-install-recommends \
    # general tools
        tzdata \
        curl \
        locales \
        git \
    # required packages
        pkg-config \
        cmake \
        gcc \
        g++ \
        gfortran \
        clang \
        libarmadillo-dev \
        libboost-dev \
        libboost-test-dev \
        libboost-graph-dev \
        libboost-regex-dev \
        libhdf5-dev \
        libspdlog-dev \
        libyaml-cpp-dev \
        python3-dev \
        python3-pip \
        python3-venv \
    # optional packages
        libtbb-dev \
        doxygen \
        ffmpeg \
        libgraphviz-dev \
        graphviz \
        texlive-latex-extra texlive-fonts-recommended dvipng cm-super \
    # cleanup to get smaller image size
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    # some final configurations
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8

# Maybe relevant in the future: Use a specific gcc version as compiler
# ENV CC=/usr/bin/gcc-12 CXX=/usr/bin/g++-12
