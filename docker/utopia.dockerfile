# This dockerfile is used to deploy a ready-to-use version of Utopia
# -----------------------------------------------------------------------------

# Make base image configurable by arguments
ARG BASE_IMAGE=ccees/utopia-base

# Select the image that is to be used as the basis
FROM ${BASE_IMAGE} AS build-env

# Maintainer info
LABEL maintainer="Lukas Riedel <lriedel@iup.uni-heidelberg.de>, \
                  Yunus Sevinchan <ysevinch@iup.uni-heidelberg.de>"

# The key with which to access the Utopia repository and dependencies
ARG DEPLOY_KEY

# Number of cores for parallel builds
ARG PROCNUM=1

# Compilers to be used
ARG CC=gcc
ARG CXX=g++
# End of argument definitions . . . . . . . . . . . . . . . . . . . . . . . . .

# Store the deploy key in the SSH config and invoke the ssh agent
# NOTE As the final image is a separate stage, the key file is super-safe.
RUN    mkdir ~/.ssh/ \
    && echo "${DEPLOY_KEY}" > ~/.ssh/id_rsa \
    && chmod 700 ~/.ssh/* \
    && eval $(ssh-agent -s)
RUN    touch ~/.ssh/known_hosts \
    && ssh-keyscan -p 10022 ts-gitlab.iup.uni-heidelberg.de \
       >> ~/.ssh/known_hosts

# Enter a new work directory and copy over all data from the build context
WORKDIR /utopia
COPY ./ ./

# Configure and build Utopia
WORKDIR /utopia/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && make -j${PROCNUM} all

# Change user configuration to write output into the (future) working directory
RUN /utopia/build/run-in-utopia-env utopia config user \
    --set paths.out_dir=/home/utopia/io/output
# Have model registry and utopia configuration in /root/.config/utopia now.

# To make sure everything works as desired, make a quick run; the layer created
# by this does not end up in the production environment, so that's fine.
RUN /utopia/build/run-in-utopia-env utopia run dummy


# Production Environment ......................................................
# Start with a fresh image
FROM ${BASE_IMAGE} AS prod-env
# NOTE With this, only data that is explicitly copied over into this stage is
#      available in the final image; makes DEPLOY_KEY super-safe! :)

# The following section copies files over from the build environment . . . . .
# Copy build directory (model binaries, utopia-env) and symlinked python code.
WORKDIR /utopia
COPY --from=build-env /utopia/build ./build/
COPY --from=build-env /utopia/python ./python/

# Copy all models' configuration files (model defaults, plots, base plots)
COPY --from=build-env /utopia/src ./tmp/
COPY ./docker/copy_rec.py ./docker/copy_rec.py
RUN    python3 docker/copy_rec.py -vr "**/*.yml" ./tmp/ ./src \
    && rm -r ./tmp

# Add the utopia group and its user; will be the future USER
RUN groupadd -r utopia && useradd --no-log-init -rm -g utopia utopia
# Have its home directory available now.

# Copy the model registry files from root's home to the utopia user's home
COPY --from=build-env /root/.config /home/utopia/.config/

# Also copy the already created font cache, such that it need not be built on
# first invocation of the Utopia CLI ...
COPY --from=build-env /root/.cache /home/utopia/.cache/

# Done carrying over files now. . . . . . . . . . . . . . . . . . . . . . . . .
# Now, make things homely for the utopia user. <3
# Everyone want's a color-capable terminal, right?
ENV TERM=xterm-256color

# ... and a proper working directory.
RUN mkdir -v /home/utopia/io
# NOTE As the model registry is in ~/.config, the home directory can NOT be
#      mounted, as it would hide everything in it. We _need_ a subdirectory.
#      Also, creating this now allows to set the rights for it; if it would be
#      created by WORKDIR below, it would be owned by root, for some reason.

# Take care that everything the user interacts with is owned by them.
RUN    chown -R utopia:utopia /utopia \
    && chown -R utopia:utopia /home/utopia

# As a convenience, install ipython and jupyter and add the utopia-env kernel
RUN /utopia/build/run-in-utopia-env pip install ipython jupyter
RUN /utopia/build/run-in-utopia-env python -m IPython kernel install --user --name=utopia-env

# Finally, switch to the utopia user and select the working directory
USER utopia
WORKDIR /home/utopia/io

# As entrypoint, enter the shell such that the virtual env is always there
ENTRYPOINT [ "/utopia/build/run-in-utopia-env", "/bin/bash" ]
