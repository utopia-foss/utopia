# Set minimum CMake version to the one supplied with Ubuntu 18.04
cmake_minimum_required(VERSION 3.10)

# Start the project
project(Utopia
    DESCRIPTION "Library for exploring complex & evolving model systems."
    LANGUAGES C CXX
    VERSION 0.1
)

# Use 'Release' as build type of none was selected by the user.
# NOTE: This is required due to https://github.com/jbeder/yaml-cpp/issues/706
if (CMAKE_BUILD_TYPE STREQUAL "")
    message(WARNING "No CMake build type specified. Selecting build type "
                    "'None'. Use -DCMAKE_BUILD_TYPE=<type> when calling "
                    "CMake to set a custom build type.")
    set (CMAKE_BUILD_TYPE None)
endif()

# --- CMake Modules ---

# Insert Utopia macros on top of the module path list
set(module_path ${PROJECT_SOURCE_DIR}/cmake/modules)
list(INSERT CMAKE_MODULE_PATH 0 ${module_path})

# Load the Utopia macros and execute them here
# (This checks dependencies, enables CMake functions, etc.)
include(UtopiaMacros)

# add extra flags to debug compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
# NOTE The ${CMAKE_CXX_FLAGS_DEBUG} variable can be set in the cache. This is,
#      for example, used in the CI/CD pipeline to add additional flags to DEBUG

# --- Utopia Target ---

# Register utopia as interface library (does not actually build objects)
add_library(utopia INTERFACE)
add_library(Utopia::utopia ALIAS utopia)

# Require C++17 support
target_compile_features(utopia INTERFACE cxx_std_17)

# Add include directories
target_include_directories(utopia INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include> # NOTE: not used for now
)

# Link with required libraries
target_link_libraries(utopia INTERFACE
    armadillo
    Boost::boost
    hdf5
    spdlog
    Threads::Threads # NOTE: required by spdlog on Ubuntu 18.04
    yaml-cpp
)

# yaml-cpp does not export interface include dirs
# NOTE This should be reported as bug or patched via MR in the yaml-cpp
#      repository. See https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
#      as a reference on how to do that.
# NOTE: Add this path only in the build context because yaml-cpp will be
#       inside the above include path after installation
target_include_directories(utopia
    # register as system headers (compilers might ignore warnings)
    SYSTEM
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/plugins/vendor/yaml-cpp/include/>
)

# --- Include Config Tree ---

# Enable testing via CTest engine
enable_testing()

# Include subdirectories
add_subdirectory(plugins/vendor)
add_subdirectory(doc)
add_subdirectory(src/models)
add_subdirectory(python)
add_subdirectory(test EXCLUDE_FROM_ALL)

# Add a test target to rule them all
add_custom_target(test_all)
add_dependencies(test_all
                    test_core
                    test_dataio
                    test_utopya
                    test_models)

# --- Build Tree Packaging Routine ---

# Functions for writing config files
include(CMakePackageConfigHelpers)

# Define filenames and directories for the configuration and installation
set(namespace ${PROJECT_NAME}::)
set(cmake_config_name ${PROJECT_NAME}Config.cmake)
set(version_config ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake)
set(targets_export_name ${PROJECT_NAME}Targets)
set(cmake_modules_dir ${PROJECT_SOURCE_DIR}/cmake/modules)
set(spdlog_config_dir ${PROJECT_BINARY_DIR}/plugins/vendor/spdlog)
set(yaml-cpp_config_dir ${PROJECT_BINARY_DIR}/plugins/vendor/yaml-cpp)

# Write a version file
write_basic_package_version_file(
    ${version_config}
    COMPATIBILITY SameMajorVersion
)

# Configure the config file for the build tree
configure_package_config_file(
    cmake/${cmake_config_name}.in
    ${cmake_config_name}
    INSTALL_DESTINATION ${PROJECT_BINARY_DIR}
    PATH_VARS cmake_modules_dir
              spdlog_config_dir
              yaml-cpp_config_dir
)

# --- Installation Routine ---

# Define installation paths according to GNU standards
include(GNUInstallDirs)

# Define installation filenames and directories
set(config_install_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
set(cmake_modules_dir ${config_install_dir}/modules)

# Configure the config file for the install tree
configure_package_config_file(
    cmake/${cmake_config_name}.in
    cmake/${cmake_config_name}
    INSTALL_DESTINATION ${config_install_dir}
    PATH_VARS CMAKE_INSTALL_PREFIX
              cmake_modules_dir
)

# Install version and config file
install(
    FILES ${version_config}
          ${PROJECT_BINARY_DIR}/cmake/${cmake_config_name}
    DESTINATION ${config_install_dir}
)

# Install the CMake modules
install(
    DIRECTORY cmake/modules/
    DESTINATION ${cmake_modules_dir}
    # Exclude files that are only used during the build
    PATTERN "UtopiaMacros.cmake" EXCLUDE
    PATTERN "UtopiaEnv.cmake" EXCLUDE
)

# Install the C++ headers
install(
    DIRECTORY include/
    # NOTE: CMake >=v3.14 supports TYPE argument
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hh"
)

# Install all our targets!
install(
    TARGETS utopia
    # register this name as the target export
    EXPORT ${targets_export_name}
    # NOTE: Trivial DESTINATIONS are not required for CMake >=v3.14
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE
)

# Install the target export
install(
    EXPORT ${targets_export_name}
    NAMESPACE ${namespace}
    DESTINATION ${config_install_dir}
)

# --- Build Tree Packaging ---

# Export the targets also into the build tree
export(
    EXPORT ${targets_export_name}
    NAMESPACE ${namespace}
    FILE ${targets_export_name}.cmake
)

# Register this package in the CMake package registry
export(PACKAGE ${PROJECT_NAME})
