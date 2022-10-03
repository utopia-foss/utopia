.. _line_plots:

Line plots and Errorbars
========================

.. admonition:: Summary

    On this page, you will learn how to

    * use ``.plot.facet_grid.line`` for line plots, and
    * use ``.plot.facet_grid.errorbars`` or ``.plot.facet_grid.errorbands`` for errorbars and errorbands.

.. admonition:: Complete example: Line plot
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- line_plot
        :end-before: ### End --- line_plot

.. admonition:: Complete example: Errorbar plot
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/multiverse_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- errorbars
        :end-before: ### End --- errorbars


A simple line plot
^^^^^^^^^^^^^^^^^^
Let us begin with a very simple, yet very common example: line plots, and line plots with errorbars.
We will use the :ref:`Utopia SEIRD model <model_SEIRD>` as an example, but it should always be very obvious how to adapt the configurations to your specific case.

The SEIRD model simulates the densities of different classes of agents over time: those who are susceptible to the disease, those who are infected, those who have recovered, and so on.
Let's say we just want to plot the number of infected agents over time.
Here is a plot configuration that generates such a plot:

.. code-block:: yaml

    infected_density:

      based_on:
        - .creator.universe
        - .plot.facet_grid.line

      select:
        data:
          path: data/SEIRD/densities
          transform:
            - .sel: [ !dag_prev , { kind: [infected] }]

There are two top-level entries to this config: :code:`based_on` and :code:`select`:

* :code:`based_on` is telling the plotting framework which underlying functions to use for this plot.

  - :code:`.creator.universe` is telling us that this is a :code:`universe` plot. If you were
    to perform a multiverse run, the :code:`universe creator` would then create a separate
    plot for every run.

  - :code:`.plot.facet_grid.line` is the actual plot function we are using.
    This is a :py:func:`pre-implemented function <dantro.plot.funcs.generic.facet_grid>`, and will plot a simple line.

* :code:`select` selects the data.
  We include the path to the data (you should adapt this to your own case), and select the :code:`kind` of agents we want (here: only the infected ones).
  The :code:`dag_prev!` tag tells the data transformation framework to apply the selection filter to the previous node in the DAG, the previous node in this case being the ``densities`` dataset (more specifically: its result, a ``xr.DataArray``).

.. hint::

    You can think of the syntax as analagous to mapping to a Python function call:

    .. code-block:: python

        data = select(path="data/SEIRD/densities").sel({"kind": ["infected"]})

    with ``select`` being a custom data selection function and ``!dag_prev`` having turned into the attribute call on its ``return`` value.

.. hint::

    If you're wondering why the plot function is called :code:`facet_grid.line` and not something simpler like just :code:`line`, take a look at the :ref:`facet_grids` section.
    In brief: The :py:func:`~dantro.plot.funcs.generic.facet_grid` plot function is capable of much more and we are using only a small aspect of it here.

.. note::

   You must *always* leave a space after a DAG tag, e.g. after :code:`!dag_tag` or :code:`!dag_prev` .

This is the output:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/density_basic.pdf
  :width: 800
  :alt: A simple line plot

Not bad! By default, you'll get an :code:`infected_densities.pdf` output in your output directory.
If, for example, you want a :code:`png` file instead, add the following entries:

.. code-block:: yaml

    infected_density:

      # all the previous entries ...

      file_ext: png
      style:
         savefig.dpi: 300

The :code:`savefig.dpi` key is optional; you can use it increase the resolution on your plots, e.g. for publications.

Another thing we may want to do is plot several lines all in one plot – for that, see the next section on :ref:`facet grids <facet_grids>`.


Changing the appearance
^^^^^^^^^^^^^^^^^^^^^^^
Now let's make the whole thing a bit prettier by adding a title and axis labels, changing the color, and
using latex:

.. code-block:: yaml

    infected_density:
      # ...
      # Add this to the configuration from above:
      style:
        text.usetex: true
        figure.figsize: [5, 4]
        font.size: 10

      color: crimson

      helpers:
        set_labels:
          y: Density [1/A]
        set_title:
          title: Density of infected agents

The :code:`helpers` entry sets labels and titles for your axes, among other things.
We'll go into more detail about customising the aesthetics in :ref:`plot_style`; for now, these few changes are enough to create a much cleaner plot:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/line_plot.pdf
  :width: 600
  :alt: A simple but prettier line plot



.. _errorbars:

Plotting errorbars
^^^^^^^^^^^^^^^^^^

In probabilistic modelling, you naturally want to be sure that your outputs are not just a coincidence, an artefact of running the model with some 'lucky' seed, but actually statistically significant effects.
To get some statistics on your outputs, you may therefore wish to run the model over several different seeds, and plot an averaged
output with some errorbars.

Let's run our :ref:`SEIRD <model_SEIRD>` model over a number of different seeds, and plot the resulting curve of infected agents with errorbars:

.. code-block:: yaml

    averaged_infected_density:
      based_on:
        - .creator.multiverse
        - .plot.facet_grid.errorbars

      select_and_combine:
        fields:
          infected:
            path: data/SEIRD/densities
            transform:
              - .sel: [ !dag_prev , { kind: [infected] }]

      transform:
        # Get the x-axis
        - .coords: [!dag_tag infected , time]
          tag: time

        # Calculate mean and standard deviations along the 'seed' dimension
        - .mean: [!dag_tag infected, seed]
          tag: infected_mean
        - .std: [!dag_tag infected, seed]
          tag: infected_std

        # Bundle everything together
        - xr.Dataset:
          - avg: !dag_tag infected_mean
            err: !dag_tag infected_std
          tag: data

      x: time
      y: avg
      yerr: err

      # Additional kwargs, passed to the plot function
      elinewidth: 0.5
      capsize: 2
      color: crimson

Several things are important:

#. First, this is a :code:`multiverse` plot, so we must base the plot on the :code:`.creator.multiverse`, as well as on :code:`.plot.facet_grid.errorbars` to get the errorbars.

#. For multiverse plots, you must use the :code:`select_and_combine` key to select data:
   This will assemble a multidimensional dataset with labelled axes, enabling selection along parameter dimensions.

#. We have added a new block to our configuration: the :code:`transform` block.
   This is the transformation part of our data analysis, and is telling the DAG how to process the data.
   Let's go through it step by step:

   * First, we extract the x-axis of the plot by selecting the :code:`time` coordinate:

     .. code-block:: yaml

        - .coords: [!dag_tag infected, time]
          tag: time

   * Then we calculate the averages and errors using the :code:`.mean` and :code:`.std` operations.
     Note how these operations are applied to the :code:`!dag_tag infected` node of the DAG:

     .. code-block:: yaml

        - .mean: [!dag_tag infected, seed]
          tag: infected_mean

     We are averaging the number of infected agents over the :code:`seed` dimension, and giving it a :code:`tag`, so that we can later reference this step in the tranformation.
     Calculating the variance is analogous.

   * Lastly, we bundle everything up into an :py:class:`xarray.Dataset`:

     .. code-block:: yaml

        - xr.Dataset:
          - avg: !dag_tag infected_mean
            err: !dag_tag infected_std
          tag: data

     The data variables (:code:`data_vars`) are the averages and standard deviations we calculated previously, and we can reference them using the :code:`!dag_tag` s we assigned them.

#. Then we call the :py:func:`plot function <dantro.plot.funcs.generic.facet_grid>`, telling it which data variables to plot where by specifying the :code:`x`, :code:`y`, and :code:`yerr` keys.
   Any additional keys (such as the errorbar line width) are passed to the low-level plot function, :py:func:`matplotlib.pyplot.errorbar`, giving us this output:

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/errorbars.pdf
  :width: 800
  :alt: An errorbar plot

Pretty neat -- but it really looks like continuous errorbands are the way to go with such a high number of data points.
All you need to do is to change the plot everything is based on:

.. code-block:: yaml

    based_on:
      # ...
      # - .plot.facet_grid.errorbars
      - .plot.facet_grid.errorbands         # sets use_bands: true
      # ...

.. image:: ../../../_static/_gen/SEIRD/multiverse_plots/errorbands.pdf
  :width: 800
  :alt: An errorbands plot

Much better!
