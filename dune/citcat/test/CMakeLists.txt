set(TESTS
	entity_test
	cell_test)

foreach(test ${TESTS})
	dune_add_test(NAME ${test} SOURCES "${test}.cc")
endforeach(test)