# The function that was *used* to register Utopia models with the frontend.
#
# The new purpose of this function is to inform a user of an update being
# required for using the latest Utopia.
#
function(register_models_with_frontend)
    #
    # NOTE If changing something here, also check the doc/dev/versioning.rst
    #      file, making sure that the instructions match each other.
    #
    message(FATAL_ERROR
        "It looks like you have updated Utopia.\n"
        "This error is to inform you about the changes needed to get your "
        "model repository to run with the latest version of Utopia:\n"
        "1) Add a project information file named `.utopya-project.yml` to the "
        "root of your model repository and add the following content, "
        "adjusting the marked fields:\n"
        "    project_name: ${CMAKE_PROJECT_NAME}\n"
        "    framework_name: Utopia\n"
        "    paths:\n"
        "      models_dir: rel/path/to/models/src/dir   # TODO adjust this\n"
        "      py_tests_dir: python/model_tests         # if they exist\n"
        "      py_plots_dir: python/model_plots         # if they exist\n"
        "    metadata:                                  # optional metadata\n"
        "      authors:\n"
        "        - your name                            # TODO adjust this\n"
        "      description: This project ...            # TODO adjust this\n"
        "\n"
        "You can later on change the content of the project information file; "
        "it will be read into utopya's project registry upon invoking CMake. "
        "Refer to the Utopia documentation for more information.\n"
        "2) Rename the CMake call to `register_models_with_frontend` (from "
        "where this error message comes) to the updated function name "
        "`register_models_with_utopya`. After having done that, this error "
        "message will no longer appear.\n"
        "3) Remove the CMake cache file (`build/CMakeCache.txt`) in your "
        "models repository or remove your whole `build` directory to avoid "
        "carrying over outdated variables.\n"
        "4) Run `cmake ..` in your models repository.\n"
        "This should get your project running again with the latest version "
        "of Utopia. However, there may be further steps necessary to update "
        "your model plots and tests. "
        "Also note that there are slight changes to the syntax of the CLI.\n"
        "For more information, refer to the Utopia Documentation:\n"
        "  https://docs.utopia-project.org/html/dev/versioning.html\n"
    )
endfunction()


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
function(register_models_with_utopya)
    # Check how many models are available. Only continue if there are models
    # available ...
    list(LENGTH UTOPIA_MODEL_TARGETS num_models)

    if (${num_models} EQUAL 0)
        message(STATUS "No models available to register with frontend.")
        return()
    endif()

    register_project_with_utopya()

    message(STATUS "Providing model information to utopya ...")
    execute_process(
        COMMAND
            ${RUN_IN_UTOPIA_ENV}
            utopya models register from-list
            "${UTOPIA_MODEL_TARGETS}"
            --executables "${UTOPIA_MODEL_BINPATHS}"
            --source-dirs "${UTOPIA_MODEL_SRC_DIRS}"
            --base-executable-dir "${CMAKE_BINARY_DIR}"
            --base-source-dir "${CMAKE_SOURCE_DIR}"
            --separator ";"
            --label added_by_cmake
            --set-default
            --project-name "${CMAKE_PROJECT_NAME}"
            --exists-action overwrite  # TODO Make configurable?
            --py-tests-dir-fstr "${UTOPIA_PYTHON_MODEL_TESTS_DIR}/{model_name}"
            --py-plots-dir-fstr "${UTOPIA_PYTHON_MODEL_PLOTS_DIR}/{model_name}"
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_VARIABLE CLI_OUTPUT
        ERROR_VARIABLE CLI_OUTPUT
)
    if (NOT RETURN_VALUE EQUAL "0")
        message(
            SEND_ERROR "Error ${RETURN_VALUE} in adding or updating models in "
                       "registry:\n${CLI_OUTPUT}\n"
                       "You can use `utopya models` and `utopya projects` "
                       "to manually fix the registry entries."
        )
        return()
    endif()

    message(
        STATUS "Added or updated registry entries for ${num_models} models.\n"
    )
endfunction()


# Add the current project to the utopya project registry
#
# .. cmake_function:: register_project_with_utopya
#
#   This function calls the utopya CLI to register this project.
#
#   TODO Make it possible to optionally pass a path to an info file
#
function(register_project_with_utopya)
    message(STATUS "Providing project information to utopya ...")
    execute_process(
        COMMAND
            ${RUN_IN_UTOPIA_ENV}
            utopya projects register "${PROJECT_SOURCE_DIR}"
            --custom-name "${CMAKE_PROJECT_NAME}"
            --require-matching-names
            --exists-action update
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_VARIABLE CLI_OUTPUT
        ERROR_VARIABLE CLI_OUTPUT
)
if (NOT RETURN_VALUE EQUAL "0")
        message(
            SEND_ERROR "Error ${RETURN_VALUE} registering project "
                       "'${CMAKE_PROJECT_NAME}':\n${CLI_OUTPUT}\n"
                       "You can use `utopya projects` to manually fix the "
                       "registry entry."
        )
        return()
    endif()

    message(
        STATUS "Updated project registry entry for '${CMAKE_PROJECT_NAME}'.\n"
    )
endfunction()
