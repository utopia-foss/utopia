"""Test the command line methods"""

import pytest

import utopya.cltools as clt

# Other tests -----------------------------------------------------------------

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

