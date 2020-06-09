# Add model tests.
# They will be available as a custom target:  test_model_${MODEL_NAME}

add_model_tests(# Use consistent capitalization for the model name!
                MODEL_NAME SEIRD
                # The sources of the model tests to carry out. Each of these
                # will become a test target with the same name.
                SOURCES
                    "movement_test.cc"
                # Optional: Files to be copied to the build directory
                # AUX_FILES
                )
