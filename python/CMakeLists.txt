# Handle configuration and installation of python packages and tests here

# Pass info to utopya  ........................................................
message(STATUS "Updating utopya module with information about models...")

generate_model_info(
    ${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/info.py.in
    ${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/info.py
)

# Installations ...............................................................

# Manually install dependencies for utopya
python_install_package_remote(URL https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace.git@releases/v1.0b
    TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)


# For easier development: install using symlinks to the local package
set(DUNE_PYTHON_INSTALL_EDITABLE True)

# Install utopya and the extras (that are needed for testing)
dune_python_install_package(PATH utopya
                            ADDITIONAL_PIP_PARAMS "${CMAKE_CURRENT_SOURCE_DIR}/utopya/[test_deps]")

# Add a custom target to symlink the utopia CLI
# set(CLI_SYMLINK /usr/local/bin/utopia)
# TODO add linker target here

# Tests .......................................................................
# Add pytest commands via dune
dune_python_add_test(NAME FRONTEND_utopya
                     COMMAND ${CMAKE_BINARY_DIR}/run-in-dune-env
                             python -m pytest 
                             ${CMAKE_CURRENT_SOURCE_DIR}/utopya/test/ -v
                             --cov=${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/
                             --cov-report=term-missing
                     )