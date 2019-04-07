State animation function
========================

.. deprecated::

  This method is deprecated! Use :func:`utopya.plot_funcs.ca.state` instead,
  and see here (:doc:`ca`) for more information.


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
                state:
                    title: State

    ``limits``: The limits of the colormap of the subplot.

        .. code-block:: yaml

            to_plot:
                state:
                    limits: [0, 1]

Example Plot Configuration Snippet
----------------------------------

This snippet can be copy-pasted into a plot configuration file. 
The fictional parameter names need to be replaced by model-specific ones.
Comments indicate where changes need to be applied.

.. code-block:: yaml

    ### Example snippet for a CA time-series plot with two states

    # The name of the plot. Replace with the desired name!
    state0_and_state1:
        creator: universe
        universes: all

        file_ext: mp4

        module: .ca
        plot_func: state_anim

        # Replace this name by the actual model name!
        model_name: MyFancyModel 

        # Select the properties to plot
        to_plot:
            # The name of the property to plot with its options
            # Replace this name by the actual state name!
            state0:
                title: State 0
                cmap: 
                    set: blue
                    empty: white

            # The name of the property to plot with its options
            # Replace this name by the actual state name!
            state1:
                title: State 1
                limits: [0, 1]
                cmap: viridis

        # Video options
        writer: ffmpeg
        fps: 2
        dpi: 96
