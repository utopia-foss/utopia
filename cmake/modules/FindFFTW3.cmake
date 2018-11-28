# CMake script to find the native FFTW3 includes and library
#
# This will import the targets
#
#   FFTW3::fftw3
#
# and the following variables
#
#   - FFTW3_FOUND           True if FFTW was found
#   - FFTW3_INCLUDE_DIRS    Location of fftw3.h
#   - FFTW3_LIBRARIES       List of libraries when using FFTW
#

# use pkg-config to help with module parameters
# the found variables are prefixed with "PC_FFTW3"
find_package(PkgConfig)
pkg_check_modules(PC_FFTW3 QUIET fftw3)

# extract version
set(FFTW3_VERSION ${PC_FFTW3_VERSION})

# find the path to the header based on pkg config information
find_path(FFTW3_INCLUDE_DIR
    NAMES fftw3.h
    PATHS ${PC_FFTW3_INCLUDE_DIRS}
    PATH_SUFFIXES fftw
)

# path to the library
find_library(FFTW3_LIBRARY
    NAMES fftw3 ${PC_FFTW3_LIBRARIES}
    PATHS ${PC_FFTW3_LIBRARY_DIRS}
    PATH_SUFFIXES fftw
)

# tell CMake that these variables are consumed
mark_as_advanced(FFTW3_FOUND FFTW3_INCLUDE_DIR FFTW3_LIBRARY FFTW3_VERSION)

# set "FFTW3_FOUND" if the specified variables have values
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    FFTW3
    REQUIRED_VARS FFTW3_INCLUDE_DIR FFTW3_LIBRARY
    VERSION_VAR FFTW3_VERSION
)

# most important part: actually define the imported target
if(FFTW3_FOUND AND NOT TARGET FFTW3::fftw3)
    # add the target
    add_library(FFTW3::fftw3 MODULE IMPORTED)
    # add the library location and include directories
    set_target_properties(FFTW3::fftw3
        PROPERTIES
            IMPORTED_LOCATION ${FFTW3_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${FFTW3_INCLUDE_DIR}
    )
endif()

# export variables mentioned above if everything checks out
if(FFTW3_FOUND)
    set(FFTW3_INCLUDE_DIRS ${FFTW3_INCLUDE_DIR})
    set(FFTW3_LIBRARIES ${FFTW3_LIBRARY})
endif()
