# New CMake policy stack
cmake_policy(PUSH)


# Use <Package>_ROOT variables in respective find_package functions/modules
if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif ()

# BOOST
find_package(Boost 1.62 REQUIRED
             COMPONENTS unit_test_framework)

# HDF5
find_package(HDF5 1.10 REQUIRED COMPONENTS C HL)
include(RegisterHDF5)

# Armadillo
# use the config if possible
find_package(Armadillo CONFIG QUIET)
# fall back to our module if no target is defined
if (TARGET armadillo)
    message (STATUS "Found Armadillo from CMake config at: ${Armadillo_DIR}")
else ()
    find_package(Armadillo MODULE REQUIRED)
endif()

# Threads (NOTE: required by spdlog on Ubuntu 18.04)
set(THREADS_PREFER_PTHREAD_FLAG)
find_package(Threads REQUIRED)

# Doxygen (for documentation only)
find_package(Doxygen
             OPTIONAL_COMPONENTS dot)

# Include Utopia macros
include(UtopiaPython)
include(UtopiaEnv)
include(UtopiaFrontendModelRegistry)

include(UtopiaAddUnitTest)
include(UtopiaAddModel)
include(UtopiaAddModelTest)


# Remove latest policy stack
cmake_policy(POP)
