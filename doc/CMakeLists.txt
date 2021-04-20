# --- Setup Sphinx ---
# Define a function to install the utilities needed for sphinx ...
function(utopia_install_doc_utils)
    # Install requirements into Utopia venv
    set(INSTALL_CMD install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt)
    execute_process(
        COMMAND ${UTOPIA_ENV_PIP} ${INSTALL_CMD}
        RESULT_VARIABLE RETURN_VALUE
        OUTPUT_QUIET)
    if (NOT RETURN_VALUE EQUAL "0")
        message(SEND_ERROR "Error installing doc utilities into virtualenv: "
                           "${RETURN_VALUE}")
    endif ()
endfunction()

# ... and run it
utopia_install_doc_utils()

# Register subdirectories
add_subdirectory("doxygen")

# Copy some files into the doc directory such that they can be included into
# the built documentation
# NOTE These need to be manually added to .gitignore!
file(COPY ${PROJECT_SOURCE_DIR}/README.md
          ${PROJECT_SOURCE_DIR}/COPYING.md
          ${PROJECT_SOURCE_DIR}/CONTRIBUTING.md
          ${PROJECT_SOURCE_DIR}/CODE_OF_CONDUCT.md
     DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})

# Configure sphinx and register associated files
configure_file(conf.py.in conf.py)
add_custom_target(sphinx_html
    COMMAND ${RUN_IN_UTOPIA_ENV} sphinx-build
            -T -b html
            -W --keep-going -w build_errors.log         # warnings as errors
            -c ${CMAKE_CURRENT_BINARY_DIR}              # conf.py dir
            -d ${CMAKE_CURRENT_BINARY_DIR}/_doctrees    # doctree pickles dir
            ${CMAKE_CURRENT_SOURCE_DIR}                 # input dir
            ${CMAKE_CURRENT_BINARY_DIR}/html            # output dir
)

# Add the doxygen-dependent documentation target only when it exists
if (TARGET doxygen_utopia)
    add_dependencies(sphinx_html doxygen_utopia)
endif ()

add_custom_target(doc)
add_dependencies(doc sphinx_html)
