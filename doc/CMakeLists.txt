# --- Setup Sphinx ---
# Define a function to install the utilities needed for sphinx ...
function(utopia_install_doc_utils)
    # Install requirements into dune virtualenv
    set(INSTALL_CMD -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt)
    dune_execute_process(
        COMMAND "${DUNE_PYTHON_VIRTUALENV_EXECUTABLE}" "${INSTALL_CMD}"
        ERROR_MESSAGE "dune_python_install_package: error installing doc utilities into virtualenv!")

    # Find Sphinx in dune virtualenv directory
    unset(SPHINX_EXECUTABLE CACHE)

    find_program(SPHINX_EXECUTABLE
        NAMES sphinx-build
        PATHS ${DUNE_PYTHON_VIRTUALENV_PATH}/bin
        NO_DEFAULT_PATH)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args("Sphinx" DEFAULT_MSG SPHINX_EXECUTABLE)
endfunction()

# ... and run it
utopia_install_doc_utils()


# Register subdirectories
add_subdirectory("doxygen")


# Copy static files to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/examples
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/_static
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/_templates
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/figures
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/README.md
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


# Configure sphinx and register associated files
include(DuneSphinxCMakeDoc)
dune_cmake_sphinx_doc(
    SPHINX_CONF ${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in
    RST_SOURCES
        index.rst
    MODULE_ONLY)

if(TARGET sphinx_html)
  add_dependencies(sphinx_html doxygen_utopia)
endif()
