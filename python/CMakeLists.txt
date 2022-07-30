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


# Tests .......................................................................

# Add target for all python model tests
add_custom_target(
    test_models_python
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} &&
        ${RUN_IN_UTOPIA_ENV} python -m pytest -vv model_tests/
)

# Add targets for inidivdual model tests
# TODO

# For running all python model tests, it is required that all models are built
# add_dependencies(test_models_python all)  # FIXME somehow, all is not defined

# Add the python model tests as a dependency for the test_models target
add_dependencies(test_models test_models_python)
