
import pytest
from utopya import Multiverse


def test_init():
    """Tests the initialsation of the Multiverse."""
    Multiverse()  # fails, if neither default nor metaconfig are present
    Multiverse(metaconfig="metaconfig.yml")
    Multiverse(metaconfig="metaconfig.yml", userconfig="userconfig.yml")

    # Testing errors
    with pytest.raises(FileNotFoundError):
        Multiverse(metaconfig="metaconfig2.yml")

    with pytest.raises(FileNotFoundError):
        Multiverse(userconfig="userconfig2.yml")
