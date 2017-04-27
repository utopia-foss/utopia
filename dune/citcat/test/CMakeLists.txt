file(COPY "square.msh" "cube.msh" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	set(COMPILE_FLAGS_TEST -Wall -Wextra)
endif()

set(TESTS
	agent_test
	entity_test
	cell_test
	grid_test
	grid_cells_test_2d
	grid_cells_test_3d)

foreach(test ${TESTS})
	dune_add_test(NAME ${test} SOURCES "${test}.cc" COMPILE_FLAGS ${COMPILE_FLAGS_TEST})
endforeach(test)