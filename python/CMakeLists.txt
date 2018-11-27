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
dune_python_find_package(PACKAGE paramspace
                         VERSION 2.1.0
                         INTERPRETER ${DUNE_PYTHON_VIRTUALENV_EXECUTABLE})

if(NOT DUNE_PYTHON_paramspace_FOUND)
    # install from current _master_ branch
    python_install_package_remote(URL ssh://git@ts-gitlab.iup.uni-heidelberg.de:10022/yunus/paramspace.git@release/v2.x
        TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)
endif()

# -- dantro --
dune_python_find_package(PACKAGE dantro
                         VERSION 0.5.2
                         INTERPRETER ${DUNE_PYTHON_VIRTUALENV_EXECUTABLE})

if(NOT DUNE_PYTHON_dantro_FOUND)
    # install from current _master_ branch
    python_install_package_remote(
        URL ssh://git@ts-gitlab.iup.uni-heidelberg.de:10022/utopia/dantro.git@release/v0.5
        TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)
endif()


# Now, ready to install utopya
message(STATUS "Installing utopya ...")

# For easier development: install using symlinks to the local package
set(DUNE_PYTHON_INSTALL_EDITABLE True)

# Install utopya and dependencies
dune_python_install_package(PATH utopya)

# Add a custom target to symlink the utopia CLI
# set(CLI_SYMLINK /usr/local/bin/utopia)
# TODO add linker target here

# Tests .......................................................................
message(STATUS "Adding test_python targets ...")

# Add test for utopya, accessible via `make target_utopya_tests`
dune_python_add_test(NAME utopya_tests
                     COMMAND ${CMAKE_BINARY_DIR}/run-in-dune-env
                             python -m pytest 
                             ${CMAKE_CURRENT_SOURCE_DIR}/utopya/test/ -v
                             --cov=${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/
                             --cov-report=term-missing
                     )

# Add tests for models, accessible via `make target_model_tests`
dune_python_add_test(NAME model_tests
                     COMMAND ${CMAKE_BINARY_DIR}/run-in-dune-env
                             python -m pytest -v
                             ${CMAKE_CURRENT_SOURCE_DIR}/model_tests/
                     )

# NOTE _both_ these tests are carried out via `make test_python`

# Add custom targets with more convenient names
add_custom_target(test_utopya COMMAND make target_utopya_tests)
add_custom_target(test_models COMMAND make target_model_tests)

message(STATUS "Python setup complete.\n")
