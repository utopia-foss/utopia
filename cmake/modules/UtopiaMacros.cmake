# Find dependencies and include Utopia CMake macros

# --- BOOST ---
find_package(Boost 1.67 REQUIRED
             COMPONENTS unit_test_framework)

# --- HDF5 ---
find_package(HDF5 1.10 REQUIRED COMPONENTS C HL)
include(RegisterHDF5)

# --- Armadillo ---
# use the config if possible
find_package(Armadillo CONFIG QUIET)
# fall back to our module if no target is defined
if (TARGET armadillo)
    message (STATUS "Found Armadillo from CMake config at: ${Armadillo_DIR}")
else ()
    find_package(Armadillo MODULE REQUIRED)
endif()

# --- Threads (NOTE: required by spdlog on Ubuntu 18.04) ---
set(THREADS_PREFER_PTHREAD_FLAG)
find_package(Threads REQUIRED)


# --- Doxygen (for documentation only) ---
find_package(Doxygen
             OPTIONAL_COMPONENTS dot)


# --- Include Utopia macros ---
include(UtopiaPython)
include(UtopiaEnv)
include(UtopiaFrontendModelRegistry)

include(UtopiaAddCoverageFlags)
include(UtopiaAddUnitTest)
include(UtopiaAddModel)
include(UtopiaAddModelTest)
