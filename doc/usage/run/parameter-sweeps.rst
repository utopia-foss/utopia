.. _run_parameter_sweeps:

Multidimensional Model Runs
===========================

You want to sweep over multiple parameters? Utopia has just the right tools for you!

In Utopia, we distinguish between universes and multiverses: a multiverse is a collection of multiple universes in which all the universes are independent of each other.
So, for example if you want to sweep over *N* different seeds of the random number generator, you create *N* different universes within the run of the multiverse.

Follow these three steps to perform a multiverse run:

1. Create a run configuration file
----------------------------------

The run configuration should specify the parameters over which to sweep. Sweep parameters need to be indicated by a ``!sweep`` tag and supplied with a ``default`` value (which is used when no sweep is performed) as well as a parameter ``range`` or a set of ``values``. It can look something like this:

.. code-block:: yaml

   # My run configuration for a parameter sweep
   # Default values can be found in the base config: utopia/python/utopya/utopya/cfg/base_cfg.yml
   ---
   # Frontend configuration parameters
   # ...

   # What is passed to the C++ side (_after_ the frontend prepared it)
   parameter_space:
     # Number of simulation steps
     num_steps: 1000

     # Write data every ... steps
     write_every: 1  # Default: 1
     # NOTE You can delete this if you're using the default!

     # The seed the RNG is initialized with
     # Indicate this by adding the !sweep tag
     seed: !sweep
       default: 42    # The value which is used if no sweep is done
       range: [10]    # The values over which to sweep, here: 0, 1, ... 9
                      # Other ways to specify them:
                      #   values: [1,2,3,4]  # taken as they are
                      #   range: [1, 4]      # passed to range()
                      #   linspace: [1,4,4]  # passed to np.linspace
                      #   logspace: []       # passed to np.logspace
       # optional arguments
       # as_type: int # let python make a type call. Possible values: str, int, float, bool
       # order: 42    # change the order of parameter dimensions; default: np.inf, in which case
                      # the sorting is done by position inside the alpabetically ordered dict.
       # name: foo    # give a custom name to this parameter dimension. If not given, a unique
                      # name within the parameter space is generated, in this case: seed

     # Now, load the configuration for your specific model
     MyFancyModel:
       # Below, you can make updates to these values
       some_parameter: 42

You can use this ``!sweep`` tag on more than one parameter.
It will create a multidimensional hypercube with all possible value combinations.

Coupled sweeps
^^^^^^^^^^^^^^

If you want to vary one or parameters *along* with another (e.g. sweep over a parameter and using a different seed for each run), use the ``!coupled-sweep`` tag on the parameter(s) to be varied along with another, and add a ``target`` key; see :ref:`here <feature_parameter_sweeps>` for an example.

Once you're done, you can pass the run configuration file to the CLI like this:

.. code-block:: shell

   $ utopia run MyFancyModel path/to/run_cfg.yml

See ``utopia run --help`` for a detailed description.


2. Create a plot configuration and corresponding plots
------------------------------------------------------

.. warning::

    There are a bunch of new capabilities in the plotting framework that are not reflected in the examples below!

    Make sure to check out the :ref:`documentation of the plotting framework <eval_plotting>`.

Plotting multidimensional data can be achieved through different means depending on what you want to plot.

For the following, create a new plot configuration file and specify the desired plots you want to perform.
You can pass the plot configuration file to the CLI by adding ``--plot-cfg path/to/plot_cfg.yml``.
See ``utopia run --help`` for a detailed description.

a. Plot all or specific universes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you have a plot function which uses *only* the data of a *single* universe, you need to write something like this:

.. code-block:: yaml

   state:
     creator: universe   # Create plots for the universes, not the multiverse
     universes: all      # Choose all the universes.

     # Select the plot function just as for a simple simulation run, e.g.
     module: model_plots.MyFancyModel
     plot_func: state

     # Below, you can put the other plot-specific parameters.

This will call the ``state`` function in the ``model_plots.MyFancyModel`` module. With ``universes: all``\ , a plot is generated for each universe that was run. However, you could also specify only certain universes to plot:

.. code-block:: yaml

   state:
     creator: universe   # Create plots for the universes, not the multiverse
     universes:
        some_sweep_parameter: [val_1, val_2, val_3] # Select the universes to plot.

b. Plot a multiverse plot
^^^^^^^^^^^^^^^^^^^^^^^^^

If you need the data from mutliple universes for a single plot, you need to write a multiverse plot function.
Let's say that you want to have an average state (averaged over different model realizations i.e. random number generator seeds).
The plot configuration then looks like this:

.. code-block:: yaml

   mean_state:
     # As you need the data of many universes, select the multiverse plot creator:
     creator: multiverse

     # The `select` key is used to select a hyperslab out of the data:
     select:
       field:
         # Choose the path in the data tree (see terminal output)
         path: data/MyFancyModel/some_state

         # Label the dimensions (optional. If not given, they are called dim_0, dim_1, ...)
         dims: [time]
     # For more syntax examples, e.g. selecting multiple fields, see here:
     #   https://gitlab.com/utopia-project/dantro/merge_requests/21#interface-examples

     # Select the plot function just as for a universe plot
     module: model_plots.MyFancyModel
     plot_func: mean_state

     # Below, you can put the other plot specific parameters.
     # ...

The data specified in ``select`` will be passed to the plotting function as ``mv_data`` parameter and as an :py:class`xarray.Dataset` object.
Take a look at the `xarray documentation <http://xarray.pydata.org/en/stable/>`_ to learn more.
The big advantage of this package is that your array dimensions are now labelled, so you can just call ``.mean(dim='time')`` on your data and don't have to worry that the wrong dimension might be chosen.

In the above example, you need to write a new plot function ``mean_state``.
It could look something like this:

.. code-block:: python

   import matplotlib.pyplot as plt

   from utopya import DataManager, UniverseGroup

   from ..tools import save_and_close

   def mean_state(dm: DataManager, *,
                  out_path: str,
                  mv_data: xr.Dataset,     # Here, you get the actual data as an xarray DataSet object
                  #
                  # Below, you can add further model specific arguments
                  save_kwargs: dict=None,
                  **plot_kwargs):
       '''Plots the mean state of multiple universes'''

       # Calculate the mean state averaged over all universes.
       state = mv_data.means(dim='seed')

       # Now, you have the average state data, which you can plot.
       # NOTE: If the write_every paramter in the config is not equal to 1,
       #       you would need to adapt this plot function such that it plots the
       #       actual time step on the x axis.
       plt.plot(state['time'], state['some_state'], **plot_kwargs)

       # Save and close the figure
       save_and_close(out_path, save_kwargs=save_kwargs)


3. Perform a Multiverse Run
---------------------------

The final step is running the sweep. The terminal command to "run a multiverse" i.e. to do a parameter sweep, is:

.. code-block:: shell

   $ utopia run MyFancyModel <path_to_run_config> --sweep --plots-cfg <path_to_plot_config>

If you leave out ``--sweep``\ , utopia will just do a single universe run using the default values you provided in the run configuration.
Alternatively, you can add ``perform_sweep: true`` to the top level of your run configuration and omit the ``--sweep`` flag in the CLI command.
Again, see ``utopia run --help`` for more information.
