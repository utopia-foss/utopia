# BOOST
find_package(Boost 1.62 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND DUNE_LIBS ${Boost_LIBRARIES})

# HDF5
find_package(HDF5 1.10 REQUIRED COMPONENTS C HL) 
include_directories(${HDF5_INCLUDE_DIRS})
list(APPEND DUNE_LIBS ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})

# FFTW3
find_package(FFTW3 3.3 REQUIRED)

# Armadillo
# use the config if possible
find_package(Armadillo CONFIG)
# fall back to our module if no target is defined
if (NOT TARGET armadillo)
    find_package(Armadillo MODULE REQUIRED)
endif()

# include Utopia macros
include(UtopiaEnv)
include(UtopiaAddModel)
include(UtopiaAddModelTest)
include(UtopiaGenerateModelInfo)
include(UtopiaPython)
