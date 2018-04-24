# Utopia-specific wrapper for add_executable
#
# .. cmake_function:: add_model
#
#   This function is invoked like 'add_executable'.
#   
#   The first argument is required and states the name of the target.
#   Additional arguments add sources to the target.
#
#   The target is registered as Utopia Model and its name and path are
#   appended to the Cache for relaying them to the utopya Python module.
#
function(add_model target_name)
    # register regularly
    add_executable(${target_name} ${ARGN})

    # add name to target list
    set(UTOPIA_MODEL_TARGETS ${UTOPIA_MODEL_TARGETS} ${target_name}
        CACHE STRING "list of Utopia model targets" FORCE)
    
    # get path to binary
    file(RELATIVE_PATH relative_binary_path
        ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR})
    set(target_binary_path 
        ${PROJECT_BINARY_DIR}/${relative_binary_path}/${target_name})
    
    # add directory to list
    set(UTOPIA_MODEL_BINPATHS ${UTOPIA_MODEL_BINPATHS} ${target_binary_path}
        CACHE STRING "list of Utopia model binpaths" FORCE)

    # information message about the model having been added
    message(STATUS "Registered model target:   ${target_name}")
endfunction()

# clear cache variables before configuration
set(UTOPIA_MODEL_TARGETS CACHE STRING "list of Utopia model targets" FORCE)
set(UTOPIA_MODEL_BINPATHS CACHE STRING "list of Utopia model binpaths" FORCE)
