# Set minimum CMake version to the one supplied with Ubuntu 20.04
cmake_minimum_required(VERSION 3.16)

# Start the project
project(Utopia
    DESCRIPTION "Framework for studying models of complex & adaptive systems."
    LANGUAGES C CXX
    VERSION 2.0.0
)

# Use 'Release' as build type of none was selected by the user.
# NOTE: This is required due to https://github.com/jbeder/yaml-cpp/issues/706
if (CMAKE_BUILD_TYPE STREQUAL "")
    message(WARNING "No CMake build type specified. Selecting build type "
                    "'None'. Use -DCMAKE_BUILD_TYPE=<type> when calling "
                    "CMake to set a custom build type.")
    set (CMAKE_BUILD_TYPE None)
endif()

# --- Parse CMake options ---

# Multithreading
option(MULTITHREADING "Enable multithreading through optional dependencies")

# C++ coverage reports
# NOTE: Option is only presented if CMAKE_BUILD_TYPE is not set to 'Release'.
#       It always defaults to OFF and will always be OFF for 'Release' builds.
include(CMakeDependentOption)
cmake_dependent_option(CPP_COVERAGE "Compile C++ code with coverage flags" OFF
                       "NOT CMAKE_BUILD_TYPE STREQUAL Release" OFF)

# --- CMake Modules ---

# Insert Utopia macros on top of the module path list
set(module_path ${PROJECT_SOURCE_DIR}/cmake/modules)
list(INSERT CMAKE_MODULE_PATH 0 ${module_path})

# Load the Utopia macros and execute them here
# (This checks dependencies, enables CMake functions, etc.)
include(UtopiaMacros)

# add extra flags to debug compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
# NOTE It is safest to use the CXXFLAGS environment variable to set additional
#      flags when calling CMake for the first time. This is also done in the
#      GitLab CI.

# --- Utopia Target ---

# Register utopia as interface library (does not actually build objects)
add_library(utopia INTERFACE)
add_library(Utopia::utopia ALIAS utopia)

# Require C++17 support
target_compile_features(utopia INTERFACE cxx_std_17)

# Add include directories
target_include_directories(utopia INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/>
    $<INSTALL_INTERFACE:include>
)

# Link with required libraries
# NOTE: Plugins might also be located in system paths, so they have to be
#       linked *first* such that their include directories are searched first.
target_link_libraries(utopia INTERFACE
    spdlog::spdlog
    yaml-cpp
    Threads::Threads # NOTE: required by spdlog on Ubuntu 18.04
    armadillo
    Boost::boost
    Boost::graph
    Boost::regex
    hdf5
)

# Set preprocessor definitions
target_compile_definitions(utopia INTERFACE
    # Enable HDF5 functions of Armadillo
    ARMA_USE_HDF5
)

# Multithreading dependencies
if (MULTITHREADING)
    target_link_libraries(utopia INTERFACE
        $<$<BOOL:${oneDPL_FOUND}>:oneDPL>
        $<$<BOOL:${INTERNAL_PSTL_WITH_TBB}>:TBB::tbb>
    )
    # Propagate availability of PSTL definitions
    target_compile_definitions(utopia INTERFACE
        $<$<BOOL:${oneDPL_FOUND}>:HAVE_ONEDPL>
        $<$<BOOL:${INTERNAL_PSTL_WITH_TBB}>:USE_INTERNAL_PSTL>
    )
endif()


# --- Include Config Tree ---

# Enable testing via CTest engine
enable_testing()

# Include subdirectories
add_subdirectory(doc)
add_subdirectory(src/utopia/models)
add_subdirectory(python)
add_subdirectory(test EXCLUDE_FROM_ALL)

# Add test targets to rule them all
add_custom_target(build_tests_all)
add_dependencies(build_tests_all
                    build_tests_backend
                    build_tests_models)

add_custom_target(test_all)
add_dependencies(test_all
                    test_core
                    test_dataio
                    test_models)

# --- Summary ---

# Summarize detected packages
include(FeatureSummary)
feature_summary(QUIET_ON_EMPTY
                WHAT REQUIRED_PACKAGES_FOUND
                     OPTIONAL_PACKAGES_FOUND
                     OPTIONAL_PACKAGES_NOT_FOUND
                     ENABLED_FEATURES
                     DISABLED_FEATURES
                     REQUIRED_PACKAGES_NOT_FOUND)

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
              UTOPIA_ENV_EXECUTABLE
              UTOPIA_ENV_PIP
              UTOPIA_ENV_DIR
              RUN_IN_UTOPIA_ENV
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
# Headers in the src/ dir will be moved into the install include/ dir
install(
    DIRECTORY include/ src/
    # NOTE: CMake >=v3.14 supports TYPE argument
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hh"
)

# Install all our targets!
install(
    TARGETS utopia
    # register this name as the target export
    EXPORT ${targets_export_name}
    RUNTIME PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
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
