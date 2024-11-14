
.. _entity_selection:

Entity Selection
================

Utopia provides a config-accessible interface to select a subset of entities from a manager, e.g. cells from a ``CellManager``. This page aims to answer some basic questions regarding that interface. For a full documentation, you should consult the :ref:`cpp_docs`, e.g. by starting from the module on entity selection.

.. contents::
   :local:
   :depth: 2

----

Configuration Access
--------------------
When is ``select_entities`` accessible via the configuration?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Whenever a model documentation states that it uses the ``select_entities`` interface, it means that a configuration node is passed down into that function. You can thus make changes to the configuration file and thereby change which entities are selected.

For example, :doc:`../../models/ForestFire` uses this interface to configure heterogeneities:

.. literalinclude:: ../../../src/utopia/models/ForestFire/ForestFire_cfg.yml
   :language: yaml
   :start-after: # --- Heterogeneities
   :end-before: # --- Output

There, the ``mode`` is crucial. Only those parameters that are relevant for the chosen mode are used; the others may be present, but will be ignored.

.. note::

  The ``enabled`` key is not a part of the ``select_entities`` interface, but is implemented by the calling structure.


Which selection modes are available?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Selection modes are defined in the ``Utopia::SelectionMode`` enum.

The following is an excerpt from the C++ documentation, showing available selection modes.
For information on parameters corresponding to each mode, take a look there or at the documentation of the respective implementations.

.. doxygengroup:: SelectionModes


Selecting from the ``CellManager``
----------------------------------
Why can't I select boundary cells?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Probably because you have configured a periodic space and for periodic space it does not really make sense to be able to configure a boundary. You should also see a warning in your logs that states this.

Change your ``space`` configuration such that it reads

.. code-block:: yaml

  space:
    periodic: false
