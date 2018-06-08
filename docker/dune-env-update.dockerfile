FROM ccees/utopia:bionic
LABEL maintainer="Lukas Riedel <lriedel@iup.uni-heidelberg.de>, \
                  Yunus Sevinchan <ysevinch@iup.uni-heidelberg.de>"
# number of cores for parallel builds
ARG PROCNUM=1

RUN apt-get clean && apt-get update && apt-get upgrade -y && apt-get clean
WORKDIR /opt/dune
RUN ./dune-common/bin/dunecontrol update
RUN MAKE_FLAGS="-j ${PROCNUM}" \
    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DDUNE_PYTHON_VIRTUALENV_SETUP=True -DDUNE_PYTHON_ALLOW_GET_PIP=True" \
    ./dune-common/bin/dunecontrol all
