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
message(STATUS "Setting up the Utopia Python virtual environment ...")

execute_process(
    COMMAND ${Python_EXECUTABLE} -m venv ${UTOPIA_ENV_DIR}
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_QUIET
)
if (NOT RETURN_VALUE EQUAL "0")
    message(FATAL_ERROR "Error creating the utopia-env: ${RETURN_VALUE}")
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


# -- virtual environment fully set up now -------------------------------------

# update pip and installation packages only if they are below a certain version
# number; updating everytime is unnecessary and takes too long... while this
# _should_ be taken care of by pip itself, it is sometimes not done on Ubuntu
# systems, leading to failure to install wheel-requiring downstream packages

python_find_package(PACKAGE pip VERSION 22.0)
if (NOT PYTHON_PACKAGE_pip_FOUND)
    python_install_package_pip(PACKAGE pip)
endif()

python_find_package(PACKAGE setuptools VERSION 51.1)
if (NOT PYTHON_PACKAGE_setuptools_FOUND)
    python_install_package_pip(PACKAGE setuptools)
endif()

python_find_package(PACKAGE wheel VERSION 0.35)
if (NOT PYTHON_PACKAGE_wheel_FOUND)
    python_install_package_pip(PACKAGE wheel)
endif()


message(STATUS "")
message(STATUS "Setting up pre-commit hooks ...")

python_find_package(PACKAGE pre-commit VERSION 2.18)
if (NOT PYTHON_PACKAGE_pre-commit_FOUND)
    python_install_package_pip(PACKAGE pre-commit)
endif()

# Let pre-commit install its git hook
execute_process(
    COMMAND ${RUN_IN_UTOPIA_ENV} pre-commit install
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_QUIET
)
if (NOT RETURN_VALUE EQUAL "0")
    message(SEND_ERROR "Failed setting up pre-commit hooks: ${RETURN_VALUE}")
endif ()



message(STATUS "")
message(STATUS "utopia-env fully set up now.")
