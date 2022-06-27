.. _plot_animations:

Animations
==========

.. admonition:: Summary \

  On this page, you will see how to

  * use ``.plot.ca`` to animate a cellular automaton
  * use the ``.animation.ffmpeg`` or  ``.animation.frames`` base functions together with ``.dag.graph``
    to animate networks
  * use the ``.animation.ffmpeg`` or  ``.animation.frames`` base functions for general animations
  * adjust the resolution of an animation using the ``writer_kwargs``
  * write your own animation function using the ``@is_plot_func`` decorator
  * write your own animation plot configuration

.. admonition:: Complete example: Animated cellular automaton
    :class: dropdown

    .. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- .plot.ca.
        :end-before: ### End --- .plot.ca

.. admonition:: Complete example: Animated network
    :class: dropdown

    .. literalinclude:: ../../../_cfg/Opinionet/graph_plot/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- animated_network
        :end-before: ### End --- animated_network

Animations can make complex dynamics much more comprehensible, and are easy to create using the plotting framework.

Animated Cellular Automaton
^^^^^^^^^^^^^^^^^^^^^^^^^^^

A particular common case is animating cellular automata, such as the SEIRD model, for which the :code:`.plot.ca`
function provides a simple interface to create an animated plot.

.. code-block:: yaml

    animated_ca:

      # Base your plot on .plot.ca
      based_on:
        - .creator.universe
        - .plot.ca

      # Select the different 'kinds' of agents to plot
      select:
        kind: kind

      # .plot.ca requires a 'to_plot' key
      to_plot:
        kind:
          limits: [~, 3]
          cmap:
            empty: white
            susceptible: lightgreen
            infected: crimson
            recovered: teal

:code:`.plot.ca` requires a :code:`to_plot` key, specifying the limits of the colormap. Here, we are only
plotting the susceptible, infected, and recovered agents, so we set the limits to :code:`[0, 3]`, and define the colors
we want for each :code:`kind`:

.. raw:: html

    <video width="600" src="../../../_static/_gen/SEIRD/universe_plots/animated_ca.mp4" controls></video>

For animations, you will typically want a fairly high plotting resolution. Change the resolution of the plot by adding an
:code:`animation` block to the plot configuration:

.. code-block:: yaml

    animated_ca:
      # all the previous entries ...

      animation:
        writer_kwargs:
          ffmpeg:
            saving:
              dpi: 400

A higher dpi will give you a higher resolution and prevent interpolation issues,
but will also take longer to plot and require more storage.

You can restrict yourself to a smaller range of frames to plot using the ``frames_isel`` key (which selects indices).
This can be useful for long simulation runs, and when only wanting to visualise a small part of the dynamics.
Simply add

.. code-block:: yaml

    animated_ca:

      # all the previous entries ...

      frames_isel: !range [30, 60]

This will only plot the frames from 30 to 60. You can also manually specify an array,
i.e. :code:`frames_isel: [10, 20, 30, 40]`.

You can also use the ``.plot.facet_grid`` base configuration with ``kind: pcolormesh``
to animate heatmaps. See the :ref:`article on heatmaps <pcolormesh>` for more details.

Animated Network Plots
^^^^^^^^^^^^^^^^^^^^^^

.. raw:: html

    <video width="800" src="../../../_static/_gen/Opinionet/graph_plot/graph_animation.mp4" controls></video>


Let's look at another example: in the :ref:`previous section <plot_networks>` we saw how to plot
networks. There, we used a node property called ``opinion`` to color the network nodes.
We can now animate them, showing how this node property changes over time.
The configuration can only requires minor modification. If you already have a
static graph plot ``static_network``, you can amend it in the following way:

.. code-block:: yaml

   static_network:
     # Plot configuration for a static network plot ...

   animated_network:
     based_on:
       - static_network
       - .animation.ffmpeg  # Use the ffmpeg writer

     # Add this entry to make the 'opinion' change over time
     graph_animation:
       sel:
         time:
           from_property: opinion

And that's it! Instead of ``ffmpeg``, you can also use the ``frames`` writer by instead basing your plot on ``.animation.frames``.
Increase the resolution of the animation by adding and updating the following entry:

.. code-block:: yaml

    animation:
      writer_kwargs:
        frames:
          saving:
            dpi: 400
        ffmpeg:
          init:
            fps: 10
          saving:
            dpi: 400

You only need to add the key for the animation writer you are actually using.

Take a look at the :ref:`Utopia Opinionet model <model_Opinionet>` for a working demo of an animated network.

Writing your own animation
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. admonition:: Idea

    Why not write an animation of the infection curve as time progresses, and show the
    result here?

Writing your own animated plot is simple with the inclusion of the ``PlotHelper`` and the :code:`@is_plot_func`
decorator. The fundamental structure of a plot function that supports animation should follow this scaffolding:
first, use the :code:`is_plot_func` decorator on your plot function:

.. code-block:: python

    from utopya import DataManager, UniverseGroup
    from utopya.plotting import UniversePlotCreator, is_plot_func, PlotHelper

    @is_plot_func(use_dag=True, supports_animation=True)
    def my_plot( *, hlpr: PlotHelper, data: dict, time: int, **plot_kwargs):

Set :code:`use_dag` and :code:`supports_animation` to :code:`True`.

Next, write your plot function. It should plot the data at a single time, and then contain an update function
that loops over the time steps, plotting a frame of the animation at each step:

.. code-block:: python

    from utopya import DataManager, UniverseGroup
    from utopya.plotting import UniversePlotCreator, is_plot_func, PlotHelper

    @is_plot_func(use_dag=True, supports_animation=True)
    def my_plot( *, hlpr: PlotHelper, data: dict, time: int, **plot_kwargs):

        hlpr.ax.plot(data[time], **plot_kwargs)

        def update():
            for idx, y_data in enumerate(data):

                # Clear the plot and plot anew
                hlpr.ax.clear()
                hlpr.ax.plot(y_data, **plot_kwargs)

                # Set the title
                hlpr.invoke_helper('set_title', title="Time {}".format(idx))

                # Done with this frame. Yield control to the plot framework,
                # which will take care of grabbing the frame.
                yield

While whatever happens before the registration of the animation function is also executed, the animation
update function should be built such as to also include the initial frame of the animation. This is to allow the
plot function itself to be more flexible, and the animation update need not distinguish between initial frame
and other frames.

Finally, register the animation with the plot helper:

.. code-block:: python

    from utopya import DataManager, UniverseGroup
    from utopya.plotting import UniversePlotCreator, is_plot_func, PlotHelper

    @is_plot_func(use_dag=True, supports_animation=True)
    def my_plot( *, hlpr: PlotHelper, data: dict, time: int, **plot_kwargs):

        # as above ...

        def update():
            # as above ...

        hlpr.register_animation_update(update)

To summarise, we

* marked the plot function as ``supports_animation``,
* defined an ``update`` function, and
* passed the ``update`` function to the helper
  via :py:meth:`~dantro.plot.plothelper.PlotHelper.register_animation_update`

Now let's look at what the ``plot_cfg.yml`` needs to contain. There are two base plot configurations
you can use: ``.animation.frames`` and ``.animation.ffmpeg``. They use different writers for the animation.
Basing your plot on either of them is sufficient for the animation to run:

.. code-block:: yaml

    my_plot:
      based_on:
        - .creator.universe
        - .animation.ffmpeg  # or .animation.frames
        - # other base settings

You can change the resolution and frame rates of the animation by adding an ``animation``
entry to the plot configuration

.. code-block:: yaml

    my_plot:
      based_on :
        - .creator.universe
        - .animation.ffmpeg
        - # ...

      module: # your module here
      plot_func: # your plot func here

      # Other settings, such as select, transform, and plot-specific arguments ...

      # Animation configuration
      animation:
        writer_kwargs:      # additional configuration for each writer
          frames:           # passed to 'frames' writer
            saving:         # passed to Writer.saving method
              dpi: 400
          ffmpeg:
            init:           # passed to Writer.__init__ method
              fps: 15
            saving:
              dpi: 400
            grab_frame: {}  # passed to Writer.grab_frame and from there to savefig


Finally, you can also pass any additional kwargs to the ``update`` function you defined by adding

.. code-block:: yaml

    my_plot:

      # same as above ...

      animation:
        animation_update_kwargs:  {}

.. hint::

    You can turn the animation off like this:

    .. code-block:: yaml

        animation:
          enabled: false

    This can be useful to avoid plotting lengthy animations for every run.
