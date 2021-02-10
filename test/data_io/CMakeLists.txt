# place auxilary files in build directory
file(COPY
        "cfg_utils_test.yml"
        "filesystem_test.yml"
        "datamanager_test.yml"
        "datamanager_test_anchors.yml"
        "datamanager_test_factory.yml"
        "graph_load_test.yml"
        "graph_load_test.gv"
        "graph_load_test.graphml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


# register targets
set(TESTS_DATAIO
    cfg_utils_test
    filesystem_test
    graph_utils_test
    graph_load_test
    hdfbufferfactory_test
    hdfchunking_test
    hdftype_test
    hdffile_test
    hdfattribute_test_functionality
    hdfattribute_test_lifecycle
    hdfdataset_test_functionality
    hdfdataset_test_lifecycle
    hdfintegration_test
    hdfgroup_test_functionality
    hdfgroup_test_lifecycle
    monitor_test
    writetask_test
    datamanager_test
    factory_test
    hdf_ndim_io_test
    hdfdataspace_test
    # hdfobject_test
    hdfidentifier_test

    # this is a template for testing memory usage over time and intended for
    # closer scrutinization with sanitizers, valgrind or heap profilers.
    # included here for it to be build conveniently upon need

    # memory_test
    )

# Add DATAIO_-prefixed test targets
foreach(test ${TESTS_DATAIO})
    add_unit_test(NAME ${test}
                  GROUP dataio
                  SOURCES ${test}.cc)
endforeach(test)

# Tell monitor test if new output precision is required
target_compile_definitions(DATAIO_monitor_test
    PRIVATE
      $<$<VERSION_GREATER:${yaml-cpp_VERSION},0.6.2>:PRECISION_OUTPUT>
)
