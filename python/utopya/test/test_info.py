import pytest
import utopya.info as info

def test_model_info():
    """Tests whether the model information is available"""
    print("Got model info:")
    print(info.MODELS)

    assert isinstance(info.MODELS, dict)
    for model_name, model_info in info.MODELS.items():
        print("Model name:", model_name)
        assert 'binpath' in model_info
    