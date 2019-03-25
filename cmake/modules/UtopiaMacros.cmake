# BOOST
find_package(Boost 1.62 REQUIRED
             COMPONENTS unit_test_framework)

# HDF5
find_package(HDF5 1.10 REQUIRED COMPONENTS C HL)

# Define a target because the CMake module does not
add_library(hdf5 INTERFACE IMPORTED GLOBAL)

# TODO: Use `target_XXX` functions after raising CMake requirements
# Sanitize the definitions because we cannot use the proper CMake function...
string(REPLACE -D "" HDF5_C_DEFINITIONS "${HDF5_C_DEFINITIONS}")
# Set properties directly because of a bug in CMake 3.10
set_target_properties(hdf5 PROPERTIES
    INTERFACE_LINK_LIBRARIES "${HDF5_LIBRARIES};${HDF5_HL_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${HDF5_INCLUDE_DIRS}"
    INTERFACE_COMPILE_DEFINITIONS "${HDF5_C_DEFINITIONS}")

# FFTW3
find_package(FFTW3 3.3 REQUIRED)

# Armadillo
# use the config if possible
find_package(Armadillo CONFIG)
# fall back to our module if no target is defined
if (NOT TARGET armadillo)
    find_package(Armadillo MODULE REQUIRED)
endif()

# Threads (NOTE: required by spdlog on Ubuntu 18.04)
set(THREADS_PREFER_PTHREAD_FLAG)
find_package(Threads REQUIRED)

# include Utopia macros
include(UtopiaEnv)
include(UtopiaAddUnitTest)
include(UtopiaAddModel)
include(UtopiaAddModelTest)
include(UtopiaGenerateModelInfo)
include(UtopiaPython)
