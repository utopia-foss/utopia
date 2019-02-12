# Utopia-specific wrapper for add_executable
#
# .. cmake_function:: add_model
#
#   This function is invoked like 'add_executable'.
#   
#   The first argument is required and states the name of the target.
#   Additional arguments add sources to the target.
#
#   The target is registered as Utopia Model and its name and path (relative
#   to the ) are
#   appended to the Cache for relaying them to the utopya Python module.
#   Additionally, the source directory is also appended to a cached list.
#
function(add_model target_name)
    # register regularly
    add_executable(${target_name} ${ARGN})

    # link library targets
    target_link_libraries(${target_name}
        spdlog
        yaml-cpp
        armadillo
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

    # the rest of this function is to relay information to the utopya package
    # add name to target list
    set(UTOPIA_MODEL_TARGETS ${UTOPIA_MODEL_TARGETS} ${target_name}
        CACHE STRING "list of Utopia model targets" FORCE)

    # find the path relative to the source directory
    file(RELATIVE_PATH rel_src_path
         ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR})
    
    # add relative path to source directory of this model to the cache
    set(UTOPIA_MODEL_SRC_DIRS ${UTOPIA_MODEL_SRC_DIRS} ${rel_src_path}
        CACHE STRING "list of relative Utopia model source directories" FORCE)

    # add target binpath to cached list
    # can use rel_src_path because the tree is mirrored in the build output
    set(UTOPIA_MODEL_BINPATHS
        ${UTOPIA_MODEL_BINPATHS} ${rel_src_path}/${target_name}
        CACHE STRING "list of relative Utopia model binpaths" FORCE)

    # information message about the model having been added
    message(STATUS "Registered model target:   ${target_name}")
endfunction()

# clear cache variables before configuration
set(UTOPIA_MODEL_TARGETS
    CACHE STRING "list of Utopia model targets" FORCE)
set(UTOPIA_MODEL_BINPATHS
    CACHE STRING "list of relative Utopia model binpaths" FORCE)
set(UTOPIA_MODEL_SRC_DIRS
    CACHE STRING "list of relative Utopia model source directories" FORCE)
