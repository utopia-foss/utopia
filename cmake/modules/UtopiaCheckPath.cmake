#
# Checks if a path is valid by checking if a referenced directory or file
# exists. Prints messages of a custom message type only if not. May return the
# result of the check as a variable.
#
# This function takes the following arguments:
#
#   - MESSAGE (single): Type of message to report if a path does not exist.
#                       Use message types as for the 'message()' function.
#                       Omit this argument for no messages.
#   - EXIST_VAR (single): Return variable for the check. May be omitted.
#   - DIRECTORIES (multi): Directories to check. Arguments may also be lists.
#
function(check_path)
    # Parse arguments
    set(SINGLE MESSAGE EXIST_VAR)
    set(MULTI DIRECTORIES)
    cmake_parse_arguments(ARG "" "${SINGLE}" "${MULTI}" ${ARGN})

    # Check arguments
    if (NOT ARG_MESSAGE AND NOT ARG_EXIST_VAR)
        message(WARNING "Setting neither MESSAGE nor EXIST_VAR means this "
                        "function has no effect at all!")
    elseif(ARG_EXIST_VAR)
        set(${ARG_EXIST_VAR} TRUE PARENT_SCOPE)
    endif()

    # Interpret every DIRECTORIES argument as list
    # NOTE: IN LISTS loops over every item in every list
    foreach(dir IN LISTS ARG_DIRECTORIES)
        if (NOT EXISTS ${dir})
            if (ARG_EXIST_VAR)
                set(${ARG_EXIST_VAR} FALSE PARENT_SCOPE)
            endif()
            if (ARG_MESSAGE)
                message(${ARG_MESSAGE} "Path does not exist: ${dir}")
            endif()
        endif()
    endforeach()
endfunction()
