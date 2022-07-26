.. _custom_DAG_ops:

Implementing your own DAG operations
====================================

.. admonition:: Summary

    On this page, you will see how to

    * make your own DAG operations available using the :py:func:` ~utopya.eval.transform.is_operation` decorator or :py:func:`~utopya.eval.transform.register_operation` function.

    See also the `full article <https://dantro.readthedocs.io/en/latest/data_io/data_ops.html?registering-operations>`_ in the dantro docs on registering custom operations.

As documented `in the overview of DAG operations <https://dantro.readthedocs.io/en/latest/data_io/data_ops_ref.html>`_, the data transformation framework has access to a plethora of standard operations you can use for your data analysis.
However, you may need a special operation for your own purposes.
In such a situation, you can include a custom operation and register it using the :py:func:`~utopya.eval.transform.is_operation` decorator.

Add something like the following to your model-specific plot module:

.. testcode:: register-operations

    """model_plots/MyModel/__init__.py"""

    # Your regular imports here

    # Import the is_operation decorator ...
    from utopya.eval import is_operation

    # ... and register your operation from some custom callable
    @is_operation("MyModel.my_operation")
    def my_operation(data, *, some_parameter):
        """Some operation on the given data"""
        # ... do something with data and the parameter ...
        return new_data


Of course, custom operations can also be defined somewhere else within your plot modules, e.g. in an ``operations.py`` file, and imported into ``__init__.py`` using ``from .operations import my_operation``.

Note that you should ideally not override existing operations.
To avoid naming conflicts, it is advisable to **use a unique name for custom operations**, e.g. by prefixing the model name for some model-specific operation as done in the example above.

**Important:** Your model-specific custom operations should be defined in the model-specific plot module, i.e.: accessible after importing ``model_plots/<your_model_name>/__init__.py``.
Prior to plotting, the :py:class:`~utopya.eval.plotmanager.PlotManager` pre-loads that module, such that the ``register_operation`` calls and the decorator definitions are actually invoked.

You can also use the :py:func:`~utopya.eval.transform.register_operation` function to register pre-existing operations, e.g. from numpy:

.. testcode:: register-operations

    import numpy as np
    from utopya.eval import register_operation

    # Register custom operations from some imported module ...
    register_operation(name="np.mean", func=np.mean)

    # ... or from a lambda
    register_operation(name="my_custom_square", func=lambda d: d**2)


Examples
^^^^^^^^

There is are some example implementions in the
`Utopia SEIRD model <https://gitlab.com/utopia-project/utopia/-/blob/master/python/model_plots/SEIRD/operations.py>`_
and the `Utopia Opinionet model <https://gitlab.com/utopia-project/utopia/-/blob/master/python/model_plots/Opinionet/data_ops.py>`_.
