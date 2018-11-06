"""Test the info module, which is created by cmake"""

import pytest

def test_model_info():
    """Tests whether the model information is available"""
    import utopya.info as info
    
    print("Got model info:")
    print(info.MODELS)

    assert isinstance(info.MODELS, dict)
    for model_name, model_info in info.MODELS.items():
        print("Model name:", model_name)
        assert 'binpath' in model_info
        assert 'src_dir' in model_info

    # Remove content and assert it still does the correct thing
    info._UTOPIA_MODEL_TARGETS = ""
    assert info.parse_model_info() == {}
    
    # Inconsistent targets and path lists
    info._UTOPIA_MODEL_TARGETS = "dummy"
    with pytest.raises(ValueError, match="need to be of the same length"):
        info.parse_model_info()

    # Missing required model config file
    info._UTOPIA_MODEL_BINPATHS = "dune/utopia/models/dummy/dummy"
    info._UTOPIA_MODEL_SRC_DIRS = "dune/utopia/models/dummy_foooo"

    with pytest.raises(FileNotFoundError, match="For target 'dummy', could n"):
        info.parse_model_info()
