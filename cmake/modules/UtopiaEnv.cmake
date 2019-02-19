#
# Create the utopia-env, a Python virtual environment for Utopia
#
# This module set the following variables:
#
#   - UTOPIA_ENV_DIR            Path to the venv installation
#   - UTOPIA_ENV_EXECUTABLE     Python interpreter executable inside the venv
#   - UTOPIA_ENV_PIP            Pip module executable inside the venv
#

# search Python (different routines depending on CMake version)
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
    find_package(Python 3.6 REQUIRED)
else ()
    find_package(PythonInterp 3.6 REQUIRED)
    set (Python_EXECUTABLE ${PYTHON_EXECUTABLE})
endif ()

# create the venv
set (UTOPIA_ENV_DIR ${CMAKE_BINARY_DIR}/utopia-env)
message(STATUS "Setting up the Utopia Python virtual envrionment")
execute_process(
    COMMAND ${Python_EXECUTABLE} -m venv ${UTOPIA_ENV_DIR}
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_QUIET
)
if (NOT RETURN_VALUE EQUAL "0")
    message(FATAL_ERROR "Error creating the utopia-env: ${RETURN_VALUE}")
endif ()

# set the path variables
set (UTOPIA_ENV_EXECUTABLE ${UTOPIA_ENV_DIR}/bin/python)
set (UTOPIA_ENV_PIP ${UTOPIA_ENV_DIR}/bin/pip)

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

# update pip and installation packages
execute_process(COMMAND ${UTOPIA_ENV_PIP} install --upgrade
                        pip wheel setuptools
                RESULT_VARIABLE RETURN_VALUE
                OUTPUT_QUIET)
if (NOT RETURN_VALUE EQUAL "0")
    message(FATAL_ERROR "Error updating pip inside utopia-env: ${RETURN_VALUE}")
endif ()

# write the convenience bash script
find_package(UnixCommands)
if (BASH)
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
else ()
    message (WARNING "Bash was not found. Your system likely does not support "
                     "the utopia-env. Skipping creation of run script.")
endif ()
