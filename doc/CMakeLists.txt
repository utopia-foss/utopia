#
# --- Setup the Sphinx-based documentation build ---
#

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

# Configure Sphinx config file
configure_file(conf.py.in conf.py)

# -- Target definitions --
# Gather shared arguments
set(SHARED_ARGS
        -W --keep-going -w build_errors.log         # warnings as errors
        -c ${CMAKE_CURRENT_BINARY_DIR}              # conf.py dir (in build!)
        -d ${CMAKE_CURRENT_BINARY_DIR}/_doctrees    # doctree pickles dir
        ${CMAKE_CURRENT_SOURCE_DIR}                 # input dir
        ${CMAKE_CURRENT_BINARY_DIR}/html            # output dir
)

# Register sphinx build targets
add_custom_target(sphinx_html
    COMMAND ${RUN_IN_UTOPIA_ENV} sphinx-build -T -b html ${SHARED_ARGS}
)

# ... and some targets for checking the resulting build
add_custom_target(sphinx_linkcheck
    COMMAND ${RUN_IN_UTOPIA_ENV} sphinx-build -T -b linkcheck ${SHARED_ARGS}
)
add_custom_target(sphinx_doctest
    COMMAND ${RUN_IN_UTOPIA_ENV} sphinx-build -T -b doctest ${SHARED_ARGS}
)

# Add the doxygen-dependent documentation target only when it exists
if (TARGET doxygen_utopia)
    add_dependencies(sphinx_html doxygen_utopia)
else()
    message(WARNING "No doxygen docs built; is doxygen installed?")
endif ()

add_custom_target(doc)
add_dependencies(doc sphinx_html)

add_custom_target(check_docs)
add_dependencies(check_docs sphinx_linkcheck sphinx_doctest)
