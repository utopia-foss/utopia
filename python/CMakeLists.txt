# -- Handle configuration and installation of python packages and tests here --

# Installations ...............................................................
message(STATUS "")
message(STATUS "Installing required Python packages ...")

# -- utopya
python_find_package(PACKAGE utopya VERSION 1.0.0a3)  # TODO Update version

if(NOT PYTHON_PACKAGE_utopya_FOUND)
    message(STATUS "Installing utopya ...")
    python_install_package_remote(URL https://gitlab.com/utopia-project/utopya)
endif()

# Inform about dantro version (installation happened via utopya dependencies)
python_find_package(PACKAGE dantro)

# Create a (relative) symlink in the utopia-env: utopia -> utopya
file(CREATE_LINK utopya ${UTOPIA_ENV_DIR}/bin/utopia SYMBOLIC)
# TODO Consider installing a custom wrapper script that advertises the CLI not
#      as "utopya" but as Utopia ...

# -- custom python dependencies
message(STATUS "")
message(STATUS "Installing custom python dependencies ...")
execute_process(
    COMMAND
        ${RUN_IN_UTOPIA_ENV}
        pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
                )
if (NOT ${exit_code} STREQUAL "0")
    message(WARNING "Error ${exit_code} in installing additional python "
                    "requirements! Install the packages defined in the "
                    "python/requirements.txt file manually to remove this "
                    "warning.\nOutput of pip install was:\n${output}")
endif()


# Pass info to utopya  ........................................................
message(STATUS "")
message(STATUS "Providing model information to utopya ...")

# Declare the directories containing the models tests and plots
set(UTOPIA_PYTHON_MODEL_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/model_tests
    CACHE STRING "the directory containing all python-based model tests" FORCE)
set(UTOPIA_PYTHON_MODEL_PLOTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/model_plots
    CACHE STRING "the directory containing all python-based model plots" FORCE)

# Register the existing models and associated frontend-related information
register_models_with_frontend()


# Tests .......................................................................
message(STATUS "")
message(STATUS "Adding python test targets ...")

# Add tests for models, accessible via `make test_models_python`
add_custom_target(test_models_python
                  COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} &&
                    ${CMAKE_BINARY_DIR}/run-in-utopia-env
                    python -m pytest -vv model_tests/
)

# For running all python model tests, it is required that all models are built
# add_dependencies(test_models_python all)  # FIXME somehow, all is not defined
# TODO Add targets for inidivdual python model tests (elsewhere)

# Add the python model tests as a dependency for the test_models target
add_dependencies(test_models test_models_python)


message(STATUS "Python setup complete.\n")
