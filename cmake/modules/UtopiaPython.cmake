
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

    # Run pip to find out the versions of all packages
    # TODO use correct interpreter here
    execute_process(
        COMMAND bash "-c"
            "${DUNE_PYTHON_VIRTUALENV_EXECUTABLE} -m pip freeze | grep ${ARG_PACKAGE}=="
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_VARIABLE PIP_OUTPUT
        ERROR_VARIABLE PIP_ERROR
        )
    # NOTE Can NOT use QUIET_OUTPUT here, because then PIP_OUTPUT will also be
    #      empty! Thus using grep above to reduce terminal output

    # Make sure this did not fail
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error running pip: ${PIP_ERROR}")
        return()
    endif ()

    # Find out the version number using a regex
    if (PIP_OUTPUT MATCHES "^${ARG_PACKAGE}==(.+)$")
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
    set(INSTALL_CMD -m pip install ${TRUSTED_HOST_CMD} --upgrade
        ${RINST_FULL_PATH})
    dune_execute_process(
        COMMAND "${DUNE_PYTHON_VIRTUALENV_EXECUTABLE}" "${INSTALL_CMD}"
        ERROR_MESSAGE "Error installing remote package into the venv!"
    )
endfunction()
