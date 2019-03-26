"""Test the command line methods"""

import pytest

import utopya.cltools as clt

# Other tests -----------------------------------------------------------------

def test_add_entry():
    """Tests the add_entry method"""
    # Basic case
    d = dict()
    clt.add_entry(123, add_to=d, key_path=["foo", "bar"])
    assert d['foo']['bar'] == 123

    # With function call
    d = dict()
    clt.add_entry(123, add_to=d, key_path=["foo", "bar"],
                  value_func=lambda v: v**2)
    assert d['foo']['bar'] == 123**2

    # Invalid value
    with pytest.raises(ValueError, match="My custom error message with -123"):
        clt.add_entry(-123, add_to=d, key_path=["foo", "bar"],
                      is_valid=lambda v: v>0,
                      ErrorMsg=lambda v: ValueError("My custom error message "
                                                    "with {}".format(v)))


def test_add_from_kv_pairs():
    """Tests the add_from_kv_pairs method"""
    d = dict()

    clt.add_from_kv_pairs("foo=bar",
                          "bar.foo=baz",
                          "an_int=123",
                          "a_float=1.23",
                          "a_bool=true",
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

