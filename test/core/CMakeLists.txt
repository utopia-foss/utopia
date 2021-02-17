# place mesh and config files in build directory
file(COPY
        "agent_manager_test.yml"
        "agent_manager_integration_test.yml"
        "cell_manager_test.yml"
        "cell_manager_test.h5"
        "cell_manager_integration_test.yml"
        "graph_creation_test.yml"
        "grid_square_test.yml"
        "model_test.yml"
        "model_nested_test.yml"
        "model_setup_test.yml"
        "neighborhood_test.yml"
        "parallel_stl_test.yml"
        "select_test.yml"
        "space_test.yml"
        "testtools_test.yml"
        "model_datamanager_test.yml"
        "model_datamanager_test_custom.yml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# collect CORE tests
set(TESTS_CORE
    agent_manager_test
    agent_manager_integration_test
    agent_test
    apply_test
    assert_is_functional_test
    cell_manager_test
    cell_manager_integration_test
    dependency_test
    exceptions_test
    graph_test
    graph_apply_test
    graph_apply_doc_test
    graph_creation_test
    graph_entity_test
    graph_iterator_test
    grid_square_test
    model_test
    model_nested_test
    model_setup_test
    model_datamanager_test
    neighborhood_test
    parallel_stl_test
    select_test
    signal_test
    space_test
    state_test
    string_test
    tags_test
    testtools_test
    utils_test
    zip_test
    )

# Add CORE-prefixed test targets
foreach(test ${TESTS_CORE})
    add_unit_test(NAME ${test}
                  GROUP core
                  SOURCES ${test}.cc)
endforeach(test)

# Add the same test again for running it in parallel
add_unit_test(NAME apply_parallel_test
              GROUP core
              SOURCES apply_test.cc)

# Add parallel interface test
# NOTE: Can only be compiled with ParallelSTL available
if (HAVE_PSTL)
    add_unit_test(NAME parallel_interface_test
                GROUP core
                SOURCES parallel_interface_test.cc)
    enable_parallel(CORE_parallel_interface_test)
endif()

# Add parallel STL again test to run it sequentially
add_unit_test(NAME parallel_stl_seq_test
              GROUP core
              SOURCES parallel_stl_test.cc)
target_compile_definitions(CORE_parallel_stl_seq_test
    PRIVATE
        DISABLE_HAVE_PARALLEL_STL)

# Enable parallel features for these tests
enable_parallel(CORE_apply_parallel_test CORE_parallel_stl_test)

# Skip tests if dependencies for parallel execution are not found
set_tests_properties(CORE_apply_parallel_test
                     CORE_parallel_stl_test
    PROPERTIES
        DISABLED $<IF:$<BOOL:${HAVE_PSTL}>,FALSE,TRUE>)
