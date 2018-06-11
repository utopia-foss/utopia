set(TESTS_SIMPLEEG
    SimpleEG_test
    )

# add SIMPLEEG tests
foreach(test ${TESTS_SIMPLEEG})
    dune_add_test(NAME SIMPLEEG_${test} SOURCES ${test}.cc)
endforeach(test)

# add custom target for 'SIMPLEEG' prefix
add_custom_target(test_simpleeg
    COMMAND ctest --output-on-failure --tests-regex ^SIMPLEEG_.+$)