"""Test the command line methods"""

import pytest

import utopya.cltools as clt
import utopya.model_registry as mr
from utopya.yaml import write_yml

# Fixtures --------------------------------------------------------------------

from .test_cfg import tmp_cfg_dir
from .test_model_registry import tmp_model_registry

# -----------------------------------------------------------------------------

def test_add_from_kv_pairs():
    """Tests the add_from_kv_pairs method"""
    d = dict()

    clt.add_from_kv_pairs("foo=bar",
                          "bar.foo=baz",
                          "an_int=123",
                          "a_float=1.23",
                          "a_bool=true",
                          "None=null",
                          "long_string=long string with spaces",
                          "not_an_int=- 10",
                          "another_float=-1.E10",
                          "failing_float=.E10",
                          add_to=d)

    assert d["foo"] == "bar"
    assert d["bar"]["foo"] == "baz"

    assert d["an_int"] == 123
    assert isinstance(d["an_int"], int)

    assert d["a_float"] == 1.23
    assert isinstance(d["a_float"], float)
    
    assert d["a_bool"] is True
    
    assert d["None"] is None

    assert d["long_string"] == "long string with spaces"

    assert d["not_an_int"] == "- 10"
    assert isinstance(d["not_an_int"], str)

    assert d["another_float"] == float("-1.E10")
    assert isinstance(d["another_float"], float)

    assert d["failing_float"] == ".E10"
    assert isinstance(d["failing_float"], str)


    # With eval allowed
    clt.add_from_kv_pairs("squared=2**4",
                          "list=[1, 2, 3]",
                          "bad_syntax=foo",
                          add_to=d, allow_eval=True)

    assert d["squared"] == 16
    assert d["list"] == [1, 2, 3]
    assert d["bad_syntax"] == "foo"

    # Deletion
    clt.add_from_kv_pairs("squared=DELETE", add_to=d)
    assert 'squared' not in d
    
    assert 'i_do_not_exist' not in d
    clt.add_from_kv_pairs("i_do_not_exist=DELETE", add_to=d)
    assert 'i_do_not_exist' not in d

    with pytest.raises(ValueError, match="Attempted deletion"):
        clt.add_from_kv_pairs("list=DELETE", add_to=d, allow_deletion=False)


def test_deploy_user_cfg(tmpdir, monkeypatch, capsys):
    """Tests whether user configuration distribution works as desired."""
    # Create a path for the user config test file; needs to be str-cast to
    # allow python < 3.6 implementation of os.path
    test_path = tmpdir.join("test_user_cfg.yml")

    # There should not be a file at the test path yet
    assert not test_path.isfile()

    # Execute the deploy function with this path and assert it worked
    clt.deploy_user_cfg(user_cfg_path=str(test_path))
    assert test_path.isfile()

    # Check the terminal output
    out, _ = capsys.readouterr()
    assert out.startswith("Deployed user config to: {}\n\nAll entries are"
                          "".format(test_path))

    # monkeypatch the "input" function, so that it returns "y" or "no".
    # This simulates the user entering something in the terminal.
    # Also, check that the question was asked and the right branch was taken.
    
    # yes-case
    monkeypatch.setattr('builtins.input', lambda x: "y")
    clt.deploy_user_cfg(user_cfg_path=str(test_path))
    
    out, _ = capsys.readouterr()
    assert out.find("A config file already exists at") >= 0
    assert out.find("Deployed user config") >= 0

    # no-case
    monkeypatch.setattr('builtins.input', lambda x: "n")
    clt.deploy_user_cfg(user_cfg_path=str(test_path))
    
    out, _ = capsys.readouterr()
    assert out.find("A config file already exists at") >= 0
    assert out.find("Not deploying user config.") >= 0


def test_register_models(tmp_model_registry, tmpdir):
    """"""
    # An attribute dict to use to mock the object that is returned after
    # command line args are parsed: an attribute-access dict
    class MockArgs(dict):
        def __init__(self, *args, **kwargs):
            super(MockArgs, self).__init__(*args, **kwargs)
            self.__dict__ = self

    # Create some testing arguments
    args = MockArgs()
    args.separator = ";"
    args.model_name = "foo;bar"
    args.bin_path = "bin/foo;bin/bar"
    args.src_dir = "src/foo;src/bar"
    args.base_src_dir = tmpdir.join("base_src")
    args.base_bin_dir = tmpdir.join("base_bin")
    args.remove_existing = False
    args.skip_existing = False
    args.label = None

    # Try registration ... will fail, because files are missing in this test
    with pytest.raises(KeyError, match="Missing required key: default_cfg"):
        clt.register_models(args, registry=tmp_model_registry)
    # NOTE It's not crucial to test a full registration here, because that's
    #      done in the test there. We just want to ascertain that the cltools
    #      function does what is expected of it

    # List mismatch
    args.src_dir += ";src/spam"
    with pytest.raises(ValueError, match="Mismatch of sequence lengths"):
        clt.register_models(args, registry=tmp_model_registry)
