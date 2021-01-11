.. _model_CopyMeGrid:

``CopyMeGrid`` - A good place to start with your CA-based model
===============================================================

This template model supplies a minimal CA-based Utopia model.
On top of what the :ref:`CopyMeBare model template provides <model_CopyMeBare>`, this model contains:

* Infrastructure for ``CellManager`` setup and data writing
* A customisable ``CellState``, including a configuration-based constructor
* The accompanying model configuration and evaluation files
* A ``cfgs`` directory with further example configuration files

Use this model if you want to start "from scratch" but want to avoid setting up the infrastructure itself.

.. hint::

    Refer to :ref:`impl_step_by_step` for information on how to implement your own model based on this template.
