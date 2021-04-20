.. _utopya_default_batch_cfg:

Default Batch Configuration
===========================
The file included below provides default values for setting up the :ref:`batch` framework, i.e. the :py:class:`utopya.batch.BatchTaskManager`.

It is similar to :ref:`utopya_base_cfg` which is the basis for the :py:class:`~utopya.multiverse.Multiverse` :ref:`meta-configuration <feature_meta_config>`.
Same as with the meta-configuration, the batch configuration defaults are subsequently updated as described :ref:`here <batch_cfg>`.

----

.. literalinclude:: ../../python/utopya/utopya/cfg/btm_cfg.yml
   :language: yaml


.. hint::

    This default configuration is meant to be self-documenting, thus allowing to see which parameters are available.
    If in doubt, refer to the individual docstrings, e.g. of the :py:class:`~utopya.workermanager.WorkerManager` or :py:class:`~utopya.reporter.WorkerManagerReporter`.
