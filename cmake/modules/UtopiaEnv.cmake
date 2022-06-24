#
# Create the utopia-env, a Python virtual environment for Utopia
#
# This module sets the following variables:
#
#   - UTOPIA_ENV_DIR            Path to the venv installation
#   - UTOPIA_ENV_EXECUTABLE     Python interpreter executable inside the venv
#   - UTOPIA_ENV_PIP            Pip module executable inside the venv
#   - RUN_IN_UTOPIA_ENV         Binary for executing a single command in a
#                               shell configured with the virtual env.
#   - UTOPYA_FROM_PYPI          Whether to install utopya from PyPI
#                               (default: True)
#   - UTOPYA_URL                The URL from which to install utopya
#   - UTOPYA_BRANCH             The branch from which to install utopya
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
set(UTOPIA_ENV_DIR ${CMAKE_BINARY_DIR}/utopia-env)
set(UTOPIA_ENV_EXECUTABLE ${UTOPIA_ENV_DIR}/bin/python)
set(UTOPIA_ENV_PIP ${UTOPIA_ENV_DIR}/bin/pip)

message(STATUS "")
message(STATUS "Setting up the utopia-env ...")

execute_process(
    COMMAND ${Python_EXECUTABLE} -m venv ${UTOPIA_ENV_DIR}
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_VARIABLE PY_OUTPUT
    ERROR_VARIABLE PY_OUTPUT
)
if (NOT RETURN_VALUE EQUAL "0")
    message(FATAL_ERROR "Error setting up venv for utopia-env:  ${PY_OUTPUT}")
endif ()

# create a symlink to the activation script
file(CREATE_LINK ${UTOPIA_ENV_DIR}/bin/activate
                 ${CMAKE_BINARY_DIR}/activate
     SYMBOLIC)

# write the convenience bash script
# configure the script
configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/run-in-utopia-env.in
                ${CMAKE_BINARY_DIR}/cmake/scripts/run-in-utopia-env
                @ONLY)
# copy the script, changing file permissions
file(COPY ${CMAKE_BINARY_DIR}/cmake/scripts/run-in-utopia-env
        DESTINATION ${CMAKE_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)

set(RUN_IN_UTOPIA_ENV ${CMAKE_BINARY_DIR}/run-in-utopia-env)


# -- virtual environment installed now, but remains to be populated -----------

# update pip and installation packages only if they are below a certain version
# number; updating everytime is unnecessary and takes too long... while this
# _should_ be taken care of by pip itself, it is sometimes not done on Ubuntu
# systems, leading to failure to install wheel-requiring downstream packages

# -- pip
python_find_package(PACKAGE pip VERSION 22.0)
if (NOT PYTHON_PACKAGE_pip_FOUND)
    python_pip_install(PACKAGE pip UPGRADE)
endif()

# -- setuptools
python_find_package(PACKAGE setuptools VERSION 51.1)
if (NOT PYTHON_PACKAGE_setuptools_FOUND)
    python_pip_install(PACKAGE setuptools UPGRADE)
endif()

# -- wheel
python_find_package(PACKAGE wheel VERSION 0.35)
if (NOT PYTHON_PACKAGE_wheel_FOUND)
    python_pip_install(PACKAGE wheel UPGRADE)
endif()

# -- utopya

if(NOT UTOPYA_FROM_PYPI)
    set(UTOPYA_FROM_PYPI On)
endif()

if(NOT UTOPYA_URL)
    set(UTOPYA_URL https://gitlab.com/utopia-project/utopya.git)
endif()

if(NOT UTOPYA_BRANCH)
    set(UTOPYA_BRANCH main)
endif()

python_find_package(PACKAGE utopya VERSION 1.0.0)
if(NOT PYTHON_PACKAGE_utopya_FOUND)
    if(UTOPYA_FROM_PYPI)
        message(STATUS "Installing utopya from PyPI ...")
        python_pip_install(PACKAGE utopya INSTALL_OPTIONS "--pre")
    else()
        message(STATUS "Installing utopya from custom URL ...")
        python_install_package_remote(
            URL ${UTOPYA_URL}
            BRANCH ${UTOPYA_BRANCH}
            EGG_NAME utopya
        )
    endif()
endif()
# TODO Once utopya is on PyPI, consider replacing with requirements file?
#      ... but need to avoid repetitive version checks on each cmake call!


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


# -- additional dependencies
message(STATUS "")
message(STATUS "Installing Python packages from requirements file ...")

python_pip_install(
    INSTALL_OPTIONS -r ${CMAKE_SOURCE_DIR}/.utopia-env-requirements.txt
    ERROR_HINT
        "Try installing the packages defined in .utopia-env-requirements.txt manually to address this error."
)


# -- pre-commit hooks
message(STATUS "")
message(STATUS "Setting up pre-commit hooks ...")

execute_process(
    COMMAND ${RUN_IN_UTOPIA_ENV} pre-commit install
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_QUIET
)
if (NOT RETURN_VALUE EQUAL "0")
    python_find_package(PACKAGE pre-commit)  # to provide package information
    message(SEND_ERROR "Failed setting up pre-commit hooks: ${RETURN_VALUE}")
else()
    message(STATUS "Installed pre-commit hooks.")
endif ()



message(STATUS "")
message(STATUS "Successfully set up the utopia-env.\n")
