# Define a global target for model tests
add_custom_target(build_model_tests)

# Utopia-specific wrapper for adding a model test
#
# .. cmake_function:: add_model_test
#
#   MODEL_NAME should specify the name of the model
#
#   The target is registered as a test and the required libraries are linked.
#
function(add_model_test)
    
    # Parse arguments
    include(CMakeParseArguments)
    set(OPTIONS)
    set(SINGLEARGS MODEL_NAME NAME)
    set(MULTIARGS SOURCES)
    cmake_parse_arguments(KW
        "${OPTIONS}" "${SINGLEARGS}" "${MULTIARGS}" ${ARGN})

    if (KW_UNPARSED_ARGUMENTS)
        message(WARNING "Encountered unparsed arguments in 'add_model_test': "
                        "${KW_UNPARSED_ARGUMENTS}")
    endif ()

    # Create the target name for the bundled model tests
    set(bundled_model_tests test_model_${KW_MODEL_NAME})

    # Add custom target for this model prefix, if it does not already exist
    if (NOT TARGET ${bundled_model_tests})
        add_custom_target(${bundled_model_tests}
            COMMAND ctest --output-on-failure
                          --tests-regex ^MODEL_${KW_MODEL_NAME}_.+$
        )
        message(STATUS "Model tests target:          ${bundled_model_tests}")

        # register it with the model tests
        add_dependencies(test_models ${bundled_model_tests})
    endif ()

    # Create the target name
    set(target_name MODEL_${KW_MODEL_NAME}_${KW_NAME})

    # Add the test
    add_executable(${target_name} ${KW_SOURCES})
    add_test(NAME ${target_name} COMMAND ${target_name})
    add_dependencies(build_model_tests ${target_name})

    # link to Utopia target
    target_link_libraries(${target_name} PUBLIC utopia)

    # Done. Inform about it
    message(STATUS "Added model test target:     ${target_name}")

endfunction()


# Utopia-specific wrapper for adding a list of model tests
function(add_model_tests)
    # Parse arguments
    include(CMakeParseArguments)
    set(OPTIONS)
    set(SINGLEARGS MODEL_NAME)
    set(MULTIARGS SOURCES AUX_FILES LINK_LIBRARIES)
    cmake_parse_arguments(KW
        "${OPTIONS}" "${SINGLEARGS}" "${MULTIARGS}" ${ARGN})

    if (KW_UNPARSED_ARGUMENTS)
        message(WARNING "Encountered unparsed arguments in 'add_model_tests': "
                        "${KW_UNPARSED_ARGUMENTS}")
    endif ()

    # Go over the list of sources and add them as test targets
    if (NOT KW_SOURCES)
        message(WARNING "No test sources given for model ${KW_MODEL_NAME}!")
        return()
    endif()

    foreach(src ${KW_SOURCES})
        get_filename_component(test_name ${src} NAME_WE)
        add_model_test(MODEL_NAME ${KW_MODEL_NAME}
                       NAME ${test_name}
                       SOURCES ${src})

        if (KW_LINK_LIBRARIES)
            target_link_libraries(MODEL_${KW_MODEL_NAME}_${test_name}
                                  PUBLIC ${KW_LINK_LIBRARIES})
        endif()
    endforeach(src)

    # Copy auxilary files
    if (KW_AUX_FILES)
        file(COPY ${KW_AUX_FILES}
             DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endfunction()
