# Set the model folders here
set(MODEL_DIRS
    dummy
    # ... add other model folders here
    )

# NOTE Do not change below this line ------------------------------------------
# -----------------------------------------------------------------------------

# Add the models as targets
foreach(model_dir ${MODEL_DIRS})
    add_subdirectory(${model_dir})
    message(STATUS "Added Utopia model directory:  ${model_dir}")
endforeach(model_dir)
