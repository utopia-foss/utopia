# NOTE ------------------------------------------------------------------------
# IMPORTANT:
#     Do not forget to increment the $BASE_IMAGE_VERSION variable in gitlab CI
#     The version has the shape x.y, where x is incremented only for changes
#     that are not backwards-compatible
# -----------------------------------------------------------------------------

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
        clang \
        gfortran \
        git \
        libarmadillo-dev \
        libboost-dev \
        libhdf5-dev \
        locales \
        pkg-config \
        python3-dev \
        python3-pip \
        python3-venv \
        ffmpeg \
        libfftw3-dev \
    && apt-get clean

# manage locales
RUN rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8
