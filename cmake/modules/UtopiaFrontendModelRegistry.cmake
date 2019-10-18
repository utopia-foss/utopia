# Add all Utopia models to the utopya model registry
#
# .. cmake_function:: register_models_with_utopya
#
#   This function calls the utopya CLI to register model information with the
#   frontend.
#   Effectively, it passes the model target names, binary paths and source
#   directory paths to the CLI; all that information is stored in the cache
#   variables which are maintained by the add_model cmake function.
#
function(register_models_with_frontend)
    # Check how many models are available. Only continue if there are models
    # available ...
    list(LENGTH UTOPIA_MODEL_TARGETS num_model_targets)

    if (${num_model_targets} EQUAL 0)
        message(STATUS "No models available to register with frontend.")
        return()
    endif()

    # NOTE Consider searching for utopia cli via find_program. Will perhaps be
    #      necessary once the utopia-env is no longer available ...
    execute_process(COMMAND
                        ${RUN_IN_UTOPIA_ENV}
                        utopia models register
                        "${UTOPIA_MODEL_TARGETS}"
                        --bin-path "${UTOPIA_MODEL_BINPATHS}"
                        --src-dir "${UTOPIA_MODEL_SRC_DIRS}"
                        --base-bin-dir "${CMAKE_BINARY_DIR}"
                        --base-src-dir "${CMAKE_SOURCE_DIR}"
                        --separator ";"
                        --label added_by_cmake
                        --overwrite-label
                        --exists-action validate
                    RESULT_VARIABLE exit_code
                    OUTPUT_VARIABLE output
                    ERROR_VARIABLE output
                    )

    if (NOT ${exit_code} STREQUAL "0")
        message(STATUS "Output of utopya CLI:\n${output}")
        message(SEND_ERROR "Error ${exit_code} in adding models to registry!")
    endif()

    message(STATUS "Added or updated model registry entries for "
                   "${num_model_targets} models.")
endfunction()


# Register an (external) python module with the Utopia Frontend
#
# .. cmake_function:: register_python_module
#
#   This function calls the utopya CLI to register paths to external python
#   modules with the frontend
#
function(register_python_module)
    # Parse function arguments
    set(OPTION)
    set(SINGLE NAME MODULE_PATH)
    set(MULTI)
    include(CMakeParseArguments)
    cmake_parse_arguments(ARG "${OPTION}" "${SINGLE}" "${MULTI}" "${ARGN}")

    if(ARG_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed arguments in register_python_module!")
    endif()

    # Invoke the CLI
    execute_process(COMMAND
                        ${RUN_IN_UTOPIA_ENV}
                        utopia config external_module_paths
                        --get
                        --set "${ARG_NAME}=${ARG_MODULE_PATH}"
                    RESULT_VARIABLE exit_code
                    OUTPUT_VARIABLE output
                    ERROR_VARIABLE output
                    )

    if (NOT ${exit_code} STREQUAL "0")
        message(STATUS "Output of utopya CLI:\n${output}")
        message(SEND_ERROR "Error ${exit_code} in registering external python "
                           "modules with Utopia frontend!")
    endif()

    message(STATUS "Registered external '${ARG_NAME}' module with frontend.")
endfunction()


# Register a custom plotting module with the Utopia frontend
#
# .. cmake_function:: register_plot_module
#
#   This function calls the utopya CLI to pass the information to the frontend.
#
function(register_plot_module)
    # Parse function arguments
    set(OPTION)
    set(SINGLE NAME MODULE_PATH)
    set(MULTI)
    include(CMakeParseArguments)
    cmake_parse_arguments(ARG "${OPTION}" "${SINGLE}" "${MULTI}" "${ARGN}")

    if(ARG_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed arguments in register_plot_module!")
    endif()

    # Invoke the CLI
    execute_process(COMMAND
                        ${RUN_IN_UTOPIA_ENV}
                        utopia config plot_module_paths
                        --get
                        --set "${ARG_NAME}=${ARG_MODULE_PATH}"
                    RESULT_VARIABLE exit_code
                    OUTPUT_VARIABLE output
                    ERROR_VARIABLE output
                    )

    if (NOT ${exit_code} STREQUAL "0")
        message(STATUS "Output of utopya CLI:\n${output}")
        message(SEND_ERROR "Error ${exit_code} in registering plot module "
                           "with Utopia frontend!")
    endif()

    message(STATUS "Registered plot module '${ARG_NAME}' with frontend.")
endfunction()
