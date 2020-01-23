# CMake script to find the Armadillo includes and library
#
# This will import the targets
#
#   armadillo                   Library to link dependent objects against
#
# and set the following variables
#
#   - Armadillo_FOUND           True if Armadillo was found
#   - Armadillo_INCLUDE_DIRS    Location of main Armadillo header
#   - Armadillo_LIBRARIES       Location of Armadillo library
#
# It uses hints provided by the CMake or environment variables
#
#   - Armadillo_ROOT            Path to the Armadillo install directory
#

# find the library itself
find_library(Armadillo_LIBRARY
    NAMES armadillo
    PATH_SUFFIXES lib
)

# find the header
find_path(Armadillo_INCLUDE_DIR
    armadillo
    PATH_SUFFIXES include
)

# report if package was found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Armadillo
    DEFAULT_MSG
    Armadillo_LIBRARY Armadillo_INCLUDE_DIR
)

mark_as_advanced(Armadillo_LIBRARY Armadillo_INCLUDE_DIR)

# set output variables
if (Armadillo_FOUND)
    set(Armadillo_LIBRARIES ${Armadillo_LIBRARY})
    set(Armadillo_INCLUDE_DIRS ${Armadillo_INCLUDE_DIR})

    # add the library target
    add_library(armadillo SHARED IMPORTED GLOBAL)
    set_target_properties(armadillo
        PROPERTIES IMPORTED_LOCATION ${Armadillo_LIBRARIES}
                   INTERFACE_INCLUDE_DIRECTORIES ${Armadillo_INCLUDE_DIRS}
    )
endif()
