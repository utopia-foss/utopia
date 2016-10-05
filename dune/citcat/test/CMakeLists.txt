set(TESTS
	entity_test
	cell_test)

foreach(test ${TESTS})
	add_executable(${test} "${test}.cc")
	target_link_dune_default_libraries(${test})
	add_test(${test} ${test})
endforeach(test)

add_directory_test_target(_test_target)
add_dependencies(${_test_target} ${TESTS})