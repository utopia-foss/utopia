.. _plot_abm:

Plotting agents in a domain
===========================

.. admonition:: Summary

  On this page, you will see how to

  * use ``.plot.abm`` to plot snapshots and animations of agents in a domain
  * plot data variables onto the agent marker size, orientation, and color
  * add fading trails to the agents moving around the domain
  * adjust the plot for periodic boundary conditions
  * adjust the plot domain

.. admonition:: Complete example: Agents in a domain (SimpleFlocking model)
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SimpleFlocking/demo/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- agents_in_domain
        :end-before: ### End --- agents_in_domain

Agent-based models (ABMs) are a common type of model to simulate and analyze the behaviour of individual *agents*
moving around in space, interacting with each other, and adapting their behaviour or even learning in the process.
Utopia uses utopya's ``.plot.abm`` plot (implemented in utopya as :py:func:`~utopya.eval.plots.abm.abmplot`) to visualize the behavior of such agents.
We will use the :ref:`SimpleFlocking ABM <model_SimpleFlocking>` to illustrate its functionality.


.. admonition:: See also

    The implementation of this plot is in :py:mod:`utopya.eval`, which is where you can find details about the available arguments and their behavior:

    * Implementation of ``.plot.abm``: :py:func:`utopya.eval.plots.abm.abmplot`
    * Helper function that draws individual agents: :py:func:`utopya.eval.plots.abm.draw_agents`
    * Custom matplotlib collection for individual agents: :py:class:`utopya.eval.plots.abm.AgentCollection`
    * `User manual <https://utopya.readthedocs.io/en/latest/eval/plot_funcs.html#plot-abm-visualize-agent-based-models-abm>`_



Animated agents in a domain
---------------------------
Below is an example animation of flocking dynamics in a square domain with periodic boundary conditions:

.. raw:: html

    <video width="600" src="../../../_static/_gen/SimpleFlocking/demo/1__seed_0.mp4" controls></video>

The agents' color represents their orientation angle, and the fading trails their trajectory over a number of past steps.
Such animations can be easily generated using the  ``.plot.abm`` base plot.

The plot requires a single :py:class:`xarray.Dataset` or an :py:class:`xarray.DataArray` to be passed containing all the plotting data.
Base your plot on the universe creator (``.creator.universe``), ``.plot.abm``, as well as an animation base configuration (see the
:ref:`animations page <plot_animations>` for more details on these, including on adjusting the animation resolution):

.. code-block:: yaml

  agents_in_domain:
    based_on:
      - .creator.universe
      - .plot.abm
      - .animations.ffmpeg
    select:
      agents: path/to/agent/data

Next, specify what to plot, and where to plot the variables:

.. code-block:: yaml

  agents_in_domain:

    # Everything from above ...

    to_plot:
      agents:
        marker: wedge

    x: x
    y: y
    frames: time
    orientation: orientation
    hue: orientation

This plots the variables onto the corresponding dimensions.
The ``to_plot`` argument is required; it can also contain information on the agent markers, colors, and size scales -- see below.
Naturally, this requires the data we selected to contain variables called ``x``, ``y``, ``time``, and ``orientation`` (which can be mapped to other variables using the corresponding keyword arguments).

.. hint::

    Arguments on the top-level of the plot config are used as defaults for all entries within ``to_plot``.
    If the individual ``to_plot`` entries have different encodings, you can also specify those arguments there:

    .. code-block:: yaml

        agents_in_domain:
          # ...
          to_plot:
            agents:
              # custom parameters for this layer
              x: y
              y: x

If you have saved these datasets separately, use the :ref:`transformation DAG <dag_intro>` to combine them into a single :py:class:`xarray.Dataset`:

.. code-block:: yaml

  agents_in_domain:
    based_on:
      # As before ...

    select:
      x:
        path: path/to/x/coordinates
        transform: [.data] # Transform into a xr.DataArray
      y:
        path: path/to/y/coordinates
        transform: [.data]
      # analogously for other variables ...

    # Combine data into one dataset
    transform:
      - xr.Dataset:
        - x: !dag_tag x
          y: !dag_tag y
          # other variables ...
        tag: agents

    to_plot:
      agents:
        marker: wedge

.. hint::

  You can turn off the time stamp by setting

  .. code-block:: yaml

    suptitle_fstr: False

Plotting frames and snapshots
-----------------------------
You can of course choose specific time steps to use for the animation. To do this, pass the ``frames_isel`` keyword:

.. code-block:: yaml

  agents_in_domain:

    # Everything else as above ...

    frames_isel: !range [10]

This will plot the first ten time steps.
Alternatively, you can pass a list of frames.
If you base the plot on ``animation.frames`` and do *not* pass ``frames_isel``, all frames will be plotted as individual images.

You can also plot **snapshots** instead of an animation.
To do this, add the ``.plot.abm.snapshot`` base configuration; then, select the time frames to plot with ``frames_isel``:

.. code-block:: yaml

    agents_in_domain:
      based_on:
        - .creator.universe
        - .plot.abm
        - .plot.abm.snapshot

      # Snapshot of the final state (default)
      frames_isel: -1




Agent hue, orientation, size, and markers
-----------------------------------------
As we saw above, the ``orientation`` and ``hue`` of the agents can be used to visualize data variables. You can then specify
a colormap using the `ColorManager <https://dantro.readthedocs.io/en/latest/plotting/color_mngr.html>`_:

.. code-block:: yaml

  agents_in_domain:

    # Everything as above

    cmap:
      continuous: true
      from_values:
        0.0: darkgreen
        0.333: yellow
        0.666: brickred
        1.0: darkgreen

Colors can also be specified e.g. as hex or rgb values. You can also simply pass the name of a colormap, use a norm,
and include limits: take a look at the :ref:`style section <colormaps>` for more details.

Additionally, you can plot data dimensions onto the agent ``size``. The size scale and marker style of the agents is determined via the ``size_scale`` key in the ``to_plot`` entry, which roughly corresponds to the area of the marker in relation to the whole domain:

.. code-block:: yaml

  agents_in_domain:

    # Everything as above ...

    size: some_dimension

    to_plot:
      agents:
        size_scale: 0.0002
        marker: fish2

The ``marker`` argument can be any one of ``wedge``, ``fish`` (a basic fish shape), and ``fish2`` (a more complex fish shape); see :py:data:`~utopya.eval.plots.abm.MARKERS` for more information and other available marker paths:

.. image:: ../../../_static/_gen/SimpleFlocking/demo/1__seed_0.pdf
    :width: 800
    :alt: ABM plot with fish

.. hint::

    See :py:class:`~utopya.eval.plots.abm.AgentCollection` for more possibilities on how to set up the ``marker``.


Adding trails
-------------
Trails can be useful to visualize the trajectories of the agents, especially in flocking or chemotaxis models.
You can control the length of the tail with the ``tail_length`` entry. The aesthetics of the tail are controlled via the ``tail_kwargs`` dict:

.. code-block:: yaml

  agents_in_domain:

    tail_length: 12
    tail_kwargs:
      color: black
      linewidth: 0.5
      alpha: .6
      zorder: -10

This sets the ``tail_length`` to 12 frames of the animation.

The ``alpha`` value is applied uniformly along the length of the tail.
To get a fading effect, add the following key:

.. code-block:: yaml

  agents_in_domain:

    tail_decay: 0.12

This will set the alpha value of each tail segment to ``1 - tail_decay`` times the alpha value of the previous segment, giving an exponential fade.


Periodic boundary conditions
----------------------------
The ``tail_max_segment_length`` parameter is useful if you plan on drawing tails of agents that move in a periodic space.
In such a case, agent positions may jump aprubtly when crossing a boundary.
Ordinarily, this would lead to the tail segment going across the whole domain.

To avoid this, set the ``tail_max_segment_length`` parameter to half the domain size; this typically suffices to detect jumps in x- or y- position and leads to these segments not being drawn.
(To be precise, the length refers not to that of the segment but to the differences in x- and/or y-position.)


Adjusting the plot domain
-------------------------
Instead of using the ``PlotHelper`` to adjust the x- and y-extent, we recommend using the ``domain/extent`` key; this way,
marker size will be kept constant. This key also allows setting the padding to the border of the domain, the height, and
the aspect of the domain:

.. code-block:: yaml

  agents_in_domain:
    domain:
      extent: [0, 4, 1, 10]
      pad: 0.5
      height: 3  # height in data units
      aspect: 2  # can also be 'auto' if height is not given


The ``extent`` entry should be a tuple of the form ``(left, right, bottom, top)``.
Alternatively, a 2-tuple will be interpreted as ``(0, right, 0, top)``.

Finally, you can also add a ``mode`` key to the ``domain`` dictionary to control automatic deduction of boundaries.
The domain mode can be ``auto``, ``manual``, ``fixed``, and ``follow``.
In ``fixed`` mode, all available data is inspected to derive domain bounds.
In ``follow`` mode, boundaries are adjusted from frame to frame.
In ``auto`` mode, will use a ``fixed`` domain if *no* ``extent`` was given, otherwise ``manual`` mode is used.
