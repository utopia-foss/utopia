# BOOST
find_package(Boost 1.62 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND DUNE_LIBS ${Boost_LIBRARIES})

# HDF5
find_package(HDF5 1.10 REQUIRED COMPONENTS C HL) 
include_directories(${HDF5_INCLUDE_DIRS})
list(APPEND DUNE_LIBS ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})

# YAML-CPP
find_package(yaml-cpp 0.5.2 REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})
list(APPEND DUNE_LIBS ${YAML_CPP_LIBRARIES})

# SPDLOG
# Notice: Ubuntu Bionic includes 0.16.3 but its version string is 0.16.2
# I guess this is a bug of this particular version
find_package(spdlog 0.16.2 REQUIRED CONFIG)

# File for module specific CMake tests.
if(NOT PSGRAF_ROOT)
	set(PSGRAF_ROOT)
endif()
find_package(PSGraf CONFIG
	HINTS ${PSGRAF_ROOT})

if(PSGRAF_FOUND)
message(STATUS "Found PSGraf: ${PSGRAF_LIBRARIES}")
	include_directories(${PSGRAF_INCLUDE_DIRS})
	list(APPEND DUNE_LIBS ${PSGRAF_LIBRARIES})
	add_definitions(-DHAVE_PSGRAF)
endif()

# include Utopia macros
include(PythonInstallPackageRemote)
include(UtopiaAddModel)
include(UtopiaGenerateModelInfo)