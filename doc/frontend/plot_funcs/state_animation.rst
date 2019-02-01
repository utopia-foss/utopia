State animation function
========================

.. autofunction:: utopya.plot_funcs.ca.state_anim


Additional Parameter Information
--------------------------------
to_plot : dict
    ``c_map``: It is possible to set the colormap by setting the ``cmap`` key. This can be done in two ways:
    
        1. use the name of a ``matplotlib`` colormap such as 'viridis'::

        .. code-block:: yaml

            to_plot:
                # The name of the property to plot with its options
                state:
                    cmap: 'viridis'

        2. provide a dictionary from which a discrete colormap is derived. The keys correspond to the labels, and the values define the color::
            
        .. code-block:: yaml

            to_plot:
                # The name of the property to plot with its options
                state:
                    cmap:
                        state_1: blue
                        state_2: green
                        state_3: gray
    
    ``title``: The title of the subplot.

        .. code-block:: yaml

            to_plot:
                # The name of the property to plot with its options
                state:
                    title: State

    ``limits``: The limits of the colormap of the subplot.

        .. code-block:: yaml

            to_plot:
                # The name of the property to plot with its options
                state:
                    limits: [0, 1]