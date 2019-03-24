# Add targets that bundle building and execution of all backend tests
add_custom_target(build_tests_backend)
add_custom_target(test_backend)

# Continue in the config tree to include the tests
add_subdirectory(core)
add_subdirectory(data_io)
