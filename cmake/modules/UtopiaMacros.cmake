# Find dependencies and include Utopia CMake macros

# --- BOOST ---
find_package(Boost 1.67 REQUIRED
             COMPONENTS unit_test_framework)

# --- HDF5 ---
find_package(HDF5 1.10 REQUIRED COMPONENTS C HL)
include(RegisterHDF5)

# --- Armadillo ---
# Use the config if possible
# NOTE: Do not check version here because an incompatible version would lead to
#       CMake trying again in MODULE mode below!
find_package(Armadillo CONFIG QUIET)
# If found, check again with version specified
if (TARGET armadillo)
    find_package(Armadillo 9.600 CONFIG REQUIRED)
    message (STATUS "Found Armadillo from CMake config at: ${Armadillo_DIR}")
# Fall back to our module if no target is defined
else ()
    find_package(Armadillo MODULE REQUIRED)
    # Check for required Armadillo features because version cannot be determined
    message(VERBOSE "Unable to determine the version of Armadillo. Checking "
                    "features by compiling a simple test program...")
    try_compile(Armadillo_SUPPORT
                ${PROJECT_BINARY_DIR}/cmake/arma_features
                ${CMAKE_CURRENT_LIST_DIR}/../scripts/arma_features.cc
                LINK_LIBRARIES armadillo
                CXX_STANDARD 17
                OUTPUT_VARIABLE Armadillo_COMPILE_OUTPUT
    )
    if (NOT Armadillo_SUPPORT)
        message(SEND_ERROR "Armadillo does not support the required features! "
                           "Please install a supported version. Compiler "
                           "output of version test program:\n"
                           "${Armadillo_COMPILE_OUTPUT}"
        )
    endif()
endif()

# --- spdlog ---
find_package(spdlog 1.3 REQUIRED)

# --- yaml-cpp ---
find_package(yaml-cpp 0.6.2 REQUIRED)

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
