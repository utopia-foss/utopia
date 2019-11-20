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

    # For persistent HTML page names, explicitly set file names to be generated
    # from the C++ definition signature and disable mixed-case file names
    set(DOXYGEN_SHORT_NAMES NO)
    set(DOXYGEN_CASE_SENSE_NAMES NO)

    # register the target
    doxygen_add_docs(doxygen_utopia
        ${doxygen_dirs}
        # use top-level dir as base directory
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Creating doxygen documentation. Check the logfile for emitted warnings."
    )
endif ()
