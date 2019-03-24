# place auxilary files in build directory
file(COPY
        "cfg_utils_test.yml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# register targets
set(TESTS_DATAIO
    cfg_utils_test
    graph_utils_test
    hdfbufferfactory_test
    hdfchunking_test
    hdftypefactory_test
    hdffile_test
    hdfattribute_test_read
    hdfattribute_test_write
    hdfattribute_test_lifecycle
    hdfdataset_test_lifecycle
    hdfgroup_test_lifecycle
    hdfdataset_test_write
    hdfdataset_test_read
    hdfintegration_test
    hdfgroup_test
    monitor_test
    )

# Add a target to build all these tests
add_custom_target(build_tests_dataio)
add_dependencies(build_tests_backend build_tests_dataio)

# Add DATAIO_-prefixed test targets
foreach(test ${TESTS_DATAIO})
    add_executable(DATAIO_${test} ${test}.cc)
    target_link_libraries(DATAIO_${test} PUBLIC utopia)
    add_test(NAME DATAIO_${test} COMMAND DATAIO_${test})

    # Add this target to the build target as a dependency
    add_dependencies(build_tests_dataio DATAIO_${test})
endforeach(test)

# no matter how ctest is called, make sure of dependencies between these tests
set_tests_properties(DATAIO_hdfattribute_test_write
                     PROPERTIES FIXTURES_SETUP H5IO)
set_tests_properties(DATAIO_hdfattribute_test_read
                     PROPERTIES FIXTURES_REQUIRED H5IO)
set_tests_properties(DATAIO_hdfdataset_test_write
                     PROPERTIES FIXTURES_SETUP H5IO)
set_tests_properties(DATAIO_hdfdataset_test_read
                     PROPERTIES FIXTURES_REQUIRED H5IO)

# Add custom target for 'DATAIO' prefix
add_custom_target(test_dataio
    COMMAND ctest --output-on-failure --tests-regex ^DATAIO_.+$)

# Make the build target a dependency
add_dependencies(test_dataio build_tests_dataio)

# And make the bundled backend tests have it as a dependency
add_dependencies(test_backend test_dataio)
