.. _uni_and_mv_plots:

Plotting Universes and Multiverses
==================================
Utopia tries to make the plotting of multidimensional data as easy as possible.
This page describes how to create plot functions and configurations for plotting data from individual universes as well as from the multiverse.

.. contents::
    :local:
    :depth: 2

.. note::

    This page documents only a selection of the plotting capabilities.
    Make sure to also visit the `dantro documentation <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/>`_, where the plotting framework is implemented.

----

Plots from Universe Data
------------------------
To create plots that use data from a single universe, use the :py:class:`~utopya.plotting.UniversePlotCreator`.
It allows to specify a set of universes to create plots for and provides the plotting function with data from the selected universes.

The plot configuration for a universe plot requires as an additional argument a selection of which universes the plot should be created for.
This is done via the ``universes`` argument:

.. code-block:: yaml

    ---
    my_universe_plot:
      universes: all        # can also be 'first', 'any', or a dict specifying
                            # a multiverse subspace to restrict the plots to
      # ... more arguments


.. _uni_plot_with_dag:

Universe plots using DAG framework *(recommended)*
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the :ref:`creator-averse plot function definition<plot_func_sig_recommended>` and specify the ``creator`` in the plot configuration.
You can then use the `regular syntax <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/plotting/plot_data_selection.html#arguments-to-control-dag-behaviour>`_ to select the desired data, based on the currently selected universe.

When using the recommended creator-averse plot function signature, the DAG is automatically enabled and allows to select data in the following way:

.. code-block:: yaml

    my_plot:
      creator: universe
      universes: all

      # Select data within the current universe
      select:
        some_data: data/MyModel/some/path/foo
        some_other_data:
          path: data/MyModel/some/path/bar
          transform:
            - mean: [!dag_prev ]
            - increment: [!dag_prev ]

      # Perform some transformation on the data
      transform:
        - add: [!dag_tag some_data, !dag_tag some_other_data]
          tag: result

      # ... further arguments

In this case, the available tags would be ``some_data``, ``some_other_data``, and ``result``.
Furthermore, for the universe plot creator, the ``uni`` tag is always available as well.

.. note::

    This is only a glimpse into the full capabilities of data selection via the transformation framework.

    For more details, have a look at the `corresponding dantro documentation <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/plotting/plot_data_selection.html#special-case-universeplotcreator>`_ and :ref:`the general remarks on the transformation framework <external_plot_creator_DAG_support>`.

Remarks
"""""""

* To access the universe configuration, you can use the ``uni`` tag and either do a selection of the desired parameter within the DAG framework or do it in the plot function, based on the ``uni`` result tag.
* Use the ``dag_options.select_path_prefix`` option to navigate to the data path of your **model**, making subsequent path definitions in ``select`` a bit simpler.
  In the example above, the paths would just be ``some/path/foo`` and ``some/path/bar`` when setting ``dag_options.select_path_prefix`` to ``data/MyModel``, thus always starting paths within the model base group.
* To access a configuration entry within the universe, you can also use the DAG framework:

    .. code-block:: yaml

        my_plot:
          creator: universe

          select:
            # This is equivalent to uni['cfg']['foo']['bar']['some_param']
            some_param:
              path: cfg
              with_previous_result: true
              transform:
                - getitem: foo
                - getitem: bar
                - getitem: some_param


Without DAG framework
^^^^^^^^^^^^^^^^^^^^^
Without the DAG framework, the data needs to be selected manually:

.. code-block:: python

    from utopya import DataManager, UniverseGroup
    from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

    @is_plot_func(creator_type=UniversePlotCreator)
    def my_plot(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
                **additional_kwargs):
        """A universe-specific plot function using the data transformation
        framework and the plot helper framework.

        Args:
            dm: The DataManager, containing *all* data
            uni: The currently selected universe. Select the data from here.
            hlpr: The associated plot helper.
            **additional_kwargs: Anything else from the plot config. Ideally,
                specify these explicitly rather than gathering them via ``**``.
        """
        # Get the data
        x = uni['data/MyModel/foo']
        y = uni['data/MyModel/bar']

        # Plot the data
        hlpr.ax.plot(x, y)

        # Add some information from the universe configuration
        cfg = uni['cfg']
        some_param = cfg['MyModel']['some_param']
        hlpr.provide_defaults('set_title',
                              title="Some Parameter: {}".format(some_param))

        # Done. The plot helper saves the plot.

Note how the data selection is hard-coded in this example.
In other words, when not using the data selection and transformation framework, you have to either hard-code the selection or parametrize it.



----

Plots from Multiverse Data
--------------------------
To create plots that use data from *more than one* universe — henceforth called *multiverse data* — use the :py:class:`~utopya.plotting.MultiversePlotCreator`.
This creator makes it possible to select and combine the data from all selected individual universes and provides the result of the combination to the plot function.

This requires the handling of multidimensional data and depends on the dimensionality of the chosen parameter space.
Say the selected data from each universe has dimensionality three and a parameter sweep was done over four dimensions, then the data provided to the plot function has seven dimensions.

See :ref:`below <select_mv_data>` on how to control the selection and combination of data.

.. _mv_plot_with_dag:

Multiverse plots using DAG framework *(recommended)*
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Again, use the :ref:`creator-averse plot function definition<plot_func_sig_recommended>` and specify the ``creator`` in the plot configuration.
For the :py:class:`~utopya.plotting.MultiversePlotCreator`, a `special syntax <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/plotting/plot_data_selection.html#special-case-multiverseplotcreator>`_ exists to select and combine the multiverse data.

When using the recommended creator-averse plot function signature, the DAG is automatically enabled and allows to select data using the ``select_and_combine`` key:

.. code-block:: yaml

    ---
    my_plot:
      creator: multiverse

      # Multiverse data selection via DAG framework
      select_and_combine:
        fields:
          some_data: some/path/foo
          some_other_data:
            path: some/path/bar
            transform:
              - mean: [!dag_prev ]
              - increment: [!dag_prev ]

        base_path: data/MyModel     # ... to navigate to the model base group

        # Default values for combination method and subspace selection; can be
        # overwritten within the entries specified in `fields`.
        combination_method: concat  # can be 'concat' (default) or 'merge'
        subspace: ~                 # some subspace selection

      transform:
        - add: [!dag_tag some_data, !dag_tag some_other_data]
          tag: result

.. note::

    As above, this is only a glimpse into the full capabilities of data selection via the transformation framework.

    For more details, have a look at the `corresponding dantro documentation <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/plotting/plot_data_selection.html#special-case-multiverseplotcreator>`_ and :ref:`the general remarks on the transformation framework <external_plot_creator_DAG_support>`.

.. warning::

    The arguments given to ``select_and_combine`` are similar **but not equal** to those to the ``select`` argument in the :ref:`legacy syntax <select_mv_data>`!
    Check out `the documentation <https://hermes.iup.uni-heidelberg.de/dantro_doc/master/html/plotting/plot_data_selection.html#special-case-multiverseplotcreator>`_ to learn about the proper usage of the ``select_and_combine`` argument.

.. hint::

    The subspace selection happens via :py:meth:`paramspace.ParamSpace.activate_subspace`.


.. _select_mv_data:

Without DAG framework *(legacy approach)*
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. deprecated:: 0.6

    This approach of selecting data from the multiverse is **deprecated.**
    Instead, use the data selection and transformation interface described :ref:`above <mv_plot_with_dag>`.

The signature for such a plot function looks like this:

.. code-block:: python


    import xarray as xr

    from utopya import DataManager
    from utopya.plotting import is_plot_func, PlotHelper, MultiversePlotCreator

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


To select the ``mv_data``, specify the ``select`` key in the configuration.
The associated ``MultiverseGroup`` will then take care to select the desired multidimensional data. The resulting data is then bundled into a ``xr.Dataset``; see `the xarray documentation <http://xarray.pydata.org/en/stable/data-structures.html#dataset>`_ for more information.

The ``select`` argument allows a number of ways to specify which data is to be selected. The examples below range from the simplest to the most explicit:

.. code-block:: yaml

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

The fields ``data`` and ``std`` are then made available to the ``mv_data`` argument to the plotting function.

For further documentation, see the functions invoked by ``MultiversePlotCreator``, ``ParamSpaceGroup.select`` and :py:meth:`paramspace.paramspace.ParamSpace.activate_subspace`:

.. autoclass:: dantro.groups.ParamSpaceGroup
    :members: select
