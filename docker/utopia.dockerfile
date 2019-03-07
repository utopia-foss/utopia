# This dockerfile is used to deploy Utopia

# Add arguments used for selecting which image is to be updated
ARG BASE_IMAGE

# Now select the image that is to be used as the basis
FROM ${BASE_IMAGE}

# Maintainer info
LABEL maintainer="Lukas Riedel <lriedel@iup.uni-heidelberg.de>, \
                  Yunus Sevinchan <ysevinch@iup.uni-heidelberg.de>"

# The key with which to access the Utopia repository and GitLab
ARG SSH_PRIVATE_KEY

# Number of cores for parallel builds
ARG PROCNUM=1

# Compilers to be used
ARG CC=gcc
ARG CXX=g++

# TODO
#   1. Setup ssh-agent
#   2. Retrieve Utopia repository
#   3. Configure Utopia, installing dependencies etc.
#   4. Build models
#   5. Remove all code and the SSH_PRIVATE_KEY again
