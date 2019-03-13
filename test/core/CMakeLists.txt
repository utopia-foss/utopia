# place mesh and config files in build directory
file(COPY "agent_manager_test.yml"
          "agent_manager_integration_test.yml"
          "cell_manager_test.yml"
          "cell_manager_integration_test.yml"
          "grid_square_test.yml"
          "model_test.yml"
          "model_nested_test.yml"
          "model_setup_test.yml"
          "neighborhood_test.yml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# collect CORE tests
set(TESTS_CORE
    agent_manager_test
    agent_manager_integration_test
    agent_test
    apply_test
    cell_manager_test
    cell_manager_integration_test
    dependency_test
    exceptions_test
    graph_test
    grid_square_test
    model_test
    model_nested_test
    model_setup_test
    neighborhood_test
    signal_test
    state_test
    tags_test
    )

# add CORE tests
foreach(test ${TESTS_CORE})
    add_executable(CORE_${test} ${test}.cc)
    target_link_libraries(CORE_${test} PUBLIC utopia)
    add_test(NAME CORE_${test} COMMAND CORE_${test})
    add_dependencies(build_unit_tests CORE_${test})
endforeach(test)

# custom dependencies of dependency test
target_link_libraries(CORE_dependency_test PUBLIC FFTW3::fftw3)

# add custom target for 'CORE' prefix
add_custom_target(test_core
    COMMAND ctest --output-on-failure --tests-regex ^CORE_.+$)