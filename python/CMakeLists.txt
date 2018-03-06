# Handle configuration and installation of python packages and tests here

# Pass info to utopya  ........................................................
# Create variables to be passed to the info module


# Now configure the file inside the utopya module that will hold that info
configure_file(utopya/utopya/info.py.in
               ${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/info.py)

# Installations ...............................................................

# Manually install dependencies for utopya
python_install_package_remote(URL https://ts-gitlab.iup.uni-heidelberg.de/yunus/paramspace.git@releases/v1.0b
    TRUSTED_HOST ts-gitlab.iup.uni-heidelberg.de)


# For easier development: install using symlinks to the local package
set(DUNE_PYTHON_INSTALL_EDITABLE True)

# Install utopya and the extras (that are needed for testing)
dune_python_install_package(PATH utopya
                            ADDITIONAL_PIP_PARAMS "${CMAKE_CURRENT_SOURCE_DIR}/utopya/[test_deps]")

# NOTE Other Utopia-packages should be installed here

# Tests .......................................................................

# Add pytest commands via dune
dune_python_add_test(NAME FRONTEND_utopya
                     COMMAND ${CMAKE_BINARY_DIR}/run-in-dune-env
                             python -m pytest 
                             ${CMAKE_CURRENT_SOURCE_DIR}/utopya/test/
                             --cov=utopya --cov-report=term-missing
                     )