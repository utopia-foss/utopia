# Add targets to build and execute model tests
add_custom_target(build_tests_models)
add_custom_target(test_models)
add_dependencies(test_models build_tests_models)

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

    # Create the target names for the single test executable, and those for
    # the bundled build and test targets for this model
    set(target MODEL_${KW_MODEL_NAME}_${KW_NAME})
    set(model_test_build_target build_tests_model_${KW_MODEL_NAME})
    set(model_test_target test_model_${KW_MODEL_NAME})

    # Add custom BUILD target for this model, if it does not already exist
    if (NOT TARGET ${model_test_build_target})
        add_custom_target(${model_test_build_target})

        # ... which is a dependency of the build target for _all_ models
        add_dependencies(build_tests_models ${model_test_build_target})
        # NOTE This global target needs to be set elsewhere, before the first
        #      invocation of this function.
    endif()

    # Add custom TEST target for this model, if it does not already exist
    if (NOT TARGET ${model_test_target})
        add_custom_target(${model_test_target}
            COMMAND ctest --output-on-failure
                          --tests-regex ^MODEL_${KW_MODEL_NAME}_.+$
        )
        message(STATUS "Model tests target:          ${model_test_target}")

        # To make sure tests are built, add the build target as a dependency
        add_dependencies(${model_test_target} ${model_test_build_target})

        # register it with the target to carry out _all_ model tests
        add_dependencies(test_models ${model_test_target})
    endif ()

    # Now, create the single test target and add it as a build dependency
    add_executable(${target} ${KW_SOURCES})
    add_dependencies(${model_test_build_target} ${target})

    # Add the executable as a test and as dependency of the bundled test target
    add_test(NAME ${target}
             COMMAND ${target} -r detailed)
    add_dependencies(${model_test_target} ${target})

    # link to Utopia target
    target_link_libraries(${target} PUBLIC utopia Boost::unit_test_framework)
    # NOTE: Might lead to issues if linked to the static unit test library
    target_compile_definitions(${target} PRIVATE BOOST_TEST_DYN_LINK)

    # Done. Inform about it
    message(STATUS "Added model test target:     ${target}")

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
