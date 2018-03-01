# place mesh in build directory
file(COPY "square.msh" "cube.msh" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# collect CORE tests
set(TESTS_CORE
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
foreach(test ${TESTS_CORE})
    dune_add_test(NAME CORE_${test} SOURCES ${test}.cc)
endforeach(test)

# add EPSWriter test and link to PSGraf library
if(PSGRAF_FOUND)
    dune_add_test(NAME output_epswriter_test SOURCES output_epswriter_test.cc)
    target_link_libraries(output_epswriter_test ${PSGRAF_LIBRARIES})
endif()