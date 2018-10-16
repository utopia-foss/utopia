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
    set(MULTIARGS)
    cmake_parse_arguments(KWARGS "${OPTIONS}" "${SINGLEARGS}" "${MULTIARGS}" ${ARGN})

    # Create the target name
    set(target_name MODEL_${KWARGS_MODEL_NAME}_${KWARGS_NAME})

    # Add the test
    dune_add_test(NAME ${target_name} ${KWARGS_UNPARSED_ARGUMENTS})

    # Link the required library targets
    target_link_libraries(${target_name}
        spdlog
        yaml-cpp
    )

    # yaml-cpp does not export interface include dirs
    # NOTE This should be reported as bug or patched via MR in the yaml-cpp
    #      repository. See https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
    #      as a reference on how to do that.
    target_include_directories(${target_name}
        # register as system headers (compilers might ignore warnings)
        SYSTEM
        PRIVATE
            ${PROJECT_SOURCE_DIR}/vendor/plugins/yaml-cpp/include/
    )

    # Done. Inform about it
    message(STATUS "Added model test target:     ${target_name}")

endfunction()
