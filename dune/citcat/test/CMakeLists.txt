if(NOT PSGRAF_ROOT)
	set(PSGRAF_ROOT)
endif()
find_package(PSGraf CONFIG
	HINTS ${PSGRAF_ROOT})

if(PSGRAF_FOUND)
	include_directories(${PSGRAF_INCLUDE_DIRS})
	# target_link_libraries(/Users/JulianWeninger/psgraf/build ${PSGRAF_LIBRARIES})
endif()

file(COPY "square.msh" "cube.msh" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(TESTS
	entity_test
	cell_test
	grid_test
	grid_cells_test_2d
	grid_cells_test_3d
	EPSwriter_test
)

foreach(test ${TESTS})
	dune_add_test(NAME ${test} SOURCES "${test}.cc")
endforeach(test)

target_link_libraries(EPSwriter_test ${PSGRAF_LIBRARIES})