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
    set(RINST_FULL_PATH "git+${RINST_URL}")

    # add trusted-host command
    set(TRUSTED_HOST_CMD "")
    if(RINST_TRUSTED_HOST)
        message(WARNING "Trusting hostname ${RINST_TRUSTED_HOST}!")
        set(TRUSTED_HOST_CMD "--trusted-host ${RINST_TRUSTED_HOST}")
    endif()

    # execute command
    set(INSTALL_CMD -m pip install "${TRUSTED_HOST_COMMAND}" --upgrade
        "${RINST_FULL_PATH}")
    dune_execute_process(
        COMMAND "${DUNE_PYTHON_VIRTUALENV_EXECUTABLE}" "${INSTALL_CMD}"
        ERROR_MESSAGE "Error installing remote package into the venv!"
    )
endfunction()