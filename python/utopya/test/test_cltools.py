"""Test the command line methods"""

import pytest

import utopya.cltools as clt
import utopya.model_registry as mr
from utopya.yaml import write_yml
from utopya.cfg import load_from_cfg_dir

# Fixtures --------------------------------------------------------------------

from .test_cfg import tmp_cfg_dir
from .test_model_registry import tmp_model_registry


class MockArgs(dict):
    """An attribute dict to mock the CL arguments"""
    def __init__(self, *args, **kwargs):
        super(MockArgs, self).__init__(*args, **kwargs)
        self.__dict__ = self

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


def test_register_models(tmp_model_registry, tmp_cfg_dir, tmpdir):
    """Test the register_models command line helper function"""
    # Create some testing arguments
    args = MockArgs()
    args.separator = ";"
    args.model_name = "foo;bar"
    args.bin_path = "bin/foo;bin/bar"
    args.src_dir = "src/foo;src/bar"
    args.base_src_dir = tmpdir.join("base_src")
    args.base_bin_dir = tmpdir.join("base_bin")
    args.exists_action = None
    args.label = None
    args.overwrite_label = False
    args.project_name = "ProjectName"
    args.update_project_info = False

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

    # Test again with invocation of the register_project function
    # Changes are made only to tmp_cfg_dir and the fixture returns them to the
    # old state after the test.
    # But this test is only meant to test the *invocation* of register_project,
    # the actual test happens in test_register_project
    args.update_project_info = True
    with pytest.raises(AttributeError,
                       match="has no attribute 'project_base_dir'"):
        clt.register_models(args, registry=tmp_model_registry)


def test_register_project(tmp_cfg_dir, tmpdir):
    """Test the register_project command line helper function"""
    args = MockArgs()
    args.name = "ProjectName"
    args.base_dir = tmpdir.join("base")
    args.models_dir = tmpdir.join("base/src/models")
    args.python_model_tests_dir = tmpdir.join("base/python/model_tests")
    args.python_model_plots_dir = tmpdir.join("base/python/model_plots")

    # Before invocation, the (temporary) cfg dir has no projects defined and
    # the plot module paths are not adjusted
    assert not load_from_cfg_dir('projects')
    assert not load_from_cfg_dir('plot_module_paths')

    # Invoke it and test that it is written
    project = clt.register_project(args)
    print(project)
    assert project == load_from_cfg_dir('projects')['ProjectName']

    # The python model plots should also have changed
    plot_module_paths = load_from_cfg_dir('plot_module_paths')
    assert "ProjectName" in plot_module_paths
    assert plot_module_paths["ProjectName"] == args.python_model_plots_dir


    # If invoking it without model plots, there should not be another one
    args.name = "AnotherProject"
    args.python_model_plots_dir = None

    project = clt.register_project(args)
    print(project)
    assert load_from_cfg_dir('projects')['AnotherProject']

    assert plot_module_paths == load_from_cfg_dir('plot_module_paths')


def test_copy_model_files(capsys, monkeypatch):
    """This tests the copy_model_files CLI helper function. It only tests
    the dry_run because mocking the write functions would be too difficult
    here. The actual copying is tested
    """
    # First, without prompts
    copy_model_files = lambda **kws: clt.copy_model_files(**kws,
                                                          use_prompts=False,
                                                          skip_exts=['.pyc'],
                                                          dry_run=True)

    # This should work
    copy_model_files(model_name="CopyMeBare",
                     new_name="FooBar", target_project="Utopia")

    # Make sure that some content is found in the output; this is a proxy for
    # the actual behaviour...
    out, _ = capsys.readouterr()
    print(out, "\n"+"#"*79)

    # Path changes
    assert 0 < out.find("CopyMeBare.cc") < out.find("FooBar.cc")
    assert 0 < out.find("CopyMeBare/CopyMeBare.cc") < out.find("FooBar/FooBar.cc")

    # Added the add_subdirectory command at the correct position
    assert (0 < out.find("add_subdirectory(dummy)")
              < out.find("add_subdirectory(FooBar)")
              < out.find("add_subdirectory(HdfBench)"))


    # Without adding to CMakeLists.txt ...
    copy_model_files(model_name="CopyMeBare", add_to_cmakelists=False,
                     new_name="FooBar2", target_project="Utopia")
    out, _ = capsys.readouterr()
    print(out, "\n"+"#"*79)
    assert not (0 < out.find("add_subdirectory(FooBar2)"))
    assert 0 < out.find("Remember to register the new model in the ")


    # These should not work due to bad model or project names
    with pytest.raises(ValueError, match="'dummy' is already registered!"):
        copy_model_files(model_name="CopyMeBare",
                        new_name="dummy", target_project="Utopia")
    _ = capsys.readouterr()
    
    with pytest.raises(ValueError, match="No Utopia project with name 'NoSu"):
        copy_model_files(model_name="CopyMeBare",
                        new_name="FooBar", target_project="NoSuchProject")
    _ = capsys.readouterr()


    # These should not work, because use_prompts == False
    with pytest.raises(ValueError, match="Missing new_name argument!"):
        copy_model_files(model_name="CopyMeBare")
    _ = capsys.readouterr()
    
    with pytest.raises(ValueError, match="Missing target_project argument!"):
        copy_model_files(model_name="CopyMeBare", new_name="FooBar")
    _ = capsys.readouterr()


    # Now, do it again, mocking some of the prompts
    copy_model_files = lambda **kws: clt.copy_model_files(**kws, dry_run=True)

    monkeypatch.setattr('builtins.input', lambda x: "MyNewModel")
    copy_model_files(model_name="CopyMeBare", target_project="Utopia")
    out, _ = capsys.readouterr()
    print(out, "\n"+"#"*79)
    assert 0 < out.find("Name of the new model:      MyNewModel")

    monkeypatch.setattr('builtins.input', lambda x: "Utopia")
    copy_model_files(model_name="CopyMeBare", new_name="FooBar")
    out, _ = capsys.readouterr()
    print(out, "\n"+"#"*79)
    assert 0 < out.find("Utopia project to copy to:  Utopia")

    monkeypatch.setattr('builtins.input', lambda x: "N")
    copy_model_files(model_name="CopyMeBare",
                     new_name="FooBar", target_project="Utopia")
    out, _ = capsys.readouterr()
    print(out, "\n"+"#"*79)
    assert 0 < out.find("Not proceeding")

    def raise_KeyboardInterrupt(*_):
        raise KeyboardInterrupt()

    monkeypatch.setattr('builtins.input', raise_KeyboardInterrupt)
    copy_model_files(model_name="CopyMeBare", target_project="Utopia")
    out, _ = capsys.readouterr()
    print(out, "\n"+"#"*79)

    monkeypatch.setattr('builtins.input', raise_KeyboardInterrupt)
    copy_model_files(model_name="CopyMeBare", new_name="FooBar")
    out, _ = capsys.readouterr()
    print(out, "\n"+"#"*79)

    monkeypatch.setattr('builtins.input', raise_KeyboardInterrupt)
    copy_model_files(model_name="CopyMeBare",
                     new_name="FooBar", target_project="Utopia")
    out, _ = capsys.readouterr()
    print(out, "\n"+"#"*79)
