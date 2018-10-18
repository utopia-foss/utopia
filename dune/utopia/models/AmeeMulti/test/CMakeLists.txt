# place auxilary files in build directory
file(COPY "multi_test_config_complex.yml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "multi_test_config_simple.yml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})



# register targets
set(TESTS_AMEEMULTI
    memorypool_test
    multi_agentstatecomplex_test
    multi_agentstatesimple_test
    multi_model_test
    multi_generatortest
    )

# add AMEEMULTI tests
foreach(test ${TESTS_AMEEMULTI})
    dune_add_test(NAME AMEEMULTI_${test} SOURCES ${test}.cc)

    # link library targets
    target_link_libraries(AMEEMULTI_${test}
        spdlog
        yaml-cpp
    )

    # yaml-cpp does not export interface include dirs
    # NOTE This should be reported as bug or patched via MR in the yaml-cpp
    #      repository. See https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
    #      as a reference on how to do that.
    target_include_directories(AMEEMULTI_${test}
        # register as system headers (compilers might ignore warnings)
        SYSTEM
        PRIVATE
            ${PROJECT_SOURCE_DIR}/vendor/plugins/yaml-cpp/include/
    )
endforeach(test)


# add custom target for 'AMEEMULTI' prefix
add_custom_target(test_ameemulti
    COMMAND ctest --output-on-failure --tests-regex ^AMEEMULTI_.+$)
