# Set minimum CMake version to the one supplied with Ubuntu 18.04
cmake_minimum_required(VERSION 3.10)

# Start the project
project(utopia
    DESCRIPTION "Library for exploring complex & evolving model systems."
    LANGUAGES C CXX
)

# --- CMake Modules ---

# Insert Utopia macros on top of the module path list
list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake/modules)

# Load the Utopia macros and execute them here
# (This checks dependencies, enables CMake functions, etc.)
include(UtopiaMacros)

# add extra flags to debug compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")

# --- Utopia Target ---

# Register utopia as interface library (does not actually build objects)
add_library(utopia INTERFACE)
# Require C++17 support
target_compile_features(utopia INTERFACE cxx_std_17)
# Add include directories
target_include_directories(utopia INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include> # NOTE: not used for now
)

# Link with required libraries
# link library targets
target_link_libraries(utopia INTERFACE
    armadillo
    Boost::boost
    hdf5
    spdlog
    yaml-cpp
)

# yaml-cpp does not export interface include dirs
# NOTE This should be reported as bug or patched via MR in the yaml-cpp
#      repository. See https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
#      as a reference on how to do that.
target_include_directories(utopia
    # register as system headers (compilers might ignore warnings)
    SYSTEM
    INTERFACE
        ${PROJECT_SOURCE_DIR}/plugins/vendor/yaml-cpp/include/
)

# --- Include Config Tree ---

# Enable testing via CTest engine
enable_testing()

# Include subdirectories
add_subdirectory(plugins/vendor)
add_subdirectory(python)
add_subdirectory(doc)
add_subdirectory(src/models)
add_subdirectory(test EXCLUDE_FROM_ALL)
