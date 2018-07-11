# place auxilary files in build directory
file(COPY "cfg_utils_test.yml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# register targets
set(TESTS_DATAIO
    cfg_utils_test
    hdfbufferfactory_test
    hdftypefactory_test
    hdffile_test
    hdfattribute_test_read
    hdfattribute_test_write
    hdfattribute_test_lifecycle
    hdfdataset_test_lifecycle
    hdfgroup_test_lifecycle
    hdfdataset_test_write
    # hdfdataset_test_read
    # hdfintegration_test
    # hdfgroup_test
    # hdfutilities_test
    )

# add DATAIO tests
foreach(test ${TESTS_DATAIO})
    dune_add_test(NAME DATAIO_${test} SOURCES ${test}.cc)

    # link library targets
    target_link_libraries(DATAIO_${test}
        spdlog
        yaml-cpp
    )

    # yaml-cpp does not export interface include dirs
    # NOTE This should be reported as bug or patched via MR in the yaml-cpp
    #      repository. See https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
    #      as a reference on how to do that.
    target_include_directories(DATAIO_${test}
        # register as system headers (compilers might ignore warnings)
        SYSTEM
        PRIVATE
            ${PROJECT_SOURCE_DIR}/vendor/plugins/yaml-cpp/include/
    )
endforeach(test)

# no matter how ctest is called, make sure of dependencies between these tests
set_tests_properties(DATAIO_hdfattribute_test_write PROPERTIES FIXTURES_SETUP H5IO)
set_tests_properties(DATAIO_hdfattribute_test_read PROPERTIES FIXTURES_REQUIRED H5IO)
# set_tests_properties(DATAIO_hdfdataset_test_write PROPERTIES FIXTURES_SETUP H5IO)
# set_tests_properties(DATAIO_hdfdataset_test_read PROPERTIES FIXTURES_REQUIRED H5IO)

# add custom target for 'DATAIO' prefix
add_custom_target(test_dataio
    COMMAND ctest --output-on-failure --tests-regex ^DATAIO_.+$)
