# Define a target because the CMake module does not
add_library(hdf5 INTERFACE IMPORTED GLOBAL)

# Use variables from CMake find module to set target properties
target_include_directories(hdf5 INTERFACE ${HDF5_INCLUDE_DIRS})
target_link_libraries(hdf5 INTERFACE ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
target_compile_definitions(hdf5 INTERFACE ${HDF5_DEFINITIONS})
