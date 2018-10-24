# Instructions for adding a model to Utopia:
#   1. Create a subfolder with the name of your model, <name>
#   2. Add the corresponding `add_subdirectory(<name>)` below
#   3. Add a CMakeLists.txt in the model directory
#   4. Call `add_model(<name> <name>.cc)` there, where <name> is the name of
#      the model directory. For consistency and identification of the models,
#      it is important that the names of folders and targets match.
#   5. Re-configure using `cmake ..`

message(STATUS "Starting Utopia model registration ...")

# Register the model directories
add_subdirectory(ContDisease)
add_subdirectory(CopyMe)
add_subdirectory(CopyMeBare)
add_subdirectory(dummy)
add_subdirectory(ForestFire)
add_subdirectory(Hierarnet)
add_subdirectory(SimpleEG)
# ...
# NOTE add your subdirectory above (alphabetically sorted)

message(STATUS "Model registration finished.")
