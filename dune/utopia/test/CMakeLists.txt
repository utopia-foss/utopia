# place mesh in build directory
file(COPY "square.msh" "cube.msh" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# add extra debugging flags for debug builds
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(COMPILE_FLAGS_TEST -Wall -Wextra -g3)
endif()

# collect tests
set(TESTS
    agent_test_2d
    agent_test_3d
    entity_test
    cell_test
    grid_test
    grid_cells_test_2d
    grid_cells_test_3d
    output_agent_test_gmsh
    output_agent_test_rect
    simulation_test)

# add all tests
foreach(test ${TESTS})
    dune_add_test(NAME ${test} SOURCES "${test}.cc"
        COMPILE_FLAGS ${COMPILE_FLAGS_TEST})
endforeach(test)

# add EPSWriter test and link to PSGraf library
if(PSGRAF_FOUND)
    dune_add_test(NAME output_epswriter_test SOURCES output_epswriter_test.cc)
    target_link_libraries(output_epswriter_test ${PSGRAF_LIBRARIES})
endif()