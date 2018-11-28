# CMake script to find the native FFTW includes and library
#
#  Variables being set:
#    - FFTW_FOUND       True if FFTW was found
#    - FFTW_INCLUDES    Location of fftw3.h
#    - FFTW_LIBRARIES   List of libraries when using FFTW

# Check if it is already in cache
if (FFTW_INCLUDES)
    # Yep. Reduce verbosity
    set(FFTW_FIND_QUIETLY TRUE)
endif (FFTW_INCLUDES)

# Find the header file and the library
find_path(FFTW_INCLUDES fftw3.h)
find_library(FFTW_LIBRARIES NAMES fftw3)

# Handle arguments 'QUIETLY' and 'REQUIRED'.
# This also sets FFTW_FOUND to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW DEFAULT_MSG
                                  FFTW_LIBRARIES
                                  FFTW_INCLUDES)

mark_as_advanced(FFTW_LIBRARIES FFTW_INCLUDES)

