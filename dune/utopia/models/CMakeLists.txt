# Set the models here
set(MODELS
    # ... add other models here
    )

# NOTE Do not change below this line ------------------------------------------
# -----------------------------------------------------------------------------

# Add the models as targets
foreach(model ${MODELS})
    add_executable(${model} ${model}.cc)
    message(STATUS "Added Utopia model executable:  ${model}")
endforeach(model)

# Get the list of targets from the directory property
get_property(model_targets
             DIRECTORY
             PROPERTY BUILDSYSTEM_TARGETS)

# Cache the value, overwriting the old one
set(UTOPIA_MODEL_TARGETS ${model_targets}
    CACHE STRING "list of Utopia model targets" FORCE)

# And fill it with the paths
foreach(model_target ${UTOPIA_MODEL_TARGETS})
    list(APPEND model_binpaths "${CMAKE_CURRENT_BINARY_DIR}/${model_target}")
endforeach(model_target)
message(STATUS "Added corresponding binary paths to ${CMAKE_CURRENT_BINARY_DIR}")

# Cache the value of the bin paths
set(UTOPIA_MODEL_BINPATHS ${model_binpaths}
    CACHE STRING "list of Utopia model binpaths" FORCE)

# NOTE The two written UTOPIA_* cache variables are read in by the CMakeLists in Python, generating the utopya/info.py module
