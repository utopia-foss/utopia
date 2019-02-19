
# Find a python package using pip freeze
#
# .. cmake_function:: python_find_package
#       
#       .. cmake_param:: REQUIRED
#          :option:
#          :optional:
#
#          Whether this is a required package
#
#       .. cmake_param:: PACKAGE
#          :required:
#          :single:
#
#          The name of the python package
#
#       .. cmake_param:: VERSION
#          :optional:
#          :single:
#
#          The desired version; if a version smaller than this one is found,
#          the package is regarded as not having been found
#
#   Install a remote python package located in a git repository
#   into the DUNE venv.
#
function(python_find_package)
    # Parse function arguments
    set(OPTION REQUIRED)
    set(SINGLE PACKAGE VERSION RESULT)
    set(MULTI)
    include(CMakeParseArguments)
    cmake_parse_arguments(ARG "${OPTION}" "${SINGLE}" "${MULTI}" "${ARGN}")

    if(ARG_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed arguments in python_find_package!")
    endif()

    if(ARG_VERSION)
        message(STATUS "Looking for python package ${ARG_PACKAGE} "
                    ">= ${ARG_VERSION} ...")
    else()
        message(STATUS "Looking for python package ${ARG_PACKAGE} ...")
    endif()

    execute_process(COMMAND ${UTOPIA_ENV_PIP} freeze
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_VARIABLE PIP_OUTPUT
    )

    # Make sure this did not fail
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error running pip: ${RETURN_VALUE}")
        return()
    endif ()

    # Find out the version number using a regex
    if (PIP_OUTPUT MATCHES "${ARG_PACKAGE}==([^\n]+)")
        # Package was found with version CMAKE_MATCH_1
        message(STATUS "Found ${ARG_PACKAGE} ${CMAKE_MATCH_1}")

        if (ARG_VERSION)
            # Check against given version
            if (CMAKE_MATCH_1 VERSION_LESS ARG_VERSION)
                # Version is lower than specified
                if (ARG_REQUIRED)
                    message(SEND_ERROR "Required package ${ARG_PACKAGE} >= "
                                "${ARG_VERSION} not found! Found only "
                                "version ${CMAKE_MATCH_1}")
                    set(PYTHON_PACKAGE_${ARG_PACKAGE}_FOUND FALSE PARENT_SCOPE)
                    return()
                endif()
                # ...but that's no issue because it's optional
            endif()
            # else: sufficient version
        endif()
        # else: version not given, but package was found ... all good.
    else()
        # Could not find the package
        if (ARG_REQUIRED)
            message(SEND_ERROR "Required package ${ARG_PACKAGE} not found!")
            set(PYTHON_PACKAGE_${ARG_PACKAGE}_FOUND FALSE PARENT_SCOPE)
            return()
        endif()
        message(STATUS "Package ${ARG_PACKAGE} not found.")

        # Did not find optional package; do not set variables
        set(PYTHON_PACKAGE_${ARG_PACKAGE}_FOUND FALSE PARENT_SCOPE)
        return()
    endif()

    # If this point is reached, everything is ok and the variables can be set
    set(PYTHON_PACKAGE_${ARG_PACKAGE}_FOUND TRUE PARENT_SCOPE)
endfunction()




# Install a python package located in a remote git repository
#
# .. cmake_function:: python_install_package_remote
#       
#       .. cmake_param:: URL
#          :required:
#          :single:
#
#          (HTTPS) URL to the git repository. May be appended with '@'
#          and a brach or tag identifier.
#
#       .. cmake_param:: TRUSTED_HOST
#          :optional:
#          :single:
#
#          URL of the respective trusted host.
#
#   Install a remote python package located in a git repository
#   into the DUNE venv.
#
function(python_install_package_remote)
    # parse function arguments
    set(OPTION)
    set(SINGLE URL TRUSTED_HOST)
    set(MULTI)
    include(CMakeParseArguments)
    cmake_parse_arguments(RINST "${OPTION}" "${SINGLE}" "${MULTI}" "${ARGN}")
    if(RINST_UNPARSED_ARGUMENTS)
        message(WARNING "Encountered unparsed arguments in
            python_install_package_remote!")
    endif()

    message(STATUS "Installing package at ${RINST_URL} into the virtualenv")

    # set 'git' in front of URL
    set(RINST_FULL_PATH git+${RINST_URL})

    # add trusted-host command
    set(TRUSTED_HOST_CMD "")
    if(RINST_TRUSTED_HOST)
        set(TRUSTED_HOST_CMD --trusted-host ${RINST_TRUSTED_HOST})
    endif()

    # execute command
    set(INSTALL_CMD install ${TRUSTED_HOST_CMD} --upgrade
        ${RINST_FULL_PATH})
    execute_process(COMMAND ${UTOPIA_ENV_PIP} ${INSTALL_CMD}
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_QUIET
    )
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error installing remote package: ${RETURN_VALUE}")
    endif ()
endfunction()

# Install a local Python package into the Utopia virtual environment
#
# This function takes the following arguments:
#
#   - PATH (single)     Path to the Python package. Relative paths will
#                       be interpreted as relative from the call site
#                       of this function.
#
#   - PIP_PARAMS (multi)    Additional parameters passed to pip.
#
# This function uses the following global variables (if defined):
#
#   - UTOPIA_PYTHON_INSTALL_EDITABLE    Install the package in editable mode.
#
function(python_install_package)
    # parse function arguments
    set(OPTION)
    set(SINGLE PATH)
    set(MULTI PIP_PARAMS)
    include(CMakeParseArguments)
    cmake_parse_arguments(PYINST "${OPTION}" "${SINGLE}" "${MULTI}" "${ARGN}")
    if(PYINST_UNPARSED_ARGUMENTS)
        message(WARNING "Encountered unparsed arguments in \
python_install_package!")
    endif()

    # determine the full path to the Python package
    set (PACKAGE_PATH ${PYINST_PATH})
    if (NOT IS_ABSOLUTE ${PYINST_PATH})
        set (PACKAGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${PYINST_PATH})
    endif ()

    # apply the global editable option
    set (EDIT_OPTION "")
    if (UTOPIA_PYTHON_INSTALL_EDITABLE)
        set (EDIT_OPTION "-e")
    endif ()

    # define the installation command
    set(INSTALL_CMD install --upgrade ${EDIT_OPTION} ${PYINST_PIP_PARAMS}
                    ${PACKAGE_PATH})

    # perform the actual installation
    message (STATUS "Installing python package at ${PACKAGE_PATH} \
into the virtual environment")
    execute_process(COMMAND ${UTOPIA_ENV_PIP} ${INSTALL_CMD}
                    RESULT_VARIABLE RETURN_VALUE
                    OUTPUT_QUIET)
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error installing package: ${RETURN_VALUE}")
    endif ()

endfunction()
