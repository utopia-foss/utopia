# This dockerfile is used to deploy a ready-to-use version of Utopia
# -----------------------------------------------------------------------------

# Select a base image via argument
ARG BASE_IMAGE
FROM ${BASE_IMAGE}

LABEL maintainer="Utopia Developers <utopia-dev@iup.uni-heidelberg.de>"

# Number of cores for parallel builds
ARG PROCNUM=1

# Compilers to be used
ARG CC=gcc
ARG CXX=g++

# commit or branch to check out and remote URL
ARG GIT_CHECKOUT=""
ARG GIT_REMOTE_URL=""


# End of argument definitions . . . . . . . . . . . . . . . . . . . . . . . . .

# Enter a new work directory and copy over all data from the build context
WORKDIR /utopia
COPY ./ ./

# Checkout a specific commit and set the origin URL as specified
RUN if [ -n "$GIT_CHECKOUT" ]; then git checkout ${GIT_CHECKOUT}; fi
RUN if [ -n "$GIT_REMOTE_URL" ]; then git remote set-url origin ${GIT_REMOTE_URL}; fi

# Configure and build Utopia
WORKDIR /utopia/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_PACKAGE_REGISTRY=On ..
RUN make -j${PROCNUM} all

# To make sure everything works as desired, make a quick run... and directly
# delete the data again.
RUN    ./run-in-utopia-env utopia run dummy \
    && rm -rf /root/utopia_output
# Will also create the font cache in /root/.cache
# Utopia is working now, but only for the root user.

# Set up the utopia user, which will be the future (non-root) USER
RUN groupadd -r utopia && useradd --no-log-init -rm -g utopia utopia

# Create the future WORKDIR
RUN mkdir -pv /home/utopia/io
# NOTE As the model registry is in ~/.config, the home directory can NOT be
#      mounted, as that would hide everything in it. We _need_ a subdirectory.
#      Also, it needs to be created here in order to be chowned (see below).

# Change the Utopia user configuration to write output to the future WORKDIR
RUN ./run-in-utopia-env utopia config user \
    --set paths.out_dir=/home/utopia/io/output
# Have model registry and utopia configuration in /root/.config/utopia now.

# Move the model registry files and any already created cache and cmake files
# from the root's home to the utopia user's home.
# Then, take care that everything the user interacts with is owned by them.
RUN    mv /root/.config /home/utopia/.config/  \
    && mv /root/.cache /home/utopia/.cache/    \
    && mv /root/.cmake /home/utopia/.cmake/    \
    && chown -R utopia:utopia /utopia          \
    && chown -R utopia:utopia /home/utopia
# Utopia works for the utopia user now

# Now, make things homely for the utopia user. <3
# Everyone want's a color-capable terminal, right?
ENV TERM=xterm-256color

# As a convenience, install ipython and jupyter, adding the utopia-env kernel
RUN    ./run-in-utopia-env pip install ipython jupyter \
    && ./run-in-utopia-env python -m IPython kernel install --user --name=utopia-env

# Finally, switch to the utopia user and select the mountable IO directory
USER utopia
WORKDIR /home/utopia/io

# As entrypoint, enter the shell from within the utopia-env
ENTRYPOINT [ "/utopia/build/run-in-utopia-env", "/bin/bash" ]
