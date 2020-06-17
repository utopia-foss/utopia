# CMake module to find the TBB includes and library.
#
# This will import the targets
#
#   TBB::tbb              Library to link dependent objects against
#
# and set the following variables
#
#   - TBB_FOUND           True if TBB was found
#   - TBB_INCLUDE_DIRS    Location of main TBB header
#   - TBB_LIBRARIES       Location of TBB library
#
# It uses hints provided by the CMake or environment variables
#
#   - TBB_ROOT            Path to the TBB install directory
#
# This module first tries to detect the library via pkg-config (if available).
# Pkg-config can detect the version of the target library which is otherwise
# only possible through the package's CMake config.
#

# Try piggybacking pkg-config if available
find_package(PkgConfig QUIET)
pkg_check_modules(PC_TBB QUIET IMPORTED_TARGET GLOBAL tbb)

# Find the library itself
find_library(TBB_LIBRARY
    NAMES tbb
    PATH_SUFFIXES lib
    HINTS ${PC_TBB_LIBRARY_DIRS}
)

# Find the main header
find_path(TBB_INCLUDE_DIR
    tbb.h
    PATH_SUFFIXES include include/tbb
    HINTS ${PC_TBB_INCLUDE_DIRS}
)

# Report if package was found
include(FindPackageHandleStandardArgs)
# Report version if pkg-config could detect it
if (PC_TBB_VERSION)
    find_package_handle_standard_args(TBB
        REQUIRED_VARS TBB_LIBRARY TBB_INCLUDE_DIR
        VERSION_VAR PC_TBB_VERSION
    )
else()
    find_package_handle_standard_args(TBB
        DEFAULT_MSG
        TBB_LIBRARY TBB_INCLUDE_DIR
    )
endif()

mark_as_advanced(TBB_LIBRARY TBB_INCLUDE_DIR)

# Set output variables
if (TBB_FOUND)
    set(TBB_LIBRARIES ${TBB_LIBRARY})
    set(TBB_INCLUDE_DIRS ${TBB_INCLUDE_DIR})

    # Add the library target
    # Use alias if pkg-config created one already
    if (TARGET PkgConfig::PC_TBB)
        add_library(TBB::tbb ALIAS PkgConfig::PC_TBB)
    else ()
        add_library(TBB::tbb UNKNOWN IMPORTED GLOBAL)
        set_target_properties(TBB::tbb
            PROPERTIES IMPORTED_LOCATION ${TBB_LIBRARIES}
                       INTERFACE_INCLUDE_DIRECTORIES ${TBB_INCLUDE_DIRS}
        )
    endif ()
endif()
