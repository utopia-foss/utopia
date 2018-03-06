
import pytest
from utopya import Multiverse


def test_init():
    """Tests the initialization of the Multiverse."""
    Multiverse()  # fails, if neither default nor metaconfig are present
    Multiverse(metaconfig="metaconfig.yml")
    Multiverse(metaconfig="metaconfig.yml", userconfig="userconfig.yml")

    # Testing errors
    with pytest.raises(FileNotFoundError):
        Multiverse(metaconfig="not_existing_metaconfig.yml")

    with pytest.raises(FileNotFoundError):
        Multiverse(userconfig="not_existing_userconfig.yml")
