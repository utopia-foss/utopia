# Instructions for adding a model to Utopia:
#   1. Create a subfolder with the name of your model, <name>
#   2. Add the corresponding `add_subdirectory(<name>)` below
#   3. Add a CMakeLists.txt in the model directory
#   4. Call `add_model(<name> <name>.cc)` there, where <name> is the name of
#      the model directory. For consistency and identification of the models,
#      it is important that the names of folders and targets match.
#   5. Re-configure using `cmake ..`

# Declare this directory as containing all models of this project
set(UTOPIA_MODELS_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE STRING "the directory containing all model source directories" FORCE)

message(STATUS "")
message(STATUS "Starting Utopia model registration ...")

# Register the model directories
add_subdirectory(ContDisease)
add_subdirectory(CopyMeGrid)
add_subdirectory(CopyMeBare)
add_subdirectory(CopyMeGraph)
add_subdirectory(dummy)
add_subdirectory(Environment)
add_subdirectory(ForestFire)
add_subdirectory(GameOfLife)
add_subdirectory(Geomorphology)
add_subdirectory(HdfBench)
add_subdirectory(Opinionet)
add_subdirectory(PredatorPrey)
add_subdirectory(PredatorPreyPlant)
add_subdirectory(SandPile)
add_subdirectory(SEIRD)
add_subdirectory(SimpleEG)
add_subdirectory(SimpleFlocking)
add_subdirectory(Vegetation)
# ...
# NOTE Add your subdirectory above (alphabetically sorted)

message(STATUS "Model registration finished.")
