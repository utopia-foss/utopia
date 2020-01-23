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

# search Python (different routines depending on CMake version)
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
    find_package(Python 3.7 REQUIRED)
else ()
    find_package(PythonInterp 3.7 REQUIRED)
    set (Python_EXECUTABLE ${PYTHON_EXECUTABLE})
endif ()

# search for a Bash environment
find_package(UnixCommands)
if (NOT DEFINED BASH)
    message(SEND_ERROR "Bash is required to execute commands in the virtual "
                       "environment")
endif ()

# the designated paths for the virtual env and some environments.
# some might not exist yet at this point, are created below
set (UTOPIA_ENV_DIR ${CMAKE_BINARY_DIR}/utopia-env)
set (UTOPIA_ENV_EXECUTABLE ${UTOPIA_ENV_DIR}/bin/python)
set (UTOPIA_ENV_PIP ${UTOPIA_ENV_DIR}/bin/pip)

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
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
    file (CREATE_LINK ${UTOPIA_ENV_DIR}/bin/activate
                      ${CMAKE_BINARY_DIR}/activate
          SYMBOLIC)
else ()
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${UTOPIA_ENV_DIR}/bin/activate
            ${CMAKE_BINARY_DIR}/activate
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_QUIET
    )
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error creating a symlink to activate: "
                           "${RETURN_VALUE}")
    endif ()
endif ()

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

# pip
python_find_package(PACKAGE pip VERSION 18.0)
if (NOT PYTHON_PACKAGE_pip_FOUND)
    message(STATUS "Installing or upgrading pip ...")

    execute_process(COMMAND ${UTOPIA_ENV_PIP} install --upgrade pip
                    RESULT_VARIABLE RETURN_VALUE
                    OUTPUT_QUIET)
    
    # FIXME does not fail, e.g. without internet connection
    if (NOT RETURN_VALUE EQUAL "0")
        message(WARNING
                    "Error upgrading pip inside utopia-env: ${RETURN_VALUE}")
    endif ()
endif()

# setuptools
python_find_package(PACKAGE setuptools VERSION 39.0)
if (NOT PYTHON_PACKAGE_setuptools_FOUND)
    message(STATUS "Installing or upgrading setuptools ...")

    execute_process(COMMAND ${UTOPIA_ENV_PIP} install --upgrade setuptools
                    RESULT_VARIABLE RETURN_VALUE
                    OUTPUT_QUIET)
    
    if (NOT RETURN_VALUE EQUAL "0")
        message(WARNING
                    "Error upgrading setuptools inside utopia-env: ${RETURN_VALUE}")
    endif ()
endif()

# wheel
python_find_package(PACKAGE wheel VERSION 0.30)
if (NOT PYTHON_PACKAGE_wheel_FOUND)
    message(STATUS "Installing or upgrading wheel ...")

    execute_process(COMMAND ${UTOPIA_ENV_PIP} install --upgrade wheel
                    RESULT_VARIABLE RETURN_VALUE
                    OUTPUT_QUIET)
    
    if (NOT RETURN_VALUE EQUAL "0")
        message(WARNING
                    "Error upgrading setuptools inside utopia-env: ${RETURN_VALUE}")
    endif ()
endif()
