.. _utopya_base_cfg:

utopya Multiverse
=================
The following is the base configuration of the :py:class:`utopya.Multiverse` class.
It provides all *defaults* that are needed to run a simulation, but is subsequently updated :ref:`by other configuration layers <feature_meta_config>` to form the *meta configuration*.

The base configuration is meant to be self-documenting, thus allowing to see which parameters are available.

.. note::

    The ``parameter_space`` key is extended with the default model configuration of the chosen model.
    This will lead to the default model configuration being available at ``parameter_space.<CurrentlyChosenModelName>``.

----

.. literalinclude:: ../../../python/utopya/utopya/cfg/base_cfg.yml
   :language: yaml


.. note::

    The ``parameter_space`` key is by default (!) assumed to be a :py:class:`paramspace.paramspace.ParamSpace` object.
    Defining sweep dimensions therein thus does *not* require to mark it with the ``!pspace`` YAML tag.
