.. _data_handling:

Handling Data
=============

In this section we will take a look at utopya's capabilities to handle simulation data.

.. contents::
   :local:
   :depth: 2

----

.. _utopya_data_manager:

The :py:class:`~utopya.eval.datamanager.DataManager`
----------------------------------------------------
Objects of this class are the home of all your simulation data.
One such :py:class:`~utopya.eval.datamanager.DataManager` object is initialized together with the :py:class:`~utopya.multiverse.Multiverse` and thereafter available as :py:attr:`~utopya.multiverse.Multiverse.dm` attribute. It is set up with a :ref:`load configuration <data_manager_load_cfg>` and, upon invocation of its :py:meth:`~dantro.data_mngr.DataManager.load_from_cfg` method, will load the simulation data using that configuration. It is equipped to handle hierarchical data, storing it as a data tree.

.. hint::

    To visually inspect the tree representation, you can use the :py:attr:`~dantro.data_mngr.DataManager.tree` property: ``print(dm.tree)``.
    This also works with every group-like member of the tree.

This functionality is all based on the `dantro package <https://dantro.readthedocs.io/en/stable/>`_, which provides a uniform interface to handle hierarchically structured data.
However, while the interface is uniform, the parts of the data tree can be adapted to ideally handle the underlying data.

*One* example of a specialization is the :py:class:`~utopya.eval.containers.GridDC` class, which is a specialization of a data container that represents data from a grid.
It is tightly coupled to the data *output* of Utopia on C++ side, where the most efficient way to write data is along the *index* of the entities, rather than the x and y coordinates.
However, to *handle* that data, one expects data with the dimensions ``x``, ``y``, and ``time``; the :py:class:`~utopya.eval.containers.GridDC` takes care of reshaping that data in this way.

.. _data_handling_proxy:

Handling Large Amounts of Data
------------------------------
To handle large amounts of simulation data (which is not uncommon), the :py:class:`~utopya.eval.datamanager.DataManager` provides so-called *proxy loading* for HDF5 data:
instead of loading the data directly into memory, the structure and metadata of the HDF5 file is used only to generate the data tree.
At the place where normally the data would be stored in the data containers, a proxy object is placed (in this case: :py:class:`~dantro.proxy.hdf5.Hdf5Proxy`).
Upon access to the data, the proxy gets automatically resolved, leading to the data being loaded into memory and replacing the proxy object in the data container.

Objects that were loaded as proxy are marked with ``(proxy)`` in the tree representation.
To load HDF5 data as proxy, use the ``hdf5_proxy`` loader in the :ref:`data_manager_load_cfg`.

These proxy objects already make handling large amounts of data much easier, because the data is only loaded if needed.


.. _data_handling_load_parallel:

Loading files in parallel
^^^^^^^^^^^^^^^^^^^^^^^^^
Despite data being loaded into proxy objects, this process can take a considerable amount of time if there are many groups or datasets in the to-be-loaded HDF5 files.
In such a case, the ``DataManager`` will be busy mostly with creating the corresponding Python objects, and less so with loading the actual data from the files.
(In other words, this would be a task that is CPU-limited, not I/O limited.)

Subsequently, there is a benefit in using multiple CPUs to build the data tree in such scenarios.
The dantro data loading interface supports parallel loading and Utopia allows to control this behavior directly via the CLI:

.. code-block:: bash

    utopia eval MyModel --load-parallel

The above command will enable parallel loading and it will use all available CPUs for that; see the CLI ``--help`` for details.

If you want more control, you can also directly configure it via the :ref:`meta-configuration <feature_meta_config>`.
Have a look at the corresponding section in the :ref:`utopya_base_cfg` for available options, e.g. for using parallel loading depending on the number of files or their total file size:

.. literalinclude:: ../../../python/utopia_mv_cfg.yml
    :language: yaml
    :start-after: # Options for loading data in parallel
    :end-before: # --- End of load configuration
    :dedent: 6

.. hint::

    The ``parallel`` option is basically available for every entry in the ``data_manager.load_cfg``.
    However, given the constant overhead of starting new loader processes, it makes most sense for the ``data`` entry, where the HDF5 files' content is loaded.



.. _data_handling_dask:

How about *huge* amounts of data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
There will be scenarios in which the data to be analyzed exceeds the limits of the physical memory of the machine.
Here, proxy objects don't help, as they only postpone the loading.

For that purpose, :py:mod:`dantro`, which heavily relies on :py:mod:`xarray` for the representation of numerical data, is able to make use of its `dask <https://dask.org>`_ integration.
The dask package allows working on chunked data, e.g. HDF5 data, and only loads those parts that are necessary for a calculation, afterwards freeing up the memory again.
Additionally, it does clever things by first building a tree of operations that are to be performed, then optimizing that tree, and only when the actual numerical result is needed, does the data need to be loaded.
Furthermore, as the data is chunked, it can potentially profit from parallel computation.
More on that can be found :ref:`here <xarray:dask>`.

To use dask when loading Utopia data, arguments need to be passed to the proxy that it should not be resolved as the actual data, but as a dask representation of it. This is done by setting the ``resolve_as_dask`` flag.
Arguments can be passed to the proxy by adding the ``proxy_kwargs`` argument to the configuration of a data entry.
Add the following part to the root level of your run configuration, which will update the :ref:`defaults <data_manager_load_cfg>`:

.. code-block:: yaml

    data_manager:
      load_cfg:
        data:
          proxy_kwargs:
            resolve_as_dask: true

    parameter_space:
      # ... your usual arguments

.. note::

    When plotting data via ``utopia eval``, you can also specify a run configuration. Check the ``utopia eval --help`` to find out how.

Once this succeeded, you will see ``proxy (hdf5, dask)`` in the tree representation of your loaded data.

There are two other ways to set this entry (following Utopia's :ref:`configuration hierarchy principle <run_config>`):

    * In the CLI, you can additionally use the ``--set-cfg`` argument for ``utopia eval`` and ``utopia run`` to set the entry:

    .. code-block:: console

        utopia eval MyModel --set-cfg data_manager.load_cfg.data.proxy_kwargs.resolve_as_dask=true

    * To permanently set this entry, you can write it to your *user* configuration:

    .. code-block:: console

        utopia config user --get --set data_manager.load_cfg.data.proxy_kwargs.resolve_as_dask=true

    This then applies to *all* models you work with. As dask does slow down some operations, it only makes sense to set this if you are mostly working with large data and tend to forget enabling dask!



Configuration and API Reference
-------------------------------

.. _data_manager_load_cfg:

Default Load Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^
Below, the default :py:class:`~utopya.eval.datamanager.DataManager` configuration is included, which also specifies the default load configuration.
Each entry of the ``load_cfg`` key refers to one so-called "data entry".
Files that match the ``glob_str`` are loaded using a certain ``loader`` and placed at a ``target_path`` within the data tree.

.. literalinclude:: ../../_inc/utopya/utopya/cfg/base_cfg.yml
   :language: yaml
   :start-after: # Data Manager ...
   :end-before: # The resulting data tree is then
