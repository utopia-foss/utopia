.. _api_paramspace:

paramspace package
==================

This page includes an *extract* of the API reference of the :py:mod:`paramspace` package, which is used throughout :py:mod:`utopya`, the Utopia frontend.
In its own description, the package does the following:

    .. automodule:: paramspace
        :members:

In everyday usage within Utopia, the :py:mod:`paramspace` objects are most easily defined directly inside the configuration files by using the specified YAML tags.

Refer to paramspace's own `documentation <https://paramspace.readthedocs.io/en/latest/>`_ for more detailed information.

.. contents::
    :local:
    :depth: 2

----


The :py:class:`~paramspace.paramspace.ParamSpace` class
-------------------------------------------------------
Available YAML tags: ``!pspace``

.. autoclass:: paramspace.paramspace.ParamSpace
    :members:


Parameter Dimensions
--------------------
The following classes are used to defined parameter dimensions inside the parameter space.


The :py:class:`~paramspace.paramdim.ParamDimBase`: A common base class
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. autoclass:: paramspace.paramdim.ParamDimBase
    :members:


The :py:class:`~paramspace.paramdim.ParamDim`: A proper parameter dimension
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Available YAML tags: ``!pdim`` and ``!sweep``

.. autoclass:: paramspace.paramdim.ParamDim
    :members:
    :show-inheritance:


The :py:class:`~paramspace.paramdim.CoupledParamDim`: attached to another dimension
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Available YAML tags: ``!coupled-pdim`` and ``!coupled-sweep``

.. autoclass:: paramspace.paramdim.CoupledParamDim
    :members:
    :show-inheritance:



The :py:mod:`paramspace.yaml` module
------------------------------------

.. automodule:: paramspace.yaml
    :members:
    :show-inheritance:
    :undoc-members: yaml



The :py:mod:`paramspace.tools` module
-------------------------------------

.. automodule:: paramspace.tools
    :members:
    :show-inheritance:
