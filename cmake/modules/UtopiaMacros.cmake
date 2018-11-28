# BOOST
find_package(Boost 1.62 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND DUNE_LIBS ${Boost_LIBRARIES})

# HDF5
find_package(HDF5 1.10 REQUIRED COMPONENTS C HL) 
include_directories(${HDF5_INCLUDE_DIRS})
list(APPEND DUNE_LIBS ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})

# FFTW
find_package(FFTW 3.3 REQUIRED) 
include_directories(${FFTW_INCLUDE_DIRS})
list(APPEND DUNE_LIBS ${FFTW_LIBRARIES})


# include Utopia macros
include(PythonInstallPackageRemote)
include(UtopiaAddModel)
include(UtopiaAddModelTest)
include(UtopiaGenerateModelInfo)
