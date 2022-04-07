# Pass model information to utopya  ...........................................

# Declare the directories containing the models tests and plots
set(UTOPIA_PYTHON_MODEL_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/model_tests
    CACHE STRING "the directory containing all python-based model tests" FORCE)
set(UTOPIA_PYTHON_MODEL_PLOTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/model_plots
    CACHE STRING "the directory containing all python-based model plots" FORCE)
# TODO Remove redundant definition; either use these to configure the utopya
#      project file or the other way around. Another option would be to let
#      utopya extract that information from the project ... ?

# Register the existing models and associated frontend-related information
register_models_with_utopya()


# Installations ...............................................................
# TODO Consider modularising into cmake macro
# TODO Consider adding a toggle to activate/deactivate
message(STATUS "Installing further python requirements ...\n")

execute_process(
    COMMAND
        ${RUN_IN_UTOPIA_ENV}
        pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_VARIABLE PIP_OUTPUT
    ERROR_QUIET
)
if (NOT RETURN_VALUE EQUAL "0")
    message(
        SEND_ERROR "Failed installing further python requirements! "
                   "Install the packages defined in the "
                   "python/requirements.txt file manually to remove this "
                   "warning. Output of `pip install` was:\n${PIP_OUTPUT}"
    )
endif()


# Tests .......................................................................

# Add tests for models, accessible via `make test_models_python`
add_custom_target(
    test_models_python
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} &&
        ${RUN_IN_UTOPIA_ENV} python -m pytest -vv model_tests/
)
# TODO Add targets for inidivdual python model tests (elsewhere)

# For running all python model tests, it is required that all models are built
# add_dependencies(test_models_python all)  # FIXME somehow, all is not defined

# Add the python model tests as a dependency for the test_models target
add_dependencies(test_models test_models_python)


message(STATUS "Python setup complete.\n")
