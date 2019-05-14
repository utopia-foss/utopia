# place auxilary files in build directory
file(COPY
        "cfg_utils_test.yml"
        "datamanager_test.yml"
        "datamanager_test_anchors.yml"
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
    writetask_test
    datamanager_test
    )

# Add DATAIO_-prefixed test targets
foreach(test ${TESTS_DATAIO})
    add_unit_test(NAME ${test}
                  GROUP dataio
                  SOURCES ${test}.cc)
endforeach(test)

# no matter how ctest is called, make sure of dependencies between these tests
set_tests_properties(DATAIO_hdfattribute_test_write PROPERTIES FIXTURES_SETUP H5IO)
set_tests_properties(DATAIO_hdfattribute_test_read PROPERTIES FIXTURES_REQUIRED H5IO)
set_tests_properties(DATAIO_hdfdataset_test_write PROPERTIES FIXTURES_SETUP H5IO)
set_tests_properties(DATAIO_hdfdataset_test_read PROPERTIES FIXTURES_REQUIRED H5IO)
