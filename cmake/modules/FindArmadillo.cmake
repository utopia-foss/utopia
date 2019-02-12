# CMake script to find the Armadillo includes and library
#
# This will import the targets
#
#   armadillo                   Library to link dependent objects against
#
# and set the following variables
#
#   - ARMADILLO_FOUND           True if Armadillo was found
#   - ARMADILLO_INCLUDE_DIRS    Location of main Armadillo header
#   - ARMADILLO_LIBRARIES       Location of Armadillo library
#
# It uses hints provided by the CMake or environment variables
#
#   - ARMADILLO_ROOT            Path to the Armadillo install directory
#

# find the library itself
find_library(ARMADILLO_LIBRARY
    NAMES armadillo

    # NOTE: Use the hinting variable explicitly.
    # This is not required for more recent versions of CMake,
    # where policy CMP0074 was introduced,
    # see https://cmake.org/cmake/help/latest/command/find_library.html
    HINTS ${ARMADILLO_ROOT} ENV ARMADILLO_ROOT
)

# find the header
find_path(ARMADILLO_INCLUDE_DIR
    armadillo
    HINTS ${ARMADILLO_ROOT} ENV ARMADILLO_ROOT
)

mark_as_advanced(ARMADILLO_LIBRARY ARMADILLO_INCLUDE_DIR)

# report if package was found
find_package_handle_standard_args(ARMADILLO
  DEFAULT_MSG
  ARMADILLO_LIBRARY ARMADILLO_INCLUDE_DIR
)

# set output variables
if (ARMADILLO_FOUND)
    set(ARMADILLO_LIBRARIES ${ARMADILLO_LIBRARY})
    set(ARMADILLO_INCLUDE_DIRS ${ARMADILLO_INCLUDE_DIR})

    # add the library target
    add_library(armadillo SHARED IMPORTED)
    set_target_properties(armadillo
      PROPERTIES IMPORTED_LOCATION ${ARMADILLO_LIBRARIES}
                 INTERACE_INCLUDE_DIRECTORIES ${ARMADILLO_INCLUDE_DIRS}
    )
endif()
