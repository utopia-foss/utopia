# look for Doxygen
find_package(Doxygen
             OPTIONAL_COMPONENTS dot)

if (DOXYGEN_FOUND)
    # set the input directories (relative to the top-level source dir)
    set(doxygen_dirs include src)

    # configure doxygen
    set(DOXYGEN_QUIET YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_GENERATE_XML YES)
    set(DOXYGEN_WARN_LOGFILE ${CMAKE_CURRENT_BINARY_DIR}/doxygen_warnings.log)

    # register the target
    doxygen_add_docs(doxygen_utopia
        ${doxygen_dirs}
        # use top-level dir as base directory
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Creating doxygen documentation. Check the logfile for emitted warnings."
    )
endif ()
