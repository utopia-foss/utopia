set(TESTS
	entity_test)

add_executable("entity_test" entity_test.cc)
target_link_dune_default_libraries("entity_test")

foreach(TEST ${TESTS})
  add_test(${TEST} ${TEST})
endforeach(TEST)

add_directory_test_target(_test_target)
add_dependencies(${_test_target} ${TESTS})