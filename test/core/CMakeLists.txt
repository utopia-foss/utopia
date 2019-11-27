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
        "select_test.yml"
        "model_datamanager_test.yml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# collect CORE tests
set(TESTS_CORE
    agent_manager_test
    agent_manager_integration_test
    agent_test
    apply_test
    apply_graph_test
    cell_manager_test
    cell_manager_integration_test
    dependency_test
    exceptions_test
    graph_test
    graph_creation_test
    graph_iterator_test
    grid_square_test
    model_test
    model_nested_test
    model_setup_test
    model_datamanager_test
    neighborhood_test
    select_test
    signal_test
    state_test
    tags_test
    utils_test
    zip_test
    )

# Add CORE-prefixed test targets
foreach(test ${TESTS_CORE})
    add_unit_test(NAME ${test}
                  GROUP core
                  SOURCES ${test}.cc)
endforeach(test)

# custom dependencies of certain tests can be added below
