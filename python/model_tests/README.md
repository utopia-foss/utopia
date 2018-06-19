# Model Tests

This directory contains tests for all models implemented in Utopia.
These tests help to assure that the models behave as intended for certain pre-defined configurations, even if parts of Utopia or the models are changing.

They make use of the `pytest` framework to define and conduct tests and rely on `utopya` to configure and run the simulations.
Thus, the assertions implemented here are testing against the _output_ of the models, and do not allow unit tests of individual model functions.


## How to create tests for a new model

To make creation of tests for new models simpler, there is a template directory that can be copied and adjusted to your needs:

* Copy the `python/model_tests/CopyMe` directory to `python/model_tests/<model_name>`
* Replace all instances of `CopyMe` and `dummy` by your model name
* Adjust the tests to your needs

Here, "model name" refers to the _exact_ name of the target for your model.
Note the `TODO` comments throughout the copied files; make sure to follow those instructions.


## Basic structure of the model tests

The tests conducted here are all carried out using `utopya`, i.e. the frontend of Utopia.
This is the same interface that is also used when simulating interactively or when using the command line interface.

Below, you will find basic instructions on how to use the testing framework.


### The `ModelTests` class

This class helps in creating `Multiverse` instances to run simulations and create output with.
If given a path to the test file, it allows easy access to files relative to it.

An instance of this class is meant to be created in each test file and used by all tests. Example:

```python
from utopya.testtools import ModelTest

# Configure the ModelTest class for dummy
mtc = ModelTest("dummy", test_file=__file__)

def test_basics():
    """Test the most basic features of the model"""
    # Create a Multiverse from the shared ModelTest class
    mv = mtc.create_mv_from_cfg("basics.yml")

    # Run the simulation
    mv.run()

    # ...

def test_advanced_stuff():
    """Test advanced model behaviour"""
    # Create a Multiverse from the shared ModelTest class
    mv = mtc.create_mv_from_cfg("advanced.yml")

    # Run the simulation
    mv.run()

    # ...
```

Now, this will have created two `Multiverse` instances from different config files.
Note that you can also create more than one `Multiverse` within a single test; just take care that your tests are neither too long nor too short.

#### Handy commands
* `mtc.get_file_path(rel_path)` returns the path to a file relative to the directory the test file (`__file__`) is in
* You can also call the `mtc.create_mv` method to create a `Multiverse` using the `run_cfg_path`.
* You can pass `update_meta_cfg` arguments as keyword arguments to both `create_mv` and `create_mv_from_cfg`.
* By default, failing simulations raise an error. To adjust this, you can set the `sim_errors` key during initialization of `ModelTest`.

#### _Further reading:_ Internal workings
* Data output of the created `Multiverse` instances is written to a temporary directory
* The temporary directory is deleted only once the `ModelTest` class goes out of scope, i.e. when all tests of that test file have finished. If you write a very large amount of data, you can consider to instantiate a separate `ModelTest` instance which goes out of scope earlier to clear out the data.
* The `sim_errors` argument sets the `worker_manager.nonzero_exit_handling` key when a `Multiverse` is to be created. The value can be changed on a per-invocation level using `update_meta_cfg`.
* The only key that cannot be changed using `update_meta_cfg` is the `paths.out_dir` value, which is reserved for the temporary directory.


### The `Multiverse`
To run simulations:

```python
# Create Multiverse
mv = mtc.create_mv()

# Run a simulation
mv.run()
```

Alternatively, a sweep can be performed using the `run_sweep` command in conjunction with an appropriately configured `parameter_space` in the meta configuration ...

_Important:_ Per `Multiverse` instance, only a single simulation can and should be performed!
Do not issue multiple `run`, `run_single` or `run_sweep` calls!


### The `DataManager`
For loading and working with the output data, the `utopya.datamanager` module and its `DataManager` class are used, which is based on the [`dantro`](https://ts-gitlab.iup.uni-heidelberg.de/utopia/dantro) package.

An instance of that class is initialized already when the `Multiverse` is created and accessible via the `dm` property.
After a simulation run, the data can be loaded in the following way:

```python

def test_basics():
    """Test that the dummy model behaves as expected"""
    # Create a Multiverse from the shared ModelTest class
    mv = mtc.create_mv_from_cfg("basics.yml")

    # Run the simulation
    mv.run()

    # Load the data using the DataManager
    mv.dm.load_from_cfg(print_tree=True)
    # print_tree gives you info on the loaded data hierarchy

    # Data is now accessible via dict-like access to mv.dm
    assert mv.dm['universes']
```

The `load_from_cfg` command uses a pre-defined configuration that loads all data generated by the run.
With `print_tree` enabled, the loaded data is visualized in a tree after loading and shows you how to access each entry.

Access to a group or container inside this hierarchy happens via dict-like access, as shown above.
The binary containers are mapped to a data structure akin to `np.ndarray`; the underlying data can be accessed via the `data` attribute (however, try to work on the container-level as long as possible, as this will allow future convenience methods to work more easily).

_WIP: add more here_

### The `pytest` framework
For more info on the `pytest` framework being used, see [the documentation](https://docs.pytest.org/en/latest/).

Most importantly, try to make use of [`pytest.fixtures`](https://docs.pytest.org/en/latest/fixture.html) for repeatedly used code.
