# Handle installation of python packages and tests here

# Installations ...............................................................

# Manually install dependencies for utopya
python_install_package_remote(URL https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace.git@releases/v1.0b
    TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)

# Install utopya and the extras (that are needed for testing)
dune_python_install_package(PATH utopya/
                            ADDITIONAL_PIP_PARAMS "${CMAKE_CURRENT_SOURCE_DIR}/utopya/[test_deps]")

# NOTE Other Utopia-packages should be installed here

# Tests .......................................................................

# Add tests for utopya package
add_custom_target(pytest_utopya
                  COMMAND ${CMAKE_BINARY_DIR}/run-in-dune-env
                          python -m pytest 
                          ${CMAKE_CURRENT_SOURCE_DIR}/utopya/test/
                          --cov=utopya
                          --cov-report=term-missing)

add_test(NAME FRONTEND_utopya COMMAND make pytest_utopya)

# add custom target for 'FRONTEND' prefix
add_custom_target(test_frontend
                  COMMAND ctest -V --output-on-failure
                          --tests-regex ^FRONTEND_.+$)

# want to have packages installed in editable mode? call
# set(DUNE_PYTHON_INSTALL_EDITABLE True)