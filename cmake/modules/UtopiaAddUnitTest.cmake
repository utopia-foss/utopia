# Add targets that bundle building and execution of all backend tests
add_custom_target(build_tests_backend
                  COMMENT "Compiling all backend tests")
add_custom_target(test_backend
                  COMMENT "Executing all backend tests")
add_dependencies(test_backend build_tests_backend)

#
# Add a unit test executable and register it to a unit testing group.
#
# This function takes the following arguments:
#
#   - NAME (single)     Name of the test. Will be prepended with a prefix.
#   - GROUP (single)    Unit test group. The uppercase group name will be
#                       prepended to the unit test name and build and test
#                       targets with the lowercase group name will be created.
#   - SOURCES (multi)   The source files for the test executable.
#
# After this function is applied once, there will be a build target
# `<GROUP>_<NAME>`, a test `<GROUP>_<NAME>`, and (possibly collective)
# build and test targets `build_tests_<group>`, and `test_<group>`,
# respectively.
#
function(add_unit_test)

    # parse the function arguments
    include(CMakeParseArguments)
    set(OPTIONS)
    set(SINGLE NAME GROUP)
    set(MULTI SOURCES)
    cmake_parse_arguments(KW "${OPTIONS}" "${SINGLE}" "${MULTI}" ${ARGN})

    if (KW_UNPARSED_ARGUMENTS)
        message(WARNING "Encountered unparsed arguments in 'add_unit_test': "
                        "${KW_UNPARSED_ARGUMENTS}")
    endif ()

    # create uppercase and lowercase representations of group name
    string(TOLOWER ${KW_GROUP} group_lower)
    string(TOUPPER ${KW_GROUP} group_upper)

    # create the collective build target
    set(collective_build build_tests_${group_lower})
    if(NOT TARGET ${collective_build})
        add_custom_target(${collective_build}
                          COMMENT "Building backend tests of group: "
                                  "${group_upper}"
        )

        # add as dependency to the global backend test build target
        add_dependencies(build_tests_backend ${collective_build})
    endif()

    # create the collective test target
    set(collective_test test_${group_lower})
    if(NOT TARGET ${collective_test})
        add_custom_target(${collective_test}
            COMMAND ctest --output-on-failure
                          --tests-regex ^${group_upper}_.+$
            COMMENT "Executing backend tests of group: ${group_upper}"
        )

        # build all tests before running them
        add_dependencies(${collective_test} ${collective_build})

        # add as dependency to the global backend test target
        add_dependencies(test_backend ${collective_test})
    endif()

    # create the test executable
    set(unit_test ${group_upper}_${KW_NAME})
    add_executable(${unit_test} ${KW_SOURCES})
    target_link_libraries(${unit_test}
        PUBLIC utopia Boost::unit_test_framework)
    # NOTE: Might lead to issues if linked to the static unit test library
    target_compile_definitions(${unit_test}
        PRIVATE BOOST_TEST_DYN_LINK)

    # add this build target to the collective build target
    add_dependencies(${collective_build} ${unit_test})

    # create the test
    # (will be picked up by the collective test via Regex match)
    add_test(NAME ${unit_test}
             COMMAND ${unit_test} -r detailed)

endfunction()
