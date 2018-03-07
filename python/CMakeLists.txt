# Handle configuration and installation of python packages and tests here

# Pass info to utopya  ........................................................
message(STATUS "Updating utopya module with information about models...")

# Set the file paths for both the file to be configured and target module file
set(CFG_UTOPYA_INFO ${CMAKE_CURRENT_SOURCE_DIR}/configure_utopya_info.cmake)
set(UTOPYA_INFO ${CMAKE_CURRENT_SOURCE_DIR}/utopya/utopya/info.py)
string(RANDOM LENGTH 42 RANDOM_STR)

configure_file(${CFG_UTOPYA_INFO}.in ${CFG_UTOPYA_INFO} @ONLY) 

add_custom_command(OUTPUT ${UTOPYA_INFO}
                   COMMAND ${CMAKE_COMMAND} -P ${CFG_UTOPYA_INFO} 
                   DEPENDS
                       ${CFG_UTOPYA_INFO}
                       ${UTOPYA_INFO}.in
                   COMMENT "Configuring input file:  utopya/info.py.in  ..." 
                   VERBATIM) 

add_custom_target(create_utopya_info_module ALL
                  DEPENDS ${CFG_UTOPYA_INFO} ${UTOPYA_INFO})

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