# NOTE ------------------------------------------------------------------------
# IMPORTANT:
#     Do not forget to increment the $BASE_IMAGE_VERSION variable in the
#     .gitlab-ci.yml when making changes to this file!
#     The version is a simple counter that should be increment upon _any_
#     change in this file. It should be reset to 1 when a new Ubuntu version
#     is used.
# -----------------------------------------------------------------------------

ARG UBUNTU_VERSION
FROM ubuntu:$UBUNTU_VERSION

LABEL maintainer="Utopia Developers <utopia-dev@iup.uni-heidelberg.de>"

# install dependencies
RUN export DEBIAN_FRONTEND=noninteractive; \
    export DEBCONF_NONINTERACTIVE_SEEN=true; \
    echo 'tzdata tzdata/Areas select Etc' | debconf-set-selections; \
    echo 'tzdata tzdata/Zones/Etc select UTC' | debconf-set-selections; \
    apt-get update -y \
    && apt-get install -y \
        tzdata \
    && apt-get install -y \
        curl \
        locales \
        pkg-config \
        git \
        cmake \
        doxygen \
        gcc-10 \
        g++-10 \
        gfortran-10 \
        clang \
        libarmadillo-dev \
        libboost-dev \
        libboost-test-dev \
        libboost-graph-dev \
        libboost-regex-dev \
        libhdf5-dev \
        libspdlog-dev \
        # NOTE: Clang package currently depends on wrong lib version
        libstdc++-10-dev \
        libtbb-dev \
        libyaml-cpp-dev \
        python3-dev \
        python3-pip \
        python3-venv \
        ffmpeg \
    && apt-get install -y \
        libgraphviz-dev \
        graphviz \
        vim \
        nano \
    && apt-get clean
# NOTE Not all packages above are _required_ by Utopia; some are added for
#      convenience in downstream images, like: vim and nano

# Make CMake use GCC 10 as compiler
ENV CC=/usr/bin/gcc-10 CXX=/usr/bin/g++-10

# manage locales
RUN rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8
