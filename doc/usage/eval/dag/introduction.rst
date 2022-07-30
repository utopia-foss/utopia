
.. _dag_intro:

Data Transformation Framework
=============================
A key idea of Utopia's evaluation pipeline (implemented in :py:mod:`dantro`) is to decouple the processes of

* loading simulation data,
* selecting and transforming it,
* and finally visualizing it.

By separating the individual tasks, each part of the evaluation pipeline can focus on what it does best; read more about the philosophy behind this `in the dantro documentation <https://dantro.readthedocs.io/>`_.
The data transformation framework plays a crucial role in that, forming the bridge between data loaded into the :ref:`DataManager <utopya_data_manager>` and the :ref:`plotting framework <plotting>`.

Essentially, the transformation framework represents operations on data as a dependency tree, a **directed acyclic graph (DAG)**.
That's also where its nickname, *DAG framework* comes from.
The nodes in these DAGs are so-called *data operations* (like ``add``, ``sqrt``, ``np.mean``) of which there are `many <https://dantro.readthedocs.io/en/latest/data_io/data_ops_ref.html>`_ (and you can also :ref:`register your own <custom_DAG_ops>`.
The edges of the DAG represent dependencies on the *results* of other nodes.

Subsequently, the DAG can represent an arbitrary computation tree.
Combined with features like `file caching <https://dantro.readthedocs.io/en/latest/data_io/transform.html#the-file-cache>`_ or `meta operations <https://dantro.readthedocs.io/en/latest/data_io/transform.html#meta-operations>`_, this is a powerful tool to bridge the gap between loading simulation data and visualizing it.

At this point, that's all that needs to be said.
For more concrete usage examples, continue reading in the :ref:`plotting tutorial <eval_plotting>` where the DAG framework is used extensively.
For an in-depth introduction into the DAG framework, have a look at the corresponding `dantro documentation page <https://dantro.readthedocs.io/en/latest/data_io/transform.html>`_.
