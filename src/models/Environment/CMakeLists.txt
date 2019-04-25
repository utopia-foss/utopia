# Add the model target
add_model(Environment Environment.cc)

# Add test directories
add_subdirectory(test EXCLUDE_FROM_ALL)
