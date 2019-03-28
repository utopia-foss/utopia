# This dockerfile is used to deploy Utopia

# Add arguments used for selecting which image is to be updated
ARG BASE_IMAGE=ccees/utopia-base

# Now select the image that is to be used as the basis
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

# Store the deploy key in the SSH config.
# NOTE One still needs to invoke the ssh-agent in order to have SSH access!
# NOTE As the final image is a separate stage, the key file is super-safe.
RUN mkdir ~/.ssh/ \
    && echo "${DEPLOY_KEY}" > ~/.ssh/id_rsa \
    && chmod 700 ~/.ssh/* \
    && eval $(ssh-agent -s)
RUN touch ~/.ssh/known_hosts \
    && ssh-keyscan -p 10022 ts-gitlab.iup.uni-heidelberg.de \
       >> ~/.ssh/known_hosts

# Enter a new work directory and copy over all data from the build context
WORKDIR /utopia
COPY ./ ./

# Configure and build Utopia, first initializing the ssh-agent
WORKDIR /utopia/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make -j${PROCNUM} all

# Start a fresh image as production environment
# NOTE: This is REQUIRED to ensure that the DEPLOY_KEY is indeed super-safe!
FROM ${BASE_IMAGE} AS prod-env

# Copy the relevant files
# Note: The paths must be the same as before because Python packages are
#       installed editable (with global paths)
WORKDIR /utopia/
COPY --from=build-env /utopia/build ./build/
COPY --from=build-env /utopia/python ./python/

# Copy models but only retain model and plot configs
COPY --from=build-env /utopia/src ./temp/
COPY ./docker/copy_rec.py ./docker/copy_rec.py
RUN python3 docker/copy_rec.py -vr "**/*.yml" ./temp/ ./src \
    && rm -r ./temp

# Switch to new user
RUN groupadd -r utopia && useradd --no-log-init -rm -g utopia utopia
USER utopia

# Set up working directory and entrypoint
WORKDIR /home/utopia
ENTRYPOINT [ "/utopia/build/run-in-utopia-env", "/bin/bash" ]
