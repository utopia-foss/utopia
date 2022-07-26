# Set up documentation building
# -----------------------------

# Need doxygen for the C++ docs
add_subdirectory("doxygen")


# .. Sphinx config ............................................................

# Install the utilities needed for sphinx ...
execute_process(
    COMMAND
        ${UTOPIA_ENV_PIP} install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    RESULT_VARIABLE RETURN_VALUE
    OUTPUT_VARIABLE PIP_OUTPUT
    ERROR_VARIABLE PIP_OUTPUT
)
if (NOT RETURN_VALUE EQUAL "0")
    message(SEND_ERROR "Error installing doc utilities into virtualenv: "
                       "${PIP_OUTPUT}")
endif ()

# Copy some files into the doc directory such that they can be included into
# the built documentation
# NOTE These need to be manually added to .gitignore!
file(COPY ${PROJECT_SOURCE_DIR}/README.md
          ${PROJECT_SOURCE_DIR}/COPYING.md
          ${PROJECT_SOURCE_DIR}/CONTRIBUTING.md
          ${PROJECT_SOURCE_DIR}/CODE_OF_CONDUCT.md
     DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})

# Can now configure Sphinx config file
configure_file(conf.py.in conf.py)

# .. Targets ..................................................................
# Gather shared arguments
set(SHARED_SPHINX_ARGS
        -w build_errors.log                         # output log file
        -c ${CMAKE_CURRENT_BINARY_DIR}              # conf.py dir (in build!)
        -d ${CMAKE_CURRENT_BINARY_DIR}/_doctrees    # doctree pickles dir
        ${CMAKE_CURRENT_SOURCE_DIR}                 # input dir
        ${CMAKE_CURRENT_BINARY_DIR}/html            # output dir
)
set(SPHINX_FAIL_ON_WARN -W --keep-going)

# Register sphinx build targets
add_custom_target(sphinx_html
    COMMAND ${RUN_IN_UTOPIA_ENV}
        sphinx-build -T -b html ${SHARED_SPHINX_ARGS} ${SPHINX_FAIL_ON_WARN}
)

# ... and some targets for checking the resulting build
add_custom_target(sphinx_doctest
    COMMAND ${RUN_IN_UTOPIA_ENV}
        sphinx-build -T -b doctest ${SHARED_SPHINX_ARGS} ${SPHINX_FAIL_ON_WARN}
)
add_custom_target(sphinx_linkcheck
    COMMAND ${RUN_IN_UTOPIA_ENV}
        sphinx-build -T -b linkcheck ${SHARED_SPHINX_ARGS}
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
add_dependencies(check_docs sphinx_doctest sphinx_linkcheck)
