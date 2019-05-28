Plotting
========

Utopia provides you with a selection of ready-to-use plot functions which just
need to be configured in the plot configuration. Each plot function documentation has an example of how to configure a plot, while this section
also includes some general information on plotting with utopya.

.. contents::
   :local:
   :depth: 2

.. toctree::
   :maxdepth: 1
   :caption: List of plot functions
   :glob:

   plot_funcs/*


.. todo::

  Extend the documentation to include more information about the plot helpers, how exactly they can be configured, and the ``style`` parameter.

----


.. _plot_cfg_inheritance:

Plot Configuration Inheritance
------------------------------
New plot configurations can be based on existing ones. This makes it very easy to define various plot functions without copy-pasting the plot configurations.

To do so, add the ``based_on`` key to your plot configuration.
As arguments, you can provide either a string or a sequence of strings, where the strings have to refer to names of so-called "base plot configuration entries". These are configuration entries that utopya and the models already provide.

For example, the following lines suffice to generate a simple line plot based on the plot function configured as ``.basic_uni.lineplot``:

.. code-block:: yaml

  tree_density:
    based_on: .basic_uni.lineplot

    model_name: ForestFire
    path_to_data: tree_density

What happens there is that first the configuration for ``.basic_uni.lineplot`` is loaded and then recursively updated with those keys that are specified below the ``based_on``.
When providing a sequence, e.g. ``based_on: [foo, bar, baz]``, the first configuration is used as the base and is subsequently recursively updated with those that follow.

For a list of all base plot configurations provided by utopya, see :doc:`here <inc/base_plots_cfg>`.


.. _external_plot_funcs:

External Plotting Functions
---------------------------

While there are those plot function implementations that are provided by utopya and those that are provided alongside models, you might also want to implement a plot function decoupled from all the existing code.

This can be achieved by specifying the ``module_file`` key instead of the ``module`` key in the plot configuration. The python module is then loaded and the ``plot_func`` key is used to retrieve the plotting function:

.. code-block:: yaml

   # Custom plots configuration file using an external python script
   ---
   my_plot:
     # Load the following file as a python module:
     module_file: ~/path/to/my/python/script.py

     # Use the function with the following name from that module
     plot_func: my_plot_func

     # ... all other arguments (as usual)


.. _plot_func_signature:

Required Plot Function Signatures
---------------------------------
The required signature that plotting functions need to adhere to in order to be callable from the ``universe`` and ``multiverse`` plot creators depends on whether the ``PlotHelper`` framework is used or not.

The aim of the helper framework is to let the plot functions focus on what cannot easily be automated: analyzing which data is to be plotted and conveying that to the framework.
One should not have to worry about the aesthetics too much.

The ``PlotHelper`` framework can make your life easier by quite a lot as it already takes care of most of the plot aesthetics by making large parts of the matplotlib interface accessible via the plot configuration.
That way, you don't need to touch Python code for trivial tasks like changing the plot limits. It also takes care of setting up a figure and storing it in the appropriate location.

Most importantly, it will make your plots future-proof and let them profit from upcoming features. A glimpse of that can be seen in how easy it is to implement an animated plot, see :ref:`plot_animations`.


With ``PlotHelper`` framework
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When using the ``PlotHelper``, the ``is_plot_func`` decorator is to be used, which also specifies the creator that is to be used, alleviating the need to specify it in the configuration.

.. code-block:: python

  import xarray as xr

  from utopya import DataManager, UniverseGroup
  from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator
  
  @is_plot_func(creator_type=UniversePlotCreator)
  def universe_plot(dm: DataManager, *, hlpr: PlotHelper, uni: UniverseGroup, 
                    **plot_kwargs):
      """...
      
      Args:
          dm (DataManager): The data manager from which to retrieve the data
          hlpr (PlotHelper): The PlotHelper that instantiates the figure and
              takes care of plot aesthetics (labels, title, ...) and saving
          uni (UniverseGroup): The selected universe data
          **plot_kwargs: Any additional kwargs
      """
      # ...
      # Call the plot function on the currently selected axis via PlotHelper
      hlpr.ax.plot(times, mean, **plot_kwargs)
      # NOTE `hlpr.ax` is just the current matplotlib.axes object. It has the
      #      same interface as `plt`, aka `matplotlib.pyplot`
  
      # Provide the plot helper with some information that is then used when
      # the helpers are invoked
      hlpr.provide_defaults('set_title', title="Mean '{}'".format(mean_of))
      hlpr.provide_defaults('set_labels', y="<{}>".format(mean_of))
      # NOTE Providing defaults recursively updates an existing configuration
      #      and marks the helper as 'enabled'


  @is_plot_func(creator_type=MultiversePlotCreator)
  def multiverse_plot(dm: DataManager, *, hlpr: PlotHelper,
                      mv_data: xr.Dataset, **plot_kwargs):
      """...
      
      Args:
          dm (DataManager): The data manager from which to retrieve the data
          hlpr (PlotHelper): The PlotHelper that instantiates the figure and
              takes care of plot aesthetics (labels, title, ...) and saving
          mv_data (xr.Dataset): The selected multiverse data
          **plot_kwargs: Any additional kwargs
      """
      # ...


The most important methods of the ``PlotHelper`` interface are the following:

.. autoclass:: dantro.plot_creators.PlotHelper
  :members: ax, available_helpers, mark_enabled, provide_defaults, _hlpr_set_title, _hlpr_set_labels, _hlpr_set_limits

A plot configuration that sets some of these helpers could look like this:

.. code-block:: yaml

  cluster_size_distribution:
    # ...

    # Configure some plot helpers
    helpers:
      set_title:
        title: Cluster Size Distribution
      set_labels:
        x: $\log_{10}(A)$
        y: $N_A$
      set_scales:
        y: log
      set_limits:
        x: [0, max]
        y: [0.8, ~]



Without ``PlotHelper`` framework
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In cases where the ``PlotHelper`` is not to be used, the signatures looks like this:

.. code-block:: python

  import xarray as xr

  from utopya import DataManager, UniverseGroup

  def bare_basics(dm: DataManager, *,
                  out_path: str,
                  **additional_kwargs):
      """Bare-basics signature required by `external` plot creator.

      Args:
          dm: The DataManager object that contains all loaded data.
          out_path: The generated path at which this plot should be saved
          **additional_kwargs: Anything else that was defined in the plot
              configuration. Consider declaring the keywords explicitly
              instead of using the ** to gather all remaining arguments.
      """
      # Your code here ...

      # Save to the specified output path
      plt.savefig(out_path)


  def universe_plot(dm: DataManager, *,
                    out_path: str,
                    uni: UniverseGroup,
                    **additional_kwargs):
      """Signature required by the `universe` plot creator.

      Args:
          ...
          uni: Contains the data from a single selected universe
      """
      # Your code here ...

      # Save to the specified output path
      plt.savefig(out_path)


  def multiverse_plot(dm: DataManager, *,
                      out_path: str,
                      mv_data: xr.Dataset,
                      **additional_kwargs):
      """Signature required by the `multiverse` plot creator.

      Args:
          ...
          mv_data: The extracted multiverse data for the chosen universes.
      """
      # Your code here ...

      # Save to the specified output path
      plt.savefig(out_path)

.. note::

  It is highly recommended to use the ``out_path`` argument for saving the
  figure. This makes use of the existing interface and puts the output data
  in a directory relative to the simulation data.

An example plot configuration then requires the matching ``creator`` key:

.. code-block:: yaml

  my_universe_plot:
    creator: universe
    module: my_module
    plot_func: universe_plot
  
  my_multiverse_plot:
    creator: multiverse
    module: my_module
    plot_func: multiverse_plot

    # Select the data that is extracted and passed as mv_data argument
    select:
      field: path/to/data


.. _select_mv_data:

Selecting Multiverse Data
-------------------------
To select data for a multiverse plot (i.e. using the ``MultiversePlotCreator``), specify the ``select`` key in the configuration.
The associated ``MultiverseGroup`` will then take care to select the desired multidimensional data. The resulting data is then bundled into a ``xr.Dataset``; see `the xarray documentation <http://xarray.pydata.org/en/stable/data-structures.html#dataset>`_ for more information.

The ``select`` argument allows a number of ways to specify which data is to be selected. The examples below range from the simplest to the most explicit:

.. code-block::

  # Select a single field from the data
  select:
    field: data/MyModel/density  # will be available as 'density' data variable

  # Select multiple data variables from a subspace of the parameter space
  select:
    fields:
      data: data/MyModel/some_data
      std:
        path: data/MyModel/stddev_of_some_data
    subspace:
      some_param:      [23, 42]              # select two entries by value
      another_param:   !slice [1.23, None]   # select a slice by value
      one_more_param:  {idx: -1}             # select single entry by index
      one_more_param2: {idx: [0, 10, 20]}    # select multiple by index

For further documentation, see the functions invoked by ``MultiversePlotCreator``, ``ParamSpaceGroup.select`` and ``ParamSpace.activate_subspace``:

.. autoclass:: dantro.groups.ParamSpaceGroup
  :members: select

.. autoclass:: paramspace.ParamSpace
  :members: activate_subspace


.. _plot_animations:

Animations
----------
By the powers of ``dantro`` and its plotting framework, it is really simple to
let your plot function support animation.

Say you have defined the following plot function, making use of the ``PlotHelper``:

.. code-block:: python

    from utopya import DataManager, UniverseGroup
    from utopya.plotting import UniversePlotCreator, is_plot_func, PlotHelper

    @is_plot_func(creator_type=UniversePlotCreator)
    def plot_some_data(dm: DataManager, *, hlpr: PlotHelper,
                       uni: UniverseGroup, 
                       data_path: str,
                       time: int,
                       **plot_kwargs):
        """Plots the data at `data_path` for the selected `time`."""
        # Get the y-data
        y_data = uni[data_path][time]

        # Via plot helper, perform a line plot
        hlpr.ax.plot(y_data, **plot_kwargs)

        # Dynamically provide some information to the plot helper
        hlpr.provide_defaults('set_title', title=data_path)
        hlpr.provide_defaults('set_labels', y=dict(label=data_path))

To now make this function support animation, you only need to extend it by the
update function and mark it as supporting an animation:

.. code-block:: python

    from utopya import DataManager, UniverseGroup
    from utopya.plotting import UniversePlotCreator, is_plot_func, PlotHelper

    @is_plot_func(creator_type=UniversePlotCreator, supports_animation=True)
    def plot_some_data(dm: DataManager, *, hlpr: PlotHelper,
                       uni: UniverseGroup, 
                       data_path: str,
                       time: int,
                       **plot_kwargs):
        """Plots the data at `data_path` for the selected `time`."""
        # Get the y-data
        y_data = uni[data_path][time]

        # Via plot helper, perform a line plot
        hlpr.ax.plot(y_data, **plot_kwargs)

        # Dynamically provide some information to the plot helper
        hlpr.provide_defaults('set_title', title=data_path)
        hlpr.provide_defaults('set_labels', y=dict(label=data_path))

        # End of regular plot function
        # Define update function

        def update():
            """The animation update function: a python generator"""
            # Go over all available times
            for idx, y_data in enumerate(uni[data_path]):
                # Clear the plot and plot anew
                hlpr.ax.clear()
                hlpr.ax.plot(y_data, **plot_kwargs)

                # Set the title
                hlpr.invoke_helper('set_title', title="Time {}".format(idx))

                # Done with this frame. Yield control to the plot framework,
                # which will take care of grabbing the frame.
                yield

        # Register the animation update with the helper
        hlpr.register_animation_update(update)


Ok, so the following things happened:

    * ``update`` function defined
    * ``update`` function passed to helper via ``register_animation_update``
    * Plot function marked ``supports_animation``.


.. autoclass:: dantro.plot_creators.PlotHelper
  :members: register_animation_update

There are a few things to look out for:
    * In order for the animation update actually being used, the feature needs
      to be enabled in the plot configuration. The behaviour of the animation
      is controlled via the ``animation`` key; in it, set the ``enabled`` flag.
    * While whatever happens before the registration of the animation function
      is also executed, the animation update function should be build such as
      to also include the initial frame of the animation. This is to allow the
      plot function itself to be more flexible and the animation update not
      requiring to distinguish between initial frame and other frames.
    * The animation update function is expected to be a so-called Python
      Generator, thus using the yield keyword. For more information, have a
      look `here <https://wiki.python.org/moin/Generators>`_.
    * The file extension is taken care of by the ``PlotManager``, which is why
      it needs to be adjusted on the top level of the plot configuration, e.g.
      when storing the animation as a movie.

An example for an animation configuration is the following:

.. code-block:: yaml

  my_plot:
    # Regular plot configuration
    # ...

    # Specify file extension to use, with leading dot (handled by PlotManager)
    file_ext: .png        # change to mp4 if using ffmpeg writer

    # Animation configuration
    animation:
      enabled: true       # false by default
      writer: frames      # which writer to use: frames, ffmpeg, ...
      writer_kwargs:      # additional configuration for each writer
        frames:           # passed to 'frames' writer
          saving:         # passed to Writer.saving method
            dpi: 254

        ffmpeg:
          init:           # passed to Writer.__init__ method
            fps: 15
          saving:
            dpi: 92
          grab_frame: {}  # passed to Writer.grab_frame and from there to savefig

      animation_update_kwargs:  {} # passed to the animation update function
