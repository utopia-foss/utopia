# Find dependencies and include Utopia CMake macros

# --- BOOST ---
find_package(Boost 1.67 REQUIRED
             COMPONENTS unit_test_framework graph regex)

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

# --- Multithreading Capabilities ---
#
# NOTE: Multithreading will only be enabled if MULTITHREADING is ON.
#       But dependencies are always checked. This must be reflected in the
#       UtopiaConfig.cmake.in file!
#
# We cover two cases in the following order of importance:
# 1. Intel oneDPL is installed. then we use it exclusively.
# 2. Intel oneDPL is not installed, but the standard library has PSTL
#    definitions and TBB is installed.
#
# The latter does not apply to macOS where incompatibilities have been found for
# macOS 11 "Big Sur" and AppleClang 11.
#
# Result variables:
# - HAVE_PSTL : If TRUE, multithreading is enabled.
# - oneDPL_FOUND: If TRUE, oneDPL is found and used.
# - INTERNAL_PSTL: If TRUE, PSTL definitions are natively available.
# - INTERNAL_PSTL_WITH_TBB: If TRUE, INTERNAL_PSTL is true and TBB is found
#                           (this variable is used for correct linking)
#

# --- Intel oneDPL ---
# Required for macOS (see above)
if (MULTITHREADING AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MAYBE_REQUIRED REQUIRED)
endif()
find_package(oneDPL ${MAYBE_REQUIRED})

# --- Thread Building Blocks (TBB) ---
# Required for parallel policies if execution header is natively available
# NOTE: Check this before testing for internal execution header because
#       GCC 11 cannot include the header without linking to TBB
if (MULTITHREADING AND NOT oneDPL_FOUND)
    set(MAYBE_REQUIRED REQUIRED)
endif()
find_package(TBB 2018.5 ${MAYBE_REQUIRED})

# Check PSTL availability from standard library
include(CheckIncludeFileCXX)
cmake_policy(PUSH)
cmake_policy(SET CMP0075 NEW)
if(TBB_FOUND)
    # Link test executable with TBB header for GCC 11 compatibility
    set(CMAKE_REQUIRED_LIBRARIES TBB::tbb)
endif()
check_include_file_cxx(execution INTERNAL_PSTL)
cmake_policy(POP)

# Multithreading summary variables
if (MULTITHREADING)
    if (oneDPL_FOUND)
        set(HAVE_PSTL TRUE)
    elseif (TBB_FOUND AND INTERNAL_PSTL)
        set(HAVE_PSTL TRUE)
        set(INTERNAL_PSTL_WITH_TBB TRUE)
    elseif (TBB_FOUND AND NOT INTERNAL_PSTL)
        message(SEND_ERROR "Multithreading is ON and TBB was found but the "
                           "standard library does not provide PSTL "
                           "definitions. Consider installing Intel oneDPL.")
    else ()
        message(SEND_ERROR "Multithreading is ON but dependencies could not "
                           "be found!")
    endif()
endif()

# Add Multithreading as listed feature
include(FeatureSummary)
add_feature_info(Multithreading
                 HAVE_PSTL
                 "parallel execution inside Utopia models")

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
include(UtopiaEnableParallel)
include(UtopiaAddUnitTest)
include(UtopiaAddModel)
include(UtopiaAddModelTest)
include(UtopiaCheckPath)
