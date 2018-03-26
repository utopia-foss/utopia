# Instructions for adding a model to Utopia:
#   1. Create a subfolder with the name of your model
#   2. Add the model directory name in the list below
#   3. Add a CMakeLists.txt in the model directory
#   4. Call `add_model(<name> <name>.cc)` there, where <name> is the name of
#      the model directory. For consistency and identification of the models,
#      it is important that the names of folders and targets match.
#   5. Re-configure using `cmake ..`

# List of model directories:
set(MODEL_DIRS
    dummy
    # ... add other model folders here
    )


# -----------------------------------------------------------------------------
# NOTE Do not change below this line ------------------------------------------

# Register the model directories
foreach(model_dir ${MODEL_DIRS})
    add_subdirectory(${model_dir})
    message(STATUS "Added Utopia model directory:  ${model_dir}")
endforeach(model_dir)
