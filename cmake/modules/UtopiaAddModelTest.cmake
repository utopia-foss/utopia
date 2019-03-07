# Define a global target for model tests
add_custom_target(build_model_tests)

# Utopia-specific wrapper for adding a model test
#
# .. cmake_function:: add_model_test
#
#   MODEL_NAME should specify the 
#
#   The target is registered as a test, the required libraries are linked,
#   and it is assured that the 
#
function(add_model_test)
    
    # Parse arguments
    include(CMakeParseArguments)
    set(OPTIONS)
    set(SINGLEARGS MODEL_NAME NAME)
    set(MULTIARGS SOURCES)
    cmake_parse_arguments(KWARGS
        "${OPTIONS}" "${SINGLEARGS}" "${MULTIARGS}" ${ARGN})

    if (KWARGS_UNPARSED_ARGUMENTS)
        message(WARNING "Encountered unparsed arguments in 'add_model_test': "
                        "${KWARGS_UNPARSED_ARGUMENTS}")
    endif ()

    # Create the target name
    set(target_name MODEL_${KWARGS_MODEL_NAME}_${KWARGS_NAME})

    # Add the test
    add_executable(${target_name} ${KWARGS_SOURCES})
    add_test(NAME ${target_name} COMMAND ${target_name})
    add_dependencies(build_model_tests ${target_name})

    # link to Utopia target
    target_link_libraries(${target_name} PUBLIC utopia)

    # Done. Inform about it
    message(STATUS "Added model test target:     ${target_name}")

endfunction()
