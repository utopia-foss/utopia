# place mesh in build directory
file(COPY "square.msh" "cube.msh"
          "model_test.yml"
          "model_nested_test.yml"
          "model_setup_test.yml"
          "model_with_manager_test.yml"
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# collect CORE tests
set(TESTS_CORE
    apply_test
    agent_naive_test
    agent_test_2d
    agent_test_3d
    cell_test
    dependency_test
    grid_cells_test_2d
    grid_cells_test_3d
    grid_test
    model_test
    model_nested_test
    model_setup_test
    model_with_manager_test
    output_agent_test_gmsh
    output_agent_test_rect
    rng_test
    simulation_test
    state_test
    tags_test
    )

# add CORE tests
foreach(test ${TESTS_CORE})
    dune_add_test(NAME CORE_${test} SOURCES ${test}.cc)

    # link library targets
    target_link_libraries(CORE_${test}
        spdlog
        yaml-cpp
    )

    # yaml-cpp does not export interface include dirs
    # NOTE This should be reported as bug or patched via MR in the yaml-cpp
    #      repository. See https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
    #      as a reference on how to do that.
    target_include_directories(CORE_${test}
        # register as system headers (compilers might ignore warnings)
        SYSTEM
        PRIVATE
            ${PROJECT_SOURCE_DIR}/vendor/plugins/yaml-cpp/include/
    )
endforeach(test)

# add EPSWriter test and link to PSGraf library
# if(PSGRAF_FOUND)
#     dune_add_test(NAME CORE_output_epswriter_test SOURCES output_epswriter_test.cc)
#     target_link_libraries(output_epswriter_test ${PSGRAF_LIBRARIES})
# endif()

# add custom target for 'CORE' prefix
add_custom_target(test_core
    COMMAND ctest --output-on-failure --tests-regex ^CORE_.+$)
