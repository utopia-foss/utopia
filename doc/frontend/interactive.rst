Interactive Scripting
=====================

Like with any other Python project, you can execute the scripts of ``utopya``
interactively. This is especially useful for digging through the data generated
by your model.

.. contents::
   :local:
   :depth: 2


Setting Up the Interactive Session
----------------------------------

Before starting an interactive Python session, you will have to :ref:`activate
the virtual environment <activate_venv>`. Then, run the Python executable
without arguments. Alternatively, you can run an instance of `IPython`_
from your *host* environment inside the virtual environment (beware though that
this may have undesired side-effects as opposed to installing IPython into
the virtual environment).

.. code-block:: bash

    # run Python executable ...
    python
    # ... or IPython if available on host system
    ipython

Now load the ``utopya`` module:

.. code-block:: python

    import utopya

You're good to start your interactive session now!


Common Use Cases
----------------

An overview of use cases for an interactive session.

Loading a Multiverse
^^^^^^^^^^^^^^^^^^^^
If you already executed your simulation and want to load the data
interactively, you will need to instantiate a ``FrozenMultiverse``. It takes
the name of your model as argument. If you do not specify any other parameters,
the object will load the output directory of that model with the latest time
stamp.

After instantiating the multiverse, you can grab its ``DataManager`` and ask
it to load the data according to the configuration files in the output
directory. You can then access the data of any universe via the ``multiverse``
key.

This is the entire procedure:

.. code-block:: python

    from utopya import FrozenMultiverse

    # load the latest multiverse of <MyModel> ...
    mv = FrozenMultiverse(model_name="<MyModel>")
    # ... or a specific multiverse by providing its path
    mv = FrozenMultiverse(model_name="<MyModel>",
                          run_dir="<path>/<MyModel>/<time_stamp>")

    # get the data manager and load all data
    dm = mv.dm
    dm.load_from_cfg(print_tree=True) # print the data tree after loading

    # iterate over all universes and access the data
    for uni in dm["multiverse"].items():
        data = uni["data"]
        # ...

For more information, you can consult the (abbreviated) documentation of the
two classes used in this example:

.. autoclass:: utopya.FrozenMultiverse
    :members: dm, pm, meta_cfg

    .. automethod:: __init__

.. autoclass:: utopya.DataManager
    :members: load_from_cfg, load

    .. automethod:: __init__


.. _IPython: https://ipython.org/install.html
