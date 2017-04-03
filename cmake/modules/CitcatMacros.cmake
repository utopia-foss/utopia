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