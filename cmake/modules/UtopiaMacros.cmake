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
# We cover three cases:
# 1. The standard library has PSTL definitions and requires TBB externally.
# 2. The standard library has no PSTL definitions and we search for ParallelSTL.
# 3. The standard library has PSTL definitions but the user wants to use
#    an external ParallelSTL package, which they indicate by setting
#    ParallelSTL_ROOT. In this case, we also check for TBB again if ParallelSTL
#    could not be found.
#
# Result variables:
# - HAVE_PSTL : If TRUE, multithreading is enabled.
# - ParallelSTL_FOUND: If TRUE, ParallelSTL is found and used.
# - INTERNAL_PSTL: If TRUE, ParallelSTL definitions are natively available.
# - INTERNAL_PSTL_WITH_TBB: If TRUE, INTERNAL_PSTL is TRUE and the native
#                           definitions are used with the external TBB library.
#

if (DEFINED ENV{ParallelSTL_ROOT} OR DEFINED CACHE{ParallelSTL_ROOT})
    set(PRIORITIZE_ParallelSTL TRUE)
endif()

# Check PSTL availability from standard library
include(CheckIncludeFileCXX)
check_include_file_cxx(execution INTERNAL_PSTL)

# --- ParallelSTL ---
# Search alternative definition of <execution>
# NOTE: ParallelSTL requires TBB and will search for it
if(NOT INTERNAL_PSTL OR PRIORITIZE_ParallelSTL)
    find_package(ParallelSTL)

    # NOTE: Default installation of ParallelSTL through Homebrew is faulty.
    #       Therefore, check if the header can actually be included.
    if (ParallelSTL_FOUND)
        get_target_property(PSTL_INCLUDE_DIRS
            pstl::ParallelSTL INTERFACE_INCLUDE_DIRECTORIES)

        include(UtopiaCheckPath)
        check_path(EXIST_VAR PSTL_INCLUDE_EXISTS
                   DIRECTORIES ${PSTL_INCLUDE_DIRS})

        if (NOT PSTL_INCLUDE_EXISTS)
            message(WARNING "ParallelSTL was detected but could NOT be "
                            "included! This can happen with Homebrew "
                            "installations. Please consult the README.md")

            # Undo effects of "find_package()"
            unset(ParallelSTL_FOUND)
            unset(ParallelSTL_DIR CACHE)
        endif()
    endif()
endif()

# --- Thread Building Blocks (TBB) ---
# Required for parallel policies if execution header is natively available
if (INTERNAL_PSTL AND NOT ParallelSTL_FOUND)
    find_package(TBB 2018.5)
endif()

# Multithreading summary variables
if (INTERNAL_PSTL AND TBB_FOUND AND NOT ParallelSTL_FOUND)
    set(INTERNAL_PSTL_WITH_TBB TRUE)
endif()
if (INTERNAL_PSTL_WITH_TBB OR ParallelSTL_FOUND)
    set(HAVE_PSTL TRUE)
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
