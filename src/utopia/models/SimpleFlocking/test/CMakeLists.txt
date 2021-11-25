add_model_tests(
    MODEL_NAME SimpleFlocking
    SOURCES
        "test_movement.cc"
        "test_state.cc"
    AUX_FILES
        "test_movement.yml"
        "test_state.yml"
)
