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
    list(LENGTH UTOPIA_MODEL_TARGETS num_models)

    if (${num_models} EQUAL 0)
        message(STATUS "No models available to register with frontend.")
        return()
    endif()

    find_program(UTOPYA_CLI utopya)
    execute_process(
        COMMAND
            ${UTOPYA_CLI} models register from-list
            "${UTOPIA_MODEL_TARGETS}"
            --executables "${UTOPIA_MODEL_BINPATHS}"
            --source-dirs "${UTOPIA_MODEL_SRC_DIRS}"
            --base-executable-dir "${CMAKE_BINARY_DIR}"
            --base-source-dir "${CMAKE_SOURCE_DIR}"
            --separator ";"
            --label added_by_cmake
            --exists-action validate
            --project-name "${CMAKE_PROJECT_NAME}"
            --py-tests-dir-fstr "${UTOPIA_PYTHON_MODEL_TESTS_DIR}/{model_name}"
            --py-plots-dir-fstr "${UTOPIA_PYTHON_MODEL_PLOTS_DIR}/{model_name}"
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
    )

    if (NOT ${exit_code} STREQUAL "0")
        message(STATUS "Output of utopya CLI call:\n\n${output}")
        message(SEND_ERROR "Error ${exit_code} in adding models to registry!")
    endif()

    message(
        STATUS "Added or updated model registry entries for ${num_models} "
               "models in project '${CMAKE_PROJECT_NAME}'."
    )
endfunction()
