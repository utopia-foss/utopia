Scatter plots
=============

.. admonition:: Summary

    On this page, you will see how to

    * use ``.plot.facet_grid.scatter`` to plot 2-dimensional scatter plots, plotting further data dimensions on the ``hue`` and ``markersize`` properties
    * use ``.plot.facet_grid.scatter3d`` to plot 3-dimensional scatter plots, plotting further data dimensions on the ``hue`` and ``markersize`` properties
    * adjust the colormap used in the scatter plot
    * create facet grids of both 2- and 3-dimensional scatter plots by adding data dimensions to the ``row`` and/or ``col`` .

.. admonition:: Complete example: 2D scatter plot
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- scatter_2d
        :end-before: ### End --- scatter_2d

.. admonition:: Complete example: 3D scatter plot
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- scatter_3d
        :end-before: ### End --- scatter_3d

.. admonition:: Complete example: 2D scatter plot facet grid
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/multiverse_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- scatter_2d_facet
        :end-before: ### End --- scatter_2d_facet



2-Dimensional Scatter Plot
^^^^^^^^^^^^^^^^^^^^^^^^^^
You can create two-dimensional scatter plots using the ``.plot.facet_grid.scatter`` base function, which invokes the :py:func:`xarray.plot.scatter` plot, which in turn wraps :py:func:`matplotlib.pyplot.scatter`.

For example, here we are plotting a phase diagram of the :ref:`SEIRD model <model_SEIRD>` showing the number of ``susceptible`` and ``infected`` agents:

.. code-block:: yaml

    phase_diagram_SI:
      based_on:
        - .creator.universe
        - .plot.facet_grid.scatter

      select:
        kind:
          path: densities

      transform:
        - .sel: [!dag_tag kind, { kind: susceptible }]
          kwargs: {drop: true}
          tag: susceptible
        - .sel: [!dag_tag kind, { kind: infected }]
          kwargs: {drop: true}
          tag: infected

        # Combine into a Dataset
        - xr.Dataset:
            data_vars:
              susceptible: !dag_tag susceptible
              infected: !dag_tag infected
          tag: data

      # Tell the scatter plot what to plot on x- and y-axes
      x: susceptible
      y: infected

.. hint::

    ``kwargs: {drop: true}`` is necessary here to drop coordinates variables: see :py:func:`xarray.DataArray.sel`.

This will output the following plot:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/scatter_2d_simple.pdf
    :width: 800
    :alt: A simple 2d scatter plot

You can change the color of the dots using the ``color`` keyword.

This would be more useful if we knew which dot corresponded to which time step.
We can use the ``hue`` and the ``markersize`` to encode additional variables; for example, we can encode the ``time`` as the hue and the number of ``recovered`` patients as the markersize:

.. code-block:: yaml

    x: susceptible
    y: infected
    hue: time
    markersize: recovered

That will produce something like this:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/scatter_2d.pdf
    :width: 800
    :alt: 2d scatter plot with hue and markersize set

Note that this requires you to have first also included the ``kind: recovered`` in the dataset above.

.. warning::

    You can only use the ``markersize`` for actual *data variables*.
    If you only want to change the actual *size* of the markers, use the ``s`` key.

    .. code-block:: yaml

        x: susceptible
        y: infected
        s: 20

    This will set the size of the markers to 20.
    Naturally you cannot provide *both* ``markersize`` and ``s`` keys!
    The ``.plot.facet_grid.scatter`` function eventually calls :py:func:`xarray.plot.scatter`, whence the syntax originates.

You can set the colormap via the ``cmap`` key, for instance by passing the name of a `matplotlib <https://matplotlib.org/stable/tutorials/colors/colormaps.html>`_ or `seaborn <https://seaborn.pydata.org/tutorial/color_palettes.html>`_ colormap.
You can also create your own colormap from a custom color palette:

.. code-block:: yaml

    phase_diagram_SI:

      # Everything as above ...

      cmap:
        continuous: true
        from_values:
          # Add your own colors here:
          0: crimson
          0.5: darkblue
          1: gold

The keys are the positions of the colors on the colormap, and must be floats
between 0 and 1.
You can pass as many keys as you like.
See the :ref:`styles section <colormaps>` for more details on colormaps.


3-Dimensional Scatter Plot
^^^^^^^^^^^^^^^^^^^^^^^^^^
For 3-dimensional scatter plots, use the ``.plot.facet_grid.scatter3d`` base
function, which calls the corresponding `matplotlib 3d scatter function <https://matplotlib.org/stable/api/_as_gen/mpl_toolkits.mplot3d.axes3d.Axes3D.html#mpl_toolkits.mplot3d.axes3d.Axes3D.scatter>`_ for 3-dimensional axes.

Let's plot a 3-dimensional phase diagram, showing ``susceptible``, ``infected``, and ``recovered`` agents all in a single plot.
Additionally, let's encode the ``time`` dimension as the ``hue``:

.. code-block:: yaml

    phase_diagram_SIR:
      based_on:
        - .creator.universe
        - .plot.facet_grid.scatter3d

      select:
        kind:
          path: densities

      transform:
        - .sel: [!dag_tag kind, { kind: susceptible }]
          kwargs: {drop: true}
          tag: susceptible
        - .sel: [!dag_tag kind, { kind: infected }]
          kwargs: {drop: true}
          tag: infected
        - .sel: [!dag_tag kind, { kind: recovered }]
          kwargs: {drop: true}
          tag: recovered

        - xr.Dataset:
            data_vars:
              susceptible: !dag_tag susceptible
              infected: !dag_tag infected
              recovered: !dag_tag recovered
          tag: data

      x: susceptible
      y: infected
      z: recovered
      hue: time

Notice the addition of the ``z`` key.
This outputs a plot like this:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/scatter_3d.pdf
    :width: 800
    :alt: simple 3d scatter plot

Adjusting the colormap works just as in the 2-dimensional case.

If you want to change the view of the axis, use the ``PlotHelper`` to change the elevation and azimuthal angle of the view:

.. code-block:: yaml

    phase_diagram_SIR:

      # Everything as above ...

      helpers:
        setup_figure:
          subplot_kw:
            elev: 20
            azim: 45


Facet grid scatter plots
^^^^^^^^^^^^^^^^^^^^^^^^
You can plot both types of scatter plot in a facet grid, using rows and columns
as additional plot dimensions for variables.
For more details on facet grids in general, take a look at the :ref:`separate article on facet grids <facet_grid_panels>`.

A facet grid of two dimensional scatter plots might look something like this:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/scatter_2d.pdf
    :width: 800
    :alt: 2d facet grid scatter plot

Here, we have performed a sweep over the ``transmission rate`` and ``immunity rate``, which we are now plotting on the columns and rows respectively.
As with all facet grid plots, all this requires is encoding the ``row`` and ``col`` variable:

.. code-block:: yaml

    phase_diagram_SI:

      # Everything as above ...

      x: susceptible
      y: infected
      hue: time
      col: transmission rate
      row: immunity rate

And of course, the same works for three-dimensional plots (take care to change the base plot to ``.plots.facet_grid.scatter3d``!):

.. code-block:: yaml

    phase_diagram_SIR:

      # Base your plot on facet_grid.scatter3d
      based_on:
        - .creator.universe
        - .plot.facet_grid.scatter3d

      # Select and transform your data, as before
      select_and_combine:
        fields:
          kind:
            path: densities

      transform:
        - .sel: [!dag_tag kind, { kind: susceptible }]
          kwargs: { drop: true }
          tag: susceptible
        - .sel: [!dag_tag kind, { kind: infected }]
          kwargs: { drop: true }
          tag: infected
        - .sel: [!dag_tag kind, { kind: recovered }]
          kwargs: { drop: true }
          tag: recovered

        - xr.Dataset:
            data_vars:
              susceptible: !dag_tag susceptible
              infected: !dag_tag infected
              recovered: !dag_tag recovered
          tag: data

      # Distribute your variables:
      x: susceptible
      y: infected
      z: recovered
      col: transmission rate
      hue: time

      # Set a colormap, if you like
      cmap:
        continuous: true
        from_values:
          0: gold
          1: skyblue

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/scatter_3d.pdf
    :width: 800
    :alt: 3d facet grid scatter plot

You might need to adjust the figure size and the margins a little.
The ``figsize`` keyword (handled by :py:class:`xarray.plot.FacetGrid`) as well as various features of the ``PlotHelper`` might be useful here:

.. code-block:: yaml

    phase_diagram_SIR:

      # Everything as above ...

      # Control the figure size
      figsize: [8, 4]

      # Use the plot helper to set various additional features
      helpers:
        # Adjust the right border of the plot
        subplots_adjust:
          right: 0.75

        # Set the ticks.
        set_tick_formatters:
          x: &tick_format
            major:
              name: StrMethodFormatter
              args: ['{x: 0.1f}']
          z:
            <<: *tick_format
        set_ticks:
          y:
            major: [0, 0.05, 0.1]

Observe the use of YAML anchors to avoid having to type things multiple times: these are described in more detail in the :ref:`style article <plot_style>`.
The :ref:`PlotHelper <plot_helper>` gives you a variety of options to format the ticks and use specific labels.

.. note::

    If you want to change the view of the axis in the case of the **faceting 3-dimensional scatter plot**, the parameters need to be passed somewhat differently than in the non-faceting case:

    .. code-block:: yaml

        phase_diagram_SIR:
          # Everything as above ...
          subplot_kws:  # sic, with trailing s unlike within setup_figure
            elev: 20
            azim: 45

    This is because in the faceting case, the :py:class:`xarray.plot.FacetGrid` class takes care of setting up the figure, not the :py:class:`~dantro.plot.plot_helper.PlotHelper`.
    We are working on a better solution that avoids needing to specify the parameters in multiple places.
