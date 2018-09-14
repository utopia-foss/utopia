# Add the model target
add_model(AmeeMulti AmeeMulti.cc)
add_subdirectory(test EXCLUDE_FROM_ALL)

# NOTE The target should have the same name as the model folder and the *.cc
