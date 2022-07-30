
.. _debug_DAG:

Debugging DAG computations
==========================

As you saw throughout the :ref:`plotting tutorial <eval_plotting>`, the :ref:`data transformation framework <plot_with_DAG>` can be a very powerful tool to prepare data for plotting.

But what about the case where this goes wrong?
**How can the DAG be debugged?**

This page presents some approaches on how to address errors in DAG computations:

.. contents::
    :local:
    :depth: 1

----

Read the error log
------------------
This is the first step towards understanding what's going on.
The data transformation framework aims to make error messages as understandable and helpful as possible.

Let's look at some examples.

Invalid operation name
^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
    :language: yaml
    :dedent: 2
    :start-after: ### START --- debug_DAG_bad_op_name
    :end-before: ### END --- debug_DAG_bad_op_name

Here, we are trying to select some data from the :ref:`SEIRD model <model_SEIRD>` but have used an operation name (``square``) that does not exist.
Creating a plot with this operation will definitely fail and generate an error message like this:

.. code-block::

    BadOperationName: Could not find an operation or meta-operation named 'square'!

    No operation 'square' registered! Did you mean: squared, sqrt ?
    Available operations:
      !=                                       .
      .()                                      .T
      .all                                     .any
      .argmax                                  .argmin
      .argpartition                            .argsort
      .assign                                  .assign_attrs

      . . .

From the error message, it's quite clear what's going on: We need to choose the correct operation name.
We also get a list of available operations and even get *suggestions* for similar names – and we can just follow those: Using ``squared`` instead of ``square`` will solve our problems.

.. admonition:: Controlling exceptions in the plotting framework
    :class: dropdown

    Error messages in the plotting framework are typically caught and re-raised by the plotting framework, resulting in ``PlotCreatorError`` messages like this:

    .. code-block::

        PlotCreatorError: An error occurred during plotting with
        UniversePlotCreator for 'debug_DAG_bad_op_name'!
        To ignore the error message and continue plotting with the other
        plots, specify `debug: False` in the plot configuration or disable
        debug mode for the PlotManager.

    This also tells us how to control whether an error will be raised or not:

    .. code-block:: yaml

        my_plot:
          debug: true   # raise an error and stop plotting

    As usual throughout Utopia, the CLI ``--debug`` flag also controls this behavior.



Failing operation
^^^^^^^^^^^^^^^^^
The case of an operation *failing* is a bit trickier, as it depends on what the operation does in particular.
Let's look at an example where we pass a wrong argument to an operation:

.. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
    :language: yaml
    :dedent: 2
    :start-after: ### START --- debug_DAG_bad_args
    :end-before: ### END --- debug_DAG_bad_args

The error output from that will be something like the following:

.. code-block::

    DataOperationFailed: Operation '.sel' failed with a KeyError, see below!
    It was called with the following arguments:
      args:
         0:  <xarray.DataArray 'densities' (time: 151, kind: 8)>
    array([[0.5968, 0.4032, 0.    , ..., 0.    , 0.    , 0.    ],
           [0.5968, 0.4032, 0.    , ..., 0.    , 0.    , 0.    ],
           [0.5968, 0.4016, 0.0012, ..., 0.    , 0.    , 0.    ],
           ...,
           [0.5968, 0.    , 0.    , ..., 0.    , 0.    , 0.    ],
           [0.5968, 0.    , 0.    , ..., 0.    , 0.    , 0.    ],
           [0.5968, 0.    , 0.    , ..., 0.    , 0.    , 0.    ]])
    Coordinates:
      * time  (time) int64 0 1 2 3 4 5 6 7 8 ... 143 144 145 146 147 148 149 150
      * kind  (kind) <U11 'empty' 'susceptible' 'exposed' ... 'source' 'inert'

         1:  {'kind': 'SuSCePTIble'}

      kwargs:  {}

    KeyError: 'SuSCePTIble'

What can we learn from that message?

* Operation ``.sel`` failed, so we know *where* the error occurred.
* We got a ``KeyError`` for the given key ``SuSCePTIble``
* We see the arguments that were passed to ``.sel``, marked as positional ``args`` 0 and 1 ... and the given :py:class:`xarray.DataArray` does not have a key ``SuSCePTIble`` in the ``kind`` coordinate dimension!

From that we can deduce: The key actually has to be ``susceptible``.

Now this was comparably straight-forward, but you get the idea.

.. hint::

    In the data operation above, we have added the ``.data`` operation, which resolves the previous object from a :py:class:`utopya.eval.containers.XarrayDC` into a regular :py:class:`xarray.DataArray` object.
    This makes debugging much easier because it shows the actual content of the array.


.. _debug_DAG_vis:

Look at the DAG visualization
-----------------------------
What about a case where it's harder to locate where an error comes from, e.g. if there are multiple operations.

.. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
    :language: yaml
    :dedent: 2
    :start-after: ### START --- debug_DAG_locate_with_vis
    :end-before: ### END --- debug_DAG_locate_with_vis

Again, one is obviously wrong here, but because there are many ``.sel`` operations, it's not immediately clear which one.

Let's have a look at the terminal log again.
You may have noticed already earlier, that something like the following is printed alongside the error:

.. code-block::

    NOTE     base              Creating DAG visualization (scenario: 'compute_error') ...
    NOTE     dag               Generating DAG representation for 7 tags ...

    . . .

    CAUTION  base              Created DAG visualization for scenario 'compute_error'. For debugging, inspecting the generated plot and the traceback information may be helpful: ... dag_compute_error.pdf
    ERROR    plot_mngr         An error occurred during plotting with UniversePlotCreator ...

Here, the plotting framework automatically created a visualization of the DAG to help with debugging.
It calls this scenario a ``compute_error``, because that's what happened:
The DAG computation failed and that's why such a visualization is created.
The log also tells you where the file was saved to, typically it ends up right beside where the plot should have been created.

Let's look at the generated DAG visualization:

.. image:: ../../../_static/_gen/SEIRD/universe_plots/debug_DAG_locate_with_vis_dag_compute_error.pdf
    :target: ../../../_static/_gen/SEIRD/universe_plots/debug_DAG_locate_with_vis_dag_compute_error.pdf
    :width: 800
    :alt: A DAG visualization in a failed scenario

This tells us a lot:

- The light red node is where the operation failed, while computing the ``infected`` tag.
- Subsequently, the ``xr.Dataset`` operation in the end cannot be carried out.
- The remaining node colors show which operations succeeded and which ones were only prepared for computation but not actually carried out.

The DAG visualization is a powerful way of understanding what is going on and if the DAG structure is actually the way you expected it to be.

The visualization feature is controlled via the ``dag_visualization`` entry of your plot configuration.
Read more about this feature and available parameters `in the dantro docs <https://dantro.readthedocs.io/en/latest/plotting/plot_data_selection.html#plot-creator-dag-vis>`_.

.. admonition:: How to always create a DAG visualization

    By default, DAG visualizations are only created if the DAG computation failed for whatever reason.

    To *always* create a DAG visualization, regardless of that, inherit the ``.dag.vis.always`` base configuration.

    .. code-block:: yaml

        my_plot:
          based_on:
            # ...
            - .dag.vis.always
            # ...

    .. admonition:: ... which translates to ...
        :class: dropdown

        .. code-block:: yaml

            my_plot:
              dag_visualization:
                enabled: true
                when:
                  always: true
                  only_once: true


.. admonition:: DAG visualization with multiverse plots

    For multiverse plots, DAG visualization may not generate a usable figure, given the potentially large number of nodes.
    In such cases it makes sense to temporarily :ref:`restrict the plot to a subspace <plot_subspaces>`:

    .. code-block:: yaml

        my_multiverse_plot:
          select_and_combine:
            subspace:
              some_dim: [0, 1]
              another_dim: [foo, bar]

    Ideally, use the same dimensionality as in the case you want to debug.

.. hint::

    Node positioning drastically improves with `pygraphviz <https://github.com/pygraphviz/pygraphviz>`_ installed in the ``utopia-env``.

.. warning::

    DAG visualization is only available *after the DAG has been fully constructed*.
    If you make errors during construction, like setting a tag multiple times, the visualization will not be able to help you.


.. _debug_DAG_print:

Print debug information
-----------------------
Even with the DAG visualization helping us in understanding the DAG structure, we may need to probe the computation progress in more detail.

To the rescue: The *good ol'* ``print`` operation!

Basically, you can insert the ``print`` operation at any point between two operations to probe the state in between.
A few examples:

.. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
    :language: yaml
    :dedent: 4
    :start-after: ### START --- debug_DAG_print_examples
    :end-before: ### END --- debug_DAG_print_examples

As you see, there are many different ways of doing this.
Choose the one that suits you best, the most important aspect is to get the ``print`` in there.

.. note::

    In case the computation is actually succeeding but does not have the expected results, you can elicit an error simply by adding a dependent operation that *fails*:

    .. code-block:: yaml

        transform:
          # ...
          - print
          - .mean: [!dag_prev , [foo, bar]]  # operation to probe
          - print
          - raise                            # operation doesn't exist -> error

The underlying function for ``print`` is actually not Python's :py:func:`print` , but :py:func:`dantro.data_ops.ctrl_ops.print_data`, which has further capabilities:

.. literalinclude:: ../../../_cfg/SEIRD/universe_plots/eval.yml
    :language: yaml
    :dedent: 4
    :start-after: ### START --- debug_DAG_advanced_print_examples
    :end-before: ### END --- debug_DAG_advanced_print_examples



Further approaches
------------------
If the above does not help in isolating the error, there are a bunch of other things you can try:

* Check the definition of a failing operation in the `operations database <https://dantro.readthedocs.io/en/latest/data_io/data_ops_ref.html>`_ and how certain operations are defined.

  * dantro-based operations are documented in :py:mod:`dantro.data_ops`
  * Documentation for operations that perform method calls (like ``.mean`` does) need to be checked in the respective package documentation.
    For instance, this may be :py:meth:`xarray.DataArray.mean`, :py:meth:`xarray.Dataset.mean`, :py:meth:`pandas.DataFrame.mean`, or other packages' ``.mean`` methods.

* Have a look at the `dantro DAG troubleshooting section <https://dantro.readthedocs.io/en/latest/data_io/data_ops.html#troubleshooting>`_.
* In case the error appears not during computation but in the plot function, check the format (dimensionality, shape, coordinate labels etc.) in which the plot function expects to receive data.
* If all of that does not work out, you can try to create a toy example in an interactive python session to find out how the objects behave.


Open an issue and ask for help
------------------------------
If all of the above approaches did not succeed, we are more than happy to assist.
Feel free to open an issue in the `Utopia GitLab project <https://gitlab.com/utopia-project/utopia/-/issues/new>`_.

For bug reports or suggestions to improve the DAG framework, we are welcoming your feedback in the `dantro GitLab project <https://gitlab.com/utopia-project/dantro/-/issues/new>`_.
