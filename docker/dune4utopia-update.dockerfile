# This dockerfile allows to update a base image, i.e. update all dependencies
# to new versions

# Add arguments used for selecting which image is to be updated
ARG BASE_IMAGE

# Define docker build arguments
# Number of cores for parallel builds
ARG PROCNUM=1

# Compilers to be used
ARG CC=gcc
ARG CXX=g++

# Now select the image that is to be used as the basis
FROM ${BASE_IMAGE}

# Maintainer info
LABEL maintainer="Lukas Riedel <lriedel@iup.uni-heidelberg.de>, \
                  Yunus Sevinchan <ysevinch@iup.uni-heidelberg.de>"

# Update all dependencies installed via apt
RUN apt-get clean && apt-get update && apt-get upgrade -y && apt-get clean

# Update DUNE
WORKDIR /opt/dune
RUN ./dune-common/bin/dunecontrol update

# ... and build it again
RUN MAKE_FLAGS="-j ${PROCNUM}" \
    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DDUNE_PYTHON_VIRTUALENV_SETUP=True -DDUNE_PYTHON_ALLOW_GET_PIP=True" \
    ./dune-common/bin/dunecontrol all
