# Handle configuration and installation of python packages and tests here

# Pass info to utopya  ........................................................
message(STATUS "Updating utopya module with information about models...")

generate_model_info(
    ${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/info.py.in
    ${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/info.py
)

# Installations ...............................................................
message(STATUS "")
message(STATUS "Installing required Python packages ...")

# Manually install dependencies for utopya, if they are not already satisfied
# NOTE Take care that the versions used here match those in the README.md
#      and those in the utopya setup.py file.
# -- paramspace --
python_find_package(PACKAGE paramspace VERSION 2.1.1)

if(NOT PYTHON_PACKAGE_paramspace_FOUND)
    # install from current _master_ branch
    python_install_package_remote(URL ssh://git@ts-gitlab.iup.uni-heidelberg.de:10022/yunus/paramspace.git@release/v2.x
        TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)
endif()

# -- dantro --
python_find_package(PACKAGE dantro VERSION 0.7.0)

if(NOT PYTHON_PACKAGE_dantro_FOUND)
    # install from current _master_ branch
    python_install_package_remote(
        URL ssh://git@ts-gitlab.iup.uni-heidelberg.de:10022/utopia/dantro.git@release/v0.7
        TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)
endif()


# Now, ready to install utopya
message(STATUS "Installing utopya ...")

# For easier development: install using symlinks to the local package
set(UTOPIA_PYTHON_INSTALL_EDITABLE True)

# Install utopya and dependencies
python_install_package(PATH utopya)

# Add a custom target to symlink the utopia CLI
# set(CLI_SYMLINK /usr/local/bin/utopia)
# TODO add linker target here

# Tests .......................................................................
message(STATUS "Adding python test targets ...")

# Add test for utopya, accessible via `make test_utopya`
add_custom_target(test_utopya
                  COMMAND ${CMAKE_BINARY_DIR}/run-in-utopia-env
                          python -m pytest
                          ${CMAKE_CURRENT_SOURCE_DIR}/utopya/test/ -v
                          --cov=${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/
                          --cov-report=term-missing
)

# Add tests for models, accessible via `make test_models_python`
add_custom_target(test_models_python
                  COMMAND ${CMAKE_BINARY_DIR}/run-in-utopia-env
                          python -m pytest -v
                          ${CMAKE_CURRENT_SOURCE_DIR}/model_tests/
)

# For running all python model tests, it is required that all models are built
# add_dependencies(test_models_python all)  # FIXME somehow, all is not defined
# NOTE There are no targets for inidivdual model tests; has to be done manually

# Add the python model tests as a dependency for the test_models target
add_dependencies(test_models test_models_python)

message(STATUS "Python setup complete.\n")
