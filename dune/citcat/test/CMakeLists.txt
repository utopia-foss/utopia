file(COPY "square.msh" "cube.msh" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(TESTS
	entity_test
	cell_test
	grid_test)

foreach(test ${TESTS})
	dune_add_test(NAME ${test} SOURCES "${test}.cc")
endforeach(test)