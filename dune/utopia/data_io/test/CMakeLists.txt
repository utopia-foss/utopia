file(COPY config_test.yaml DESTINATION .)

# collect DATAIO tests
set(TESTS_DATAIO
    config_test)

# add DATAIO tests
foreach(test ${TESTS_DATAIO})
    dune_add_test(NAME DATAIO_${test} SOURCES ${test}.cc)
endforeach(test)

# add custom target for 'DATAIO' prefix
add_custom_target(test_dataio
    COMMAND ctest --output-on-failure --tests-regex ^DATAIO_.+$)