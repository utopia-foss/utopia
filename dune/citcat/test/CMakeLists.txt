file(COPY "square.msh" "cube.msh" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(TESTS
	entity_test
	cell_test
	grid_test
	grid_cells_test_2d
	grid_cells_test_3d
)

if(PSGRAF_FOUND)
	dune_add_test(NAME output_epswriter_test SOURCES output_epswriter_test.cc)
	target_link_libraries(output_epswriter_test ${PSGRAF_LIBRARIES})
endif()

foreach(test ${TESTS})
	dune_add_test(NAME ${test} SOURCES "${test}.cc")
endforeach(test)
