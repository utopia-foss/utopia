# Handle configuration and installation of python packages and tests here

# Installations ...............................................................
message(STATUS "")
message(STATUS "Installing required Python packages ...")

# Manually install dependencies for utopya, if they are not already satisfied
# NOTE Take care that the versions used here match those in the README.md
#      and those in the utopya setup.py file.
# -- dantro --
python_find_package(PACKAGE dantro VERSION 0.16.0a1)

if(NOT PYTHON_PACKAGE_dantro_FOUND)
    # install from current _master_ branch
    python_install_package_remote(
        URL https://ts-gitlab.iup.uni-heidelberg.de/utopia/dantro.git@release/v0.16
        TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)
endif()


# Now, ready to install utopya

# -- utopya --
python_find_package(PACKAGE utopya VERSION 0.8.4)

if(NOT PYTHON_PACKAGE_utopya_FOUND)
    message(STATUS "Installing utopya ...")

    # For easier development: install using symlinks to the local package
    set(UTOPIA_PYTHON_INSTALL_EDITABLE True)

    # Install utopya and dependencies
    python_install_package(PATH utopya)
endif()


# Pass info to utopya  ........................................................
message(STATUS "")
message(STATUS "Providing model information to Utopia frontend ...")

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

# Add test for utopya, accessible via `make test_utopya`
add_custom_target(test_utopya
                  COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/utopya/test &&
                    ${CMAKE_BINARY_DIR}/run-in-utopia-env
                    python ../run_test.py ./ -vv
                    --cov=utopya --cov-config=.coveragerc
                    --cov-report=term-missing --cov-report=xml
)

# Add tests for models, accessible via `make test_models_python`
add_custom_target(test_models_python
                  COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} &&
                    ${CMAKE_BINARY_DIR}/run-in-utopia-env
                    python -m pytest -vv model_tests/
)

# For running all python model tests, it is required that all models are built
# add_dependencies(test_models_python all)  # FIXME somehow, all is not defined
# NOTE There are no targets for inidivdual model tests; has to be done manually

# Add the python model tests as a dependency for the test_models target
add_dependencies(test_models test_models_python)


message(STATUS "Python setup complete.\n")
