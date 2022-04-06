
# Find a python package using pip freeze
#
# .. cmake_function:: python_find_package
#       
#       .. cmake_param:: REQUIRED
#          :option:
#          :optional:
#
#          Whether this is a required package. Errors are only thrown if this
#          flag is set.
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
#   into the Utopia venv.
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

    # decide if we let errors pass
    set (ERROR_SWITCH "STATUS")
    if (ARG_REQUIRED)
        set (ERROR_SWITCH "SEND_ERROR")
    endif ()

    # Now, let pip try to find the package
    execute_process(COMMAND ${UTOPIA_ENV_PIP} show ${ARG_PACKAGE}
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_VARIABLE PIP_OUTPUT
    )

    # If this failed, a package with that name is not available
    if (NOT RETURN_VALUE EQUAL "0")
        message(${ERROR_SWITCH} "Could not find Python package ${ARG_PACKAGE}")
        return()
    endif ()
    
    # else: Package is installed.
    # Find out the version number using a regex
    if (PIP_OUTPUT MATCHES "Version: ([^\n]+)")
        # Found a version. Check it.
        if (ARG_VERSION)
            # Check against given version
            if (CMAKE_MATCH_1 VERSION_LESS ARG_VERSION)
                message(${ERROR_SWITCH} "Could not find Python package "
                                        "${ARG_PACKAGE} (Found version "
                                        "${CMAKE_MATCH_1} does not satisfy "
                                        "required version ${ARG_VERSION})")
                # Regard as not found
                return ()
            else ()
                message(STATUS "Found Python package ${ARG_PACKAGE} "
                               "${CMAKE_MATCH_1} (Required version is "
                               "${ARG_VERSION})")
            endif ()
        # any version is fine
        else ()
            message(STATUS "Found Python package ${ARG_PACKAGE} "
                           "${CMAKE_MATCH_1}")
        endif()
    else ()
        if (ARG_VERSION)
            message(${ERROR_SWITCH} "Found Python package ${ARG_PACKAGE}, "
                                    "but it does not specify a version. "
                                    "(Required version is ${ARG_VERSION})")
            # Regard as not found
            return()
        else ()
            message(STATUS "Found Python package ${ARG_PACKAGE} "
                           "(version unknown)")
        endif()
    endif ()

    # If this point is reached, we may set the success variable
    set(PYTHON_PACKAGE_${ARG_PACKAGE}_FOUND TRUE PARENT_SCOPE)
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
        message(WARNING "Encountered unparsed arguments in "
                        "python_install_package!")
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
    message (STATUS "Installing python package at ${PACKAGE_PATH} "
                    "into the virtual environment ...")
    execute_process(COMMAND ${UTOPIA_ENV_PIP} ${INSTALL_CMD}
                    RESULT_VARIABLE RETURN_VALUE
                    OUTPUT_QUIET)
    
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error installing package: ${RETURN_VALUE}")
    endif ()

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
#   into the Utopia venv.
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
