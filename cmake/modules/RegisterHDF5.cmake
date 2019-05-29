# Define a target because the CMake module does not
add_library(hdf5 INTERFACE IMPORTED GLOBAL)

# TODO: Use `target_XXX` functions after raising CMake requirements
# Sanitize the definitions because we cannot use the proper CMake function...
string(REPLACE -D "" HDF5_C_DEFINITIONS "${HDF5_C_DEFINITIONS}")
# Set properties directly because of a bug in CMake 3.10
set_target_properties(hdf5 PROPERTIES
    INTERFACE_LINK_LIBRARIES "${HDF5_LIBRARIES};${HDF5_HL_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${HDF5_INCLUDE_DIRS}"
    INTERFACE_COMPILE_DEFINITIONS "${HDF5_C_DEFINITIONS}"
)
