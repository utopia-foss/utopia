
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
#       .. cmake_param:: SHOW_IN_SUMMARY
#          :option:
#
#          If given, will add the package name to the feature summary list.
#          If a version is required, will include that one.
#
#       .. cmake_param:: SHOW_AS_REQUIRED
#          :option:
#
#          If given, will show the package as required in the feature summary.
#          Note that this does not change the function behavior and is
#          decoupled from the REQUIRED flag.
#
#   Install a remote python package located in a git repository
#   into the Utopia venv.
#
function(python_find_package)
    # Parse function arguments
    set(OPTION REQUIRED SHOW_IN_SUMMARY SHOW_AS_REQUIRED)
    set(SINGLE PACKAGE VERSION RESULT)
    set(MULTI)
    include(CMakeParseArguments)
    cmake_parse_arguments(ARG "${OPTION}" "${SINGLE}" "${MULTI}" "${ARGN}")

    if(ARG_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed arguments in python_find_package!")
    endif()

    # decide if we let errors pass
    set(ERROR_SWITCH "STATUS")
    if (ARG_REQUIRED)
        set(ERROR_SWITCH "SEND_ERROR")
    endif ()

    # Now, let pip try to find the package
    execute_process(COMMAND ${UTOPIA_ENV_PIP} show ${ARG_PACKAGE}
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_VARIABLE PIP_OUTPUT
        ERROR_VARIABLE PIP_OUTPUT
    )

    # If this failed, a package with that name is not available
    if (NOT RETURN_VALUE EQUAL "0")
        message(
            ${ERROR_SWITCH} "Could not find Python package ${ARG_PACKAGE}."
        )
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

    # Optionally, make it appear in the summary of required packages
    if (ARG_SHOW_IN_SUMMARY)
        set_property(
            GLOBAL APPEND PROPERTY
                PACKAGES_FOUND ${ARG_PACKAGE}
        )
        if (ARG_VERSION)
            set_property(
                GLOBAL APPEND PROPERTY
                    _CMAKE_${ARG_PACKAGE}_REQUIRED_VERSION ">= ${ARG_VERSION}"
            )
        endif()
        if (ARG_SHOW_AS_REQUIRED)
            set_property(
                GLOBAL APPEND PROPERTY
                    _CMAKE_${ARG_PACKAGE}_TYPE REQUIRED
            )
        endif()
    endif()
endfunction()




# Call python `pip install` command inside Utopia's virtual environment
#
# .. cmake_function:: python_pip_install
#
#       .. cmake_param:: PACKAGE
#          :optional:
#          :single:
#
#          The name of the package to install
#
#       .. cmake_param:: UPGRADE
#          :option:
#
#          If given, adds the --upgrade flag to the command
#
#       .. cmake_param:: ALLOW_FAILURE
#          :option:
#
#          If given, will send a warning instead of an error
#
#       .. cmake_param:: QUIET
#          :option:
#
#          If given, does not generate status messages
#
#       .. cmake_param:: INSTALL_OPTIONS
#          :optional:
#          :multi:
#
#          Additional installation parameters
#
#       .. cmake_param:: ERROR_HINT
#          :optional:
#          :single:
#
#          A string that is emitted alongside an error message and hints at a
#          resolution of the error
#
function(python_pip_install)
    set(OPTION UPGRADE ALLOW_FAILURE QUIET)
    set(SINGLE PACKAGE ERROR_HINT)
    set(MULTI INSTALL_OPTIONS)
    include(CMakeParseArguments)
    cmake_parse_arguments(PYINST "${OPTION}" "${SINGLE}" "${MULTI}" "${ARGN}")
    if(PYINST_UNPARSED_ARGUMENTS)
        message(
            WARNING "Encountered unparsed arguments in python_pip_install!"
        )
    endif()

    # decide whether to allow failure
    set(ERROR_SWITCH "SEND_ERROR")
    if (PYINST_ALLOW_FAILURE)
        set(ERROR_SWITCH "WARNING")
    endif ()

    set(INSTALL_ARGS)
    if(PYINST_PACKAGE)
        if(NOT PYINST_QUIET)
            message(STATUS "Installing Python package ${PYINST_PACKAGE} ...")
        endif()
        set(INSTALL_ARGS ${PYINST_PACKAGE})
    endif()

    if(PYINST_UPGRADE)
        set(PYINST_INSTALL_OPTIONS ${PYINST_INSTALL_OPTIONS} --upgrade)
    endif()
    set(INSTALL_ARGS ${PYINST_INSTALL_OPTIONS} ${INSTALL_ARGS})

    execute_process(
        COMMAND ${UTOPIA_ENV_PIP} install ${INSTALL_ARGS}
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_VARIABLE PIP_OUTPUT
        ERROR_VARIABLE PIP_OUTPUT
    )
    if (NOT RETURN_VALUE EQUAL "0")
        string(JOIN " " _INSTALL_ARGS ${INSTALL_ARGS})
        message(
            ${ERROR_SWITCH}
            "Call to `pip install ${_INSTALL_ARGS}` failed:\n"
            "${PIP_OUTPUT}"
            "${PYINST_ERROR_HINT}\n"
        )
        return()
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
#          (HTTPS) URL to the git repository. To select a branch or egg, use
#          the respective other commands
#
#       .. cmake_param:: TRUSTED_HOST
#          :optional:
#          :single:
#
#          URL of the trusted host.
#
#       .. cmake_param:: EGG_NAME
#          :optional:
#          :single:
#
#          Name of the egg (typically: the package name) that is appended to
#          the given URL, separated by a `#`.
#
#       .. cmake_param:: BRANCH
#          :optional:
#          :single:
#
#          Name of the branch from which to install the package; is appended
#          to the URL, separated by a `@`.
#
function(python_install_package_remote)
    # parse function arguments
    set(OPTION QUIET)
    set(SINGLE URL TRUSTED_HOST EGG_NAME BRANCH)
    set(MULTI)
    include(CMakeParseArguments)
    cmake_parse_arguments(RINST "${OPTION}" "${SINGLE}" "${MULTI}" "${ARGN}")
    if(RINST_UNPARSED_ARGUMENTS)
        message(WARNING "Encountered unparsed arguments in
            python_install_package_remote!")
    endif()

    if (NOT RINST_QUIET)
        if (RINST_EGG_NAME)
            message(STATUS "Installing python package ${RINST_EGG_NAME} ...")
            message(STATUS "  URL:     ${RINST_URL}")
        else()
            message(STATUS "Installing package from ${RINST_URL} ...")
        endif()
    endif()

    # Need 'git' in front of URL
    set(RINST_FULL_PATH git+${RINST_URL})

    # if branch and egg name are given, add those
    if (RINST_BRANCH)
        if (NOT RINST_QUIET)
            message(STATUS "  Branch:  ${RINST_BRANCH}")
        endif()
        set(RINST_FULL_PATH ${RINST_FULL_PATH}@${RINST_BRANCH})
    endif()
    if (RINST_EGG_NAME)
        set(RINST_FULL_PATH ${RINST_FULL_PATH}\#${RINST_EGG_NAME})
    endif()

    # add trusted-host command
    set(TRUSTED_HOST_CMD "")
    if(RINST_TRUSTED_HOST)
        set(TRUSTED_HOST_CMD --trusted-host ${RINST_TRUSTED_HOST})
    endif()

    # execute command
    set(INSTALL_CMD install ${TRUSTED_HOST_CMD} --upgrade
        ${RINST_FULL_PATH})
    execute_process(
        COMMAND ${UTOPIA_ENV_PIP} ${INSTALL_CMD}
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_VARIABLE PIP_OUTPUT
    )
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error installing remote package:\n${PIP_OUTPUT}")
    endif ()
endfunction()
