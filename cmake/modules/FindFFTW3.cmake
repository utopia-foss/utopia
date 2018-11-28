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
#   - FFTW3_LIBRARIES       List of FFTW libraries
#

# use pkg-config to determine module parameters
# and immediately import a target
find_package(PkgConfig)
pkg_check_modules(FFTW3
    QUIET REQUIRED IMPORTED_TARGET
    FFTW3)

# tell CMake that these variables are consumed
mark_as_advanced(FFTW3_FOUND FFTW3_VERSION)

# have CMake check that these variables are set
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    FFTW3
    REQUIRED_VARS FFTW3_LIBRARIES FFTW3_INCLUDE_DIRS
    VERSION_VAR FFTW3_VERSION
)

if(FFTW3_FOUND)
    # promote pkg config target to global scope
    set_target_properties(PkgConfig::FFTW3
        PROPERTIES
            IMPORTED_GLOBAL TRUE
    )

    # add an alias target
    add_library(FFTW3::fftw3 ALIAS PkgConfig::FFTW3)
endif()
