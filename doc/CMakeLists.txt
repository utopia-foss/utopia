# --- Setup Sphinx ---
# Define a function to install the utilities needed for sphinx ...
function(utopia_install_doc_utils)
    # Install requirements into dune virtualenv
    set(INSTALL_CMD -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt)
    execute_process(
        COMMAND ${UTOPIA_ENV_EXECUTABLE} ${INSTALL_CMD}
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_QUIET)
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error installing doc utilities into virtualenv: \
${RETURN_VALUE}")
    endif ()

    # Find Sphinx in dune virtualenv directory
    unset(SPHINX_EXECUTABLE CACHE)

    find_program(SPHINX_EXECUTABLE
        NAMES sphinx-build
        PATHS ${UTOPIA_ENV_EXECUTABLE}/bin
        NO_DEFAULT_PATH)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args("Sphinx" DEFAULT_MSG SPHINX_EXECUTABLE)
endfunction()

# ... and run it
utopia_install_doc_utils()

# Register subdirectories
add_subdirectory("doxygen")

# Copy README to this directory
file(COPY "${CMAKE_SOURCE_DIR}/README.md"
     DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})

# Configure sphinx and register associated files
configure_file(conf.py.in conf.py)
add_custom_target(sphinx_html
    COMMAND ${SPHINX_EXECUTABLE}
            -T -b html
            -c ${CMAKE_CURRENT_BINARY_DIR} # conf.py dir
            -d ${CMAKE_CURRENT_BINARY_DIR}/_doctrees # doctree pickles dir
            ${CMAKE_CURRENT_SOURCE_DIR} # input dir
            ${CMAKE_CURRENT_BINARY_DIR}/html # output dir
)

add_dependencies(sphinx_html doxygen_utopia)
add_dependencies(doc sphinx_html)
