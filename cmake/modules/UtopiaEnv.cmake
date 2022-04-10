#
# Create the utopia-env, a Python virtual environment for Utopia
#
# This module set the following variables:
#
#   - UTOPIA_ENV_DIR            Path to the venv installation
#   - UTOPIA_ENV_EXECUTABLE     Python interpreter executable inside the venv
#   - UTOPIA_ENV_PIP            Pip module executable inside the venv
#   - RUN_IN_UTOPIA_ENV         Binary for executing a single command in a
#                               shell configured with the virtual env.
#

# Search for Python
find_package(Python 3.7 REQUIRED)

# search for a Bash environment
find_package(UnixCommands QUIET)
if (NOT DEFINED BASH)
    message(SEND_ERROR "Bash is required to execute commands in the virtual "
                       "environment")
endif ()

# the designated paths for the virtual env and some environments.
# some might not exist yet at this point, are created below
set (UTOPIA_ENV_DIR ${CMAKE_BINARY_DIR}/utopia-env)
set (UTOPIA_ENV_EXECUTABLE ${UTOPIA_ENV_DIR}/bin/python)
set (UTOPIA_ENV_PIP ${UTOPIA_ENV_DIR}/bin/pip)

message(STATUS "")
message(STATUS "Setting up the utopia-env ...")

execute_process(
    COMMAND ${Python_EXECUTABLE} -m venv ${UTOPIA_ENV_DIR}
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_QUIET
)
if (NOT RETURN_VALUE EQUAL "0")
    message(FATAL_ERROR "Error creating the utopia-env:  ${RETURN_VALUE}")
endif ()

# create a symlink to the activation script
file (CREATE_LINK ${UTOPIA_ENV_DIR}/bin/activate
                  ${CMAKE_BINARY_DIR}/activate
      SYMBOLIC)

# write the convenience bash script
# configure the script
configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/run-in-utopia-env.in
                ${CMAKE_BINARY_DIR}/cmake/scripts/run-in-utopia-env
                @ONLY)
# copy the script, changing file permissions
file (COPY ${CMAKE_BINARY_DIR}/cmake/scripts/run-in-utopia-env
        DESTINATION ${CMAKE_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)

set (RUN_IN_UTOPIA_ENV ${CMAKE_BINARY_DIR}/run-in-utopia-env)


# -- virtual environment installed now, but remains to be populated -----------

# update pip and installation packages only if they are below a certain version
# number; updating everytime is unnecessary and takes too long... while this
# _should_ be taken care of by pip itself, it is sometimes not done on Ubuntu
# systems, leading to failure to install wheel-requiring downstream packages

# -- pip
python_find_package(PACKAGE pip VERSION 22.0)
if (NOT PYTHON_PACKAGE_pip_FOUND)
    python_install_package_pip(PACKAGE pip)
endif()

# -- setuptools
python_find_package(PACKAGE setuptools VERSION 51.1)
if (NOT PYTHON_PACKAGE_setuptools_FOUND)
    python_install_package_pip(PACKAGE setuptools)
endif()

# -- wheel
python_find_package(PACKAGE wheel VERSION 0.35)
if (NOT PYTHON_PACKAGE_wheel_FOUND)
    python_install_package_pip(PACKAGE wheel)
endif()

# -- utopya
# TODO Check if there is a more sensible spot to define these defaults
# TODO Is this the correct way to check for no values being set?

if("${UTOPYA_URL}" STREQUAL "")
    set(UTOPYA_URL https://gitlab.com/utopia-project/utopya.git)
endif()

if("${UTOPYA_BRANCH}" STREQUAL "")
    set(UTOPYA_BRANCH smoothe-out-integration-into-utopia)  # FIXME set to main
endif()

if("${UTOPYA_MIN_VERSION}" STREQUAL "")
    set(UTOPYA_MIN_VERSION 1.0.0a3)   # FIXME Update
endif()

python_find_package(PACKAGE utopya VERSION ${UTOPYA_MIN_VERSION})
if(NOT PYTHON_PACKAGE_utopya_FOUND)
    python_install_package_remote(
        URL ${UTOPYA_URL}
        BRANCH ${UTOPYA_BRANCH}
        EGG_NAME utopya
    )
    # TODO Consider replacing with requirements file? ... but need to avoid
    #      repetitive version checks on each cmake call!
endif()

# -- utopya CLI
# Create a (relative) symlink in the utopia-env: utopia -> utopya
if (NOT IS_SYMLINK ${UTOPIA_ENV_DIR}/bin/utopia)
    file(CREATE_LINK utopya ${UTOPIA_ENV_DIR}/bin/utopia SYMBOLIC)
endif()
# TODO Consider installing a custom wrapper script that advertises the CLI not
#      as "utopya" but as Utopia ...

# -- dantro
# Inform about dantro version (installation happened via utopya dependencies)
python_find_package(PACKAGE dantro)


# -- pre-commit and hooks
message(STATUS "")
message(STATUS "Setting up pre-commit hooks ...")

python_find_package(PACKAGE pre-commit VERSION 2.18)
if (NOT PYTHON_PACKAGE_pre-commit_FOUND)
    python_install_package_pip(PACKAGE pre-commit)
endif()

execute_process(
    COMMAND ${RUN_IN_UTOPIA_ENV} pre-commit install
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_QUIET
)
if (NOT RETURN_VALUE EQUAL "0")
    message(SEND_ERROR "Failed setting up pre-commit hooks: ${RETURN_VALUE}")
endif ()



message(STATUS "")
message(STATUS "utopia-env fully set up now.\n")
