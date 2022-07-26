.. _plot_style:

Customising plot styles
=======================

.. admonition:: Summary \

  On this page, you will see how to

  * use configuration inheritance and YAML anchors to globally define figure and font sizes
  * use configuration inheritance and YAML anchors to globally define colors and color cyclers
  * define and use discrete and continuous colormaps from the configuration
  * globally define the style of your plots
  * base your plots on matplotlib or seaborn styles
  * base your plot style on a custom RC file
  * use the ``PlotHelper`` to set titles, labels, scales, and more


It can be a laborious task to customise plots and figures, especially when they are intended for publication.
However, it becomes easy when using a configuration-based approach. You can define figure and font sizes,
define colors and colormaps, set axis labels and titles, legend styles, grid line widths, and more,
all in a single location, and automatically have these be applied to all your plots.
**When using configuration inheritance and YAML anchors, appearance and style settings need only be defined once**.
And since Utopia includes Latex support, it becomes easy to produce plots that integrate seamlessly into your
tex document.

Example: adjusting figure and font sizes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Let us look at a simple example: you may wish for your plots to have the same width and font size
as the documents in which you are including them. To do this, define the following base style in your plot configuration:

.. code-block:: yaml

    _my_custom_style:
      style:
        text.usetex: True
        mathtext.fontset: 'stix'
        figure.figsize: [7.516, 3.758]   # figure width and height in inches
        font.family: 'serif'
        font.size: 10
        axes.titlesize: 10

Then, simply base your plot on ``_my_custom_style``:

.. code-block:: yaml

    my_plot:
      based_on:
        - _my_custom_style
        - # other base configurations ...

Remember: entries are updated recursively in the order in which they are listed. If entries listed after ``_my_custom_style``
set styles that are also set in ``_my_custom_style``, the entries from ``_my_custom_style`` will be overwritten.
You can make use of this when individual plots differ only in a few aspects from the global style.
Imagine you want your plot to conform to ``_my_custom_style`` in everything but the figure size.
Instead of writing out the entire entry again, you can do this:

.. code-block:: yaml

    my_small_plot:
      based_on:
        - _my_custom_style
        - # other base configurations ...
      style:
        figure.figsize: [3.758, 3.758]  # This will overwrite the figure.figsize key from _my_custom_plot

Since the ``figsize`` is redefined in ``my_small_plot``, this last entry takes precedent. All the other properties
of ``_my_custom_style`` remain in place.

Hierarchical plot configuration inheritance is very useful, especially when used in tandem with YAML anchors.
For exmaple, you may want some plots to have full page width, some plots to only have half a page width, etc.
These widths can be defined globally using **YAML anchors** and used throughout the plots config:

.. code-block::

    _sizes:
      page_width: &full_width 7.516
      half_width: &half_width 3.758

    a_large_plot:
      style:
        figure.figsize: [*full_width, *half_width]
      # ...

    a_small_plot:
       style:
        figure.figsize: [*half_width, *half_width]

Now, if you want to change the image widths relative to your page width, it is sufficient to change the
definitions in ``_sizes`` – they will automatically be applied to all plots based on that style.

.. hint::

    Plot configuration entries starting with an underscore are ignored by the plot manager.
    This can be useful when defining YAML anchors that are used in the actual configuration entries.

An alternative and entirely equivalent method is to define some additional base plots and use a 'double hierarchy':

.. code-block:: yaml

    _my_custom_style:
      style:
        # some style arguments

    _large_size:
      based_on: _my_custom_style
      style:
        figure.figsize: [*full_width, *half_width]

    _small_size:
      based_on: _my_custom_style
      style:
        figure.figsize: [*half_width, *half_width]

    # This plot will have full page width
    my_large_plot:
      based_on:
        - _large_size
        - # others ...

    # This plot will have half page width
    my_small_plot:
      based_on:
        - _small_size
        - # others ...

Whichever way you choose, you will only need to set the figure widths once.

Colors and color cyclers
^^^^^^^^^^^^^^^^^^^^^^^^

YAML anchors and base configurations are also very useful when using your own custom colors; you can define a
global color palette, and use these definitions throughout your configuration:

.. code-block:: yaml

    _pretty_colors:
      blue: &blue '#0099CC'
      green: &green teal
      yellow: &yellow [1, 0.8, 0.4]
      red: &red r

    my_pretty_plot:
      # define your plot

      color: *red

Assuming ``my_pretty_plot`` takes a ``color`` kwarg, it will use the color you defined. You can define colors using any
way permitted by the `matplotlib specification <https://matplotlib.org/stable/tutorials/colors/colors.html>`_.
A change in the color definitions will then automatically be applied to all plots.

When creating several plots (possibly in a single figure), you may need to cycle through a given list of colors.
To do this, use the ``axes.prop_cycle`` key, like so:

.. code-block:: yaml

    _my_style:
      style:
        axes.prop_cycle: cycler('color', ['#AFD8BC', '#FFCC66', '#006666'])
    my_plot:
      based_on:
        - _my_style
        - # ...

The colors in your plot will then cycle through the colors you specify. If you want the
cycler to be based on a predefined color palette, use `fstrings <https://docs.python.org/3/tutorial/inputoutput.html>`_
to avoid having to define colors multiple times:

.. code-block:: yaml

    # Define your color palette
    _colors &colors:
      red: '#CC3333'
      green: '#339999'
      blue: '#0099CC'

    # Define a style
    _style:
      axes.prop_cycle: !format
        fstr: "cycler('color', ['{colors[red]:}', '{colors[green]:}', '{colors[blue]:}'])"
        colors: *colors

.. _colormaps:

Colormaps
^^^^^^^^^

Creating custom colormaps is simple with dantro's `ColorManager <https://dantro.readthedocs.io/en/latest/plotting/plot_functions.html#colormanager-integration>`_; whenever you pass a ``cmap`` argument,
it will automatically be passed to the ``ColorManager``, which takes care of the construction. For instance, you can create
a continuous colormap using six colors like this:

.. code-block:: yaml

    cmap:
      continuous: true
      from_values:
        0: crimson
        0.2: gold
        0.4: mediumseagreen
        0.6: teal
        0.8: skyblue
        1: midnightblue

And voilà, a wonderfully colorful plot emerges:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/scatter_2d_colorful.pdf
  :width: 800
  :alt: A beautifully colorful plot

The keys of the ``from_values`` dictionary are the locations of the colors you define
in the colormap, and must be values between 0 and 1. You can pass as many colors as you
like. And you can also pass additional arguments, such as setting a ``bad`` color, the limits via ``vmin`` and ``vmax``,
setting colors that are out of range (``under``/``over``), or passing a ``place_holder`` color
for ``None`` values.
See `this entry <https://dantro.readthedocs.io/en/latest/api/dantro.plot.utils.html#dantro.plot.utils.color_mngr.ColorManager>`_
for the full documentation.

You can add a norm to the ``cmap`` by passing a ``norm`` dict; for example

.. code-block:: yaml

    cmap:
      # as above ...

    norm:
      name: BoundaryNorm
      boundaries: [0, 30, 60, 90, 200]
      ncolors: 4

will return discrete bounds. You can use any of the
`matplotlib norms <https://matplotlib.org/stable/tutorials/colors/colormapnorms.html>`_,
e.g. a logarithmic norm (``name: LogNorm``).

For discrete colormaps, simply drop the ``continuous`` argument and pass a dict of colors:

.. code-block:: yaml

    cmap:
      from_values:
        susceptible: olive
        infected: crimson
        recovered: teal

Again, take a look at the `dantro documentation entry <https://dantro.readthedocs.io/en/latest/plotting/plot_functions.html#colormanager-integration>`_ for a full overview.

Lastly of course, you can simply pass the name of a
`matplotlib <https://matplotlib.org/stable/tutorials/colors/colormaps.html>`_
or `seaborn <https://seaborn.pydata.org/tutorial/color_palettes.html>`_ colormap:

.. code-block:: yaml

    cmap: "Paired" # calls the seaborn 'Paired' color palette

.. hint::

    When using the *BoundaryNorm* together with one of the pre-registered colormaps
    (e.g., *viridis*), use the ``lut`` argument (see :py:func:`matplotlib.cm.get_cmap`)
    to resample the colormap to have *lut* entries in the lookup table.
    Set ``lut = <BoundaryNorm.ncolors>`` to use the full colormap range.



Using matplotlib or seaborn stylesheets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``style`` keyword sets the ``matplotlib.rcParams`` of your plot, and all keys are interpreted
directly as ``rcParams``. You can set these yourself, or use `predefined matplotlib or seaborn stylesheets <https://matplotlib.org/stable/gallery/style_sheets/style_sheets_reference.html>`_
using the ``base_style`` key; here is an example based on ``ggplot``:

.. code-block:: yaml

    my_ggplot:
      style:
        base_style: ggplot
        lines.linewidth : 3
        lines.markersize : 10
        xtick.labelsize : 16
        ytick.labelsize : 16
      # ...

The ``ggplot`` style is applied and subsequently modified with a custom linewidth, marker size, and label sizes.
For the ``base_style`` entry, choose the name of a
`matplotlib stylesheet <https://matplotlib.org/stable/gallery/style_sheets/style_sheets_reference.html>`_.
For valid RC parameters, see the
`matplotlib customization documentation <https://matplotlib.org/stable/tutorials/introductory/customizing.html>`_.

Using a custom ``rcParams.yml`` file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to share styles across models, you can also create a ``rcParams.yml`` file containing all your style
settings, and include it like so:

.. code-block:: yaml

    my_plot:
      style:
        base_style: ~ # Add a base style if you wish
        rc_file: absolute/path/to/rcfile
        # other rcParameters here ...

Remember that entries are overwritten recursively: this means that, in the above example, the RC parameters from your file
will overwrite the entries of any ``base_style`` you provide, but parameters coming *after* the ``rc_file`` entry will
again overwrite your RC file entries.

.. note::

    You must provide an **absolute** path to the RC file.

.. _plot_helper:

Using the ``PlotHelper``
^^^^^^^^^^^^^^^^^^^^^^^^

.. hint::

    This is just a short introduction to the ``PlotHelper`` s functionality.
    Read the full article on the ``PlotHelper`` `here <https://dantro.readthedocs.io/en/latest/plotting/plot_helper.html>`_.

With the ``PlotHelper``, you can easily make further adjustments to your plot, including setting the
title, axis labels, and axis scales:

.. code-block:: yaml

    my_plot:
      # define your plot ...
      helpers:
        set_title:
          title: My over-designed phase diagram
        set_labels:
          x: 'This is the x-axis'
          y:
            label: 'And this is the y-axis'
            labelpad: 5

.. hint::

    Of course you can also use latex in your labels:

    .. code-block:: yaml

        helpers:
          set_labels:
            x: $\alpha$
            y: $\beta$

You could add some lines:

.. code-block:: yaml

    helpers:
      set_hv_lines:
        hlines:
          - pos: 0.05
            color: teal
            label: 'Just a horizontal line'
          - pos: 0.01
            color: crimson
            linestyle: 'dotted'
            label: "Livin' in a lonely world"

    # Let's also add a legend:
    set_legend:
      loc: 'best'

Or annotate some noteworthy points in the plot:

.. code-block:: yaml

    helpers:
      annotate:
        annotations:
          - xy: [ 0.08, 0.16 ]
            xycoords: data
            text: Here is the maximum!
            xytext: [ 0.05, 0.1 ]
            arrowprops:
              facecolor: skyblue
              shrink: 0.05
              linewidth: 0
              alpha: 0.5
            bbox:
              facecolor: skyblue
              linewidth: 0
              alpha: 0.5
              boxstyle: round

The struggle with matplotlib ticks is finally over:

.. code-block:: yaml

    helpers:
      set_ticks:
        x:
          major:
            locs: [0, 0.1, 0.2, 0.3, 0.4]
            labels: ['No', 'more', 'trouble', 'with', 'tick labels']

.. image:: ../../../_static/_gen/SEIRD/universe_plots/helper_demo.pdf
  :width: 800
  :alt: PlotHelper demo

These are just some of the possibilities of the ``PlotHelper``. Happy Plotting!
