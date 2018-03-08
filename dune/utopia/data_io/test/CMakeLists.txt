file(COPY config_test.yaml DESTINATION .)

# collect DATAIO tests
set(TESTS_DATAIO
    config_test
    hdfattribute_test_read
    hdfattribute_test_write
    hdfbufferfactory_test
    hdftypefactory_test
    hdfgroup_test)

# add DATAIO tests
foreach(test ${TESTS_DATAIO})
    dune_add_test(NAME DATAIO_${test} SOURCES ${test}.cc)
endforeach(test)

# no matter how ctest is called, make sure of dependencies between these tests
set_tests_properties(DATAIO_hdfattribute_test_write PROPERTIES FIXTURES_SETUP H5IO)
set_tests_properties(DATAIO_hdfattribute_test_read PROPERTIES FIXTURES_REQUIRED H5IO)

# add custom target for 'DATAIO' prefix
add_custom_target(test_dataio
    COMMAND ctest --output-on-failure --tests-regex ^DATAIO_.+$)