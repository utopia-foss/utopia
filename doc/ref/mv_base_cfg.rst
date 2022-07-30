.. _mv_base_cfg:

Multiverse Base Configuration
=============================

This page documents the default values for the configuration of the :py:class:`~utopya.multiverse.Multiverse` class, a central element for running simulations using Utopia.
The configuration is meant to contain all information that is needed to run a simulation; have a look at :ref:`run_config` for a more detailed motivation.

For higher flexibility, the configuration is built up from a number of individual layers and can be adjusted at multiple points.
Below, you will both find details of the update scheme as well as the default values for certain configuration layers.
See :ref:`config_hierarchy` for more information.

.. contents::
    :local:
    :depth: 1

----


.. _utopya_mv_base_cfg:

utopya ``Multiverse`` base configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The utopya ``Multiverse`` base configuration provides a large number of defaults and, at the same time, is meant to be self-documenting, thus allowing to see which parameters are available.

.. toggle::

    .. literalinclude:: ../_inc/utopya/utopya/cfg/base_cfg.yml
       :language: yaml


.. _utopia_mv_base_cfg:

Utopia ``Multiverse`` configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Utopia's ``Multiverse`` base configuration recursively updates the above.
It adjusts and expands the configuration to specialize for use with Utopia as a modelling framework:

* Put output into ``~/utopia_output``.
* Configure HDF5 data loading, because all Utopia models need that.
* Set defaults within ``parameter_space`` that are expected on C++ side.

For Utopia, this configuration (at ``python/utopia_mv_cfg``) takes the place of the *framework configuration* in the update scheme.

.. toggle::

    .. literalinclude:: ../../python/utopia_mv_cfg.yml
       :language: yaml
