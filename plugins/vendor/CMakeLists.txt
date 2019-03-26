# disable spdlog tests and examples
set(SPDLOG_BUILD_TESTING OFF
    CACHE BOOL "Option for building tests of spdlog")
set(SPDLOG_BUILD_EXAMPLES OFF
    CACHE BOOL "Option for building example executables of spdlog")
add_subdirectory(spdlog EXCLUDE_FROM_ALL)

# disable yaml-cpp build tests and tools
set(YAML_CPP_BUILD_TESTS OFF
    CACHE BOOL "Option for building tests of yaml-cpp")
add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL)
