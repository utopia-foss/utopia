# Model-specific plotting scripts

This python package gathers all model-specific plotting scripts.
These are scripts that go beyond the (rather general) capabilities of the `utopya.plot_funcs` subpackage in that they, e.g., perform hard-to-generalize transformations on the data before creating a plot.

While it is ok (and necessary) to use these model-specific scripts, please ask yourself if it is _really_ necessary to add a new script or if an easy change might make it possible to use a more general version from `utopya.plot_funcs` for the same purposes?

In the following, it is described how you can configure plotting functions and access your own plotting scripts.


## How to access plotting scripts
Conceptually, there are three different ways to access

### Model-specific plotting scripts
The model-specific plotting scripts are automatically available, as the path to the python directory is part of the `sys.path` for `ExternalPlotCreator`.

Thus, importing a plotting script using that creator is a simple matter of specifying the correct module and function name:
```yaml
my_plot:
  creator: external  # also the default value, so need not specify this

  # Select the module to import
  module: model_plots.<model_name>.<module_name>

  # Specify the name of the plot function to import from that module
  plot_func: <plot_func_name>

  # All further kwargs here are passed on to that plot function
  # ...
```
Such a configuration entry should be created for each available plotting function. You can add them to `<model_name>_plots.yml`. By adding the `enabled: false` entry, they can also be disabled by default.

(_Behind-the-scenes detail:_ The `model_plots` package is made available by adding its location to `sys.path`.)


### General plotting scripts (from `utopya`)
The `utopya` package also supplies some general plotting scripts. As the name suggests, these provide plotting capabilities that are _not_ specific to a model, e.g. `lineplot` which just plots a line of some `y`-data against some `x`-data.

While these functions can never be fully general, they do provide some flexibility in the way data is selected, processed, and visualized.
Thus, it is often worth to check whether they might already suit your needs.
Still, their generality will often be limiting to what you might want to visualize with plots; in those cases, feel free to create a model-specific plotting script.

General plotting scripts can be imported with the somewhat shorter, relative import syntax (note the leading `.` in the `module` string):
```yaml
my_plot_using_a_general_plot_function:
  # Select the basic
  module: .basic

  # Specify the name of the plot function to import
  plot_func: lineplot

  # Further kwargs to the lineplot method
  # Select the data
  y: uni/0/data/my_model/state

  # Set line style format and arguments for saving
  fmt: go-
  save_kwargs:
    bbox_inches: ~
    transparent: true
```
(_Behind-the-scenes detail:_ it is just a relative import from the `utopya.plot_funcs` sub-package.)



### Plotting scripts from files
It is also possible to load modules directly from files:
```yaml
my_plot_from_a_module_file:
  # Use a module file from a path
  module_file: /abs/path/to/module.py

  # Select the plot function
  plot_func: my_plot_function_in_the_module_file

  # further kwargs to that plot function
```

In order to allow relative imports, the `ExternalPlotCreator` needs to be initialized with the `base_module_file_dir` argument; paths given to the `module_file` argument are then seen as relative to that directory.

The `PlotManager` is able to set this value. To do so, set the following key in a run configuration or in your user configuration:
```yaml
plot_manager:
  creator_init_kwargs:
    external:
      # Path to my 
      base_module_file_dir: ~/my_plot_funcs
```
(Yep, this is a bit nested, but this is also a special feature... :wink:)

With this config entry set, the plot configuration is simpler:
```yaml
my_plot_from_a_relative_module_file:
  # Define the module file, now as relative path, yay
  module_file: best_plots_ever.py
  plot_func: rainbow_unicorn

  # further kwargs to that plot function
```


## How to add plotting scripts for a new model?
1. Create a directory: `python/model_plots/<model_name>`
2. As it needs to be a python package, add an `__init__.py` into the directory (can be empty)
3. For each group of plotting functions that should go together, create a module file, e.g. `ca_anims.py`.
4. In these module files, define your plotting functions. They should take the `DataManager` as positional argument and `out_path` as keyword argument.

```python
# ca_anmis.py
"""This module defines the <model_name>-specific CA animation plots"""

import matplotlib.pyplot as plt

from utopya import DataManager

def my_very_specific_ca_anim(dm: DataManager, *, out_path: str, **kwargs):
    """Creates a CA animation"""
    # do all the plotting ...
```

Note the `*` in the method signature; this forces the following arguments to be so-called _keyword-only_ arguments and it is good practice to use this syntax as it avoids confusion when passing arguments.


### Best practices
If you use code just for yourself, you can do whatever floats your boat.
However, if the model plots are to be contributed to the common repository, please follow these best practices:

* Plotting functions should not depend on plotting functions of other _models_ within the `model_plots` package.
* Try to re-use code _within_ the model-specific plotting function sub-package.
* If applicable, it is advisable to re-use plotting functions from `utopya.plot_funcs`.
* Also, some functions that perform simple tasks are shared among the subpackages; these can be imported from the `tools` module and reduce rewriting of often-used code, e.g. saving and closing a figure.
* To allow a slightly easier import: specifying `<module_name>` can be avoided by using `from .<module_name> import *` statements in `__init__.py`. While the wildcard import is considered _ok_ in this case, it should be taken care that not too many (>15) plotting functions are available directly under the `model_plots.<model_name>` scope.

If you are unsure, feel free to approach anyone in the frontend team to assist you.
