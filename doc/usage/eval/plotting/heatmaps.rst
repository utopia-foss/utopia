.. _plot_heatmaps:

Heatmaps
========

There are two options for plotting heatmaps:
For general heatmaps, ``.plot.facet_grid`` with ``kind: pcolormesh`` offers many possibilities.
In the case of cellular automata (CA) time series data, there is :ref:`a specialized plot function <plot_ca>`: ``.plot.ca``.

.. admonition:: Summary

    On this page, you will see how to

    * use ``.plot.facet_grid`` with ``kind: pcolormesh`` to plot heatmaps and spatially two-dimensional plots.
    * use ``.plot.ca`` to create plots of cellular automata (CA).
    * adjust the style and coloring of your plots.
    * animate your heatmaps based on ``pcolormesh`` by passing a ``frames`` key.
    * animate your heatmaps based on ``.plot.ca``, with more details in the :ref:`section on animations <plot_animations>`.

.. admonition:: Complete example: Heatmap with ``pcolormesh``
    :class: dropdown

    .. literalinclude:: ../../../_cfg/ForestFire/age_plot/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- pcolormesh
        :end-before: ### End --- pcolormesh

.. admonition:: Complete example: Cellular Automaton Plot
    :class: dropdown

    .. literalinclude:: ../../../_cfg/ForestFire/age_plot/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- .plot.ca
        :end-before: ### End --- .plot.ca



.. _pcolormesh:

Plotting heatmaps with ``pcolormesh``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Let's look at the ``pcolormesh``-based approach first:

.. code-block:: yaml

    heatmap:
      based_on:
        - .creator.universe            # or .creator.multiverse
        - .plot.facet_grid.pcolormesh  # short for `kind: pcolormesh`

      # further entries here ...

For instance, let us plot the age of each tree in the :ref:`ForestFire <model_ForestFire>` model at the final time step:

.. code-block:: yaml

    heatmap:
      based_on:
        - .creator.universe
        - .plot.facet_grid.pcolormesh

      # Select the age of the trees at time = -1
      select:
        data:
          path: age
          transform:
            - .isel: [!dag_prev , {time: -1}]

      x: x

Notice that we need to select a time step in the ``select`` section of the config. This produces a plot like this:

.. image:: ../../../_static/_gen/ForestFire/age_plot/forest_age_with_pcolormesh.pdf
   :width: 800
   :alt: The Forest age from pcolormesh

The ``x`` key is optional, but makes sure that the ``x``-dimension is plotted on the x-axis (and not the y-axis).
As this is a :py:func:`~dantro.plot.funcs.generic.facet_grid` plot, we can specify further axes onto which to plot data: ``pcolormesh`` supports the following encodings:

* ``x``: the x-axis
* ``y``: the y-axis
* ``row``: the rows of the facet grid
* ``col``: the columns of the facet grid
* ``frames``: animation frames

For instance, you can drop the ``transform`` argument in the above configuration, thereby selecting all time steps, and plot the ``time`` variable as the frames of an animation.
If you do this, you must additionally base your plot on an animation base plot, e.g. ``.animation.ffmpeg``:

.. code-block:: yaml

    animated_heatmap:

      # Also include .animation.ffmpeg (or .animation.frames)
      based_on:
        - .creator.universe
        - .plot.facet_grid.pcolormesh
        - .animation.ffmpeg              # or .animation.frames for PDF frames

      select:
        data: age

      x: x
      frames: time

We will discuss animations in more detail in the :ref:`animations section <plot_animations>`.


Changing the appearance
^^^^^^^^^^^^^^^^^^^^^^^
Use the ``PlotHelper`` (see :ref:`here <plot_helper>`) to set titles, axis labels, scales, annotations, and much more.

Colormaps
"""""""""
With the dantro :py:class:`~dantro.plot.utils.color_mngr.ColorManager`, adjusting the colormap is easy:
Just add a ``cmap`` key to the plot configuration.
You can define your own continuous or discrete colormap right from the configuration:

.. code-block:: yaml

    my_plot:

      # Everything as before ...

      # Add this to the above configuration:
      cmap:
        continuous: true
        from_values:
          0: crimson
          0.5: gold
          1: dodgerblue

Take a look at the :ref:`style section <colormaps>` for more details.
Alternatively, you can set a `predefined matplotlib <https://matplotlib.org/stable/tutorials/colors/colormaps.html>`_ or `seaborn <https://seaborn.pydata.org/tutorial/color_palettes.html>`_ colormap.



.. _plot_ca:

Plotting 2D states with ``.plot.ca``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Equally capable is the ``.plot.ca`` plot function (implemented :py:func:`in utopya <utopya.eval.plots.ca.caplot>`), which is optimized for plotting two-dimensional cellular automata, such as the grid-based Utopia :ref:`SEIRD <model_SEIRD>` and :ref:`ForestFire <model_ForestFire>` models.

To plot a snapshot of a two-dimensional state, base your plot on ``.plot.ca`` *and* include the ``.plot.ca.snapshot`` modifier.
You can specify the time of the snapshot with the ``frames_isel`` argument (``-1`` by default).
Here is an example for the ForestFire model, using the ``age`` variable:

.. code-block:: yaml

    forest_age_final:
      based_on:
        - .creator.universe
        - .plot.ca
        - .plot.ca.snapshot

      select:
        age: data/ForestFire/age

      frames_isel: -1  # last frame

      to_plot:
        age:
          title: Forest Age
          cmap: YlGn

This will produce something like this:

.. image:: ../../../_static/_gen/ForestFire/age_plot/forest_age_with_ca.pdf
    :width: 800
    :alt: The Forest age from .plot.ca

Just like ``pcolormesh``, ``.plot.ca`` supports animations.
To animate, simply remove the ``.plot.ca.snapshot`` reference in the above code.
You do not need to add an animation base plot, since this is already part of ``.plot.ca``.
More details on this are given in the :ref:`animations article <plot_animations>`.


.. _plot_ca_hex:

Hexagonal grids
"""""""""""""""
Aside from the typically used square grid discretizations, the Utopia ``CellManager`` :ref:`supports a hexagonal discretization <cell_manager_grid_discretization>` as well.
For some model dynamics, the grid discretization can have an effect on the behavior, e.g. because all neighbors are at an equal distance (unlike with a Moore neighborhood in a square grid).

Correspondingly, the :py:func:`utopya.eval.plots.ca.caplot` invoked by ``.plot.ca`` has support to visualize hexagonal grids.
By default, there is nothing you need to do:
The grid structure and its properties are stored alongside the data and the underlying :py:func:`~utopya.eval.plots.ca.imshow_hexagonal` plotting function reads that metadata to generate the appropriate visualization.
In effect, the same ``.plot.ca``-configurations used above are also valid for hexagonal grid structure.

Let's say we have told the model's ``CellManager`` to use a hexagonal grid, e.g. as is done in the ``hex_grid`` config set of the :ref:`SEIRD model <model_SEIRD>`:

.. code-block:: bash

    utopia run SEIRD --cs hex_grid

The resulting ``ca/state`` plot will create output like this.

.. image:: ../../../_static/_gen/SEIRD/hex_grid/kind_snapshot.pdf
    :width: 800
    :alt: A hexagonal SEIRD grid visualized by .plot.ca

For more information on available visualization options, see :py:func:`~utopya.eval.plots.ca.caplot`.

.. hint::

    Have a look at the ``grid_structure_sweep`` config set to compare the effect of the different discretizations on the SEIRD model.

.. note::

    The underlying function to draw the hexagons, :py:func:`~utopya.eval.plots.ca.imshow_hexagonal`, is also available for use in facet grid by setting ``kind: imshow_hexagonal`` or using the ``.plot.facet_grid.imshow_hexagonal`` base configuration.

    .. code-block:: yaml

        my_own_hexgrid_plot:
          based_on:
            - .creator.universe
            - .plot.facet_grid.imshow_hexagonal

          # ...
