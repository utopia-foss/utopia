add_executable("entity_test" entity_test.cc)
target_link_dune_default_libraries("entity_test")
add_test("entity_test" "entity_test")