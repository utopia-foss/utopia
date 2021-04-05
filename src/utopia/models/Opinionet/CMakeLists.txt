# Add the model target
add_model(Opinionet Opinionet.cc)
# NOTE The target should have the same name as the model folder and the *.cc
# Add test directories
add_subdirectory(test EXCLUDE_FROM_ALL)