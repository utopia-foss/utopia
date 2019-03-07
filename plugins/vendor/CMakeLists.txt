# disable spdlog tests and examples
set(SPDLOG_BUILD_TESTING OFF)
set(SPDLOG_BUILD_EXAMPLES OFF)
add_subdirectory(spdlog EXCLUDE_FROM_ALL)

# disable yaml-cpp build tests and tools
set(YAML_CPP_BUILD_TESTS OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_CPP_BUILD_CONTRIB OFF)
add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL)
