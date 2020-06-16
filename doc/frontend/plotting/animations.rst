.. _plot_animations:

Animations
==========
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
            dpi: 254
          grab_frame: {}  # passed to Writer.grab_frame and from there to savefig

      animation_update_kwargs:  {} # passed to the animation update function


.. note::

    For high-resolution plots, e.g. from a cellular automaton state, take care to choose a high-enough ``dpi`` value.
    Otherwise, you might get interpolation issues.
