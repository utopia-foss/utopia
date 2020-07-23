.. _api_paramspace:

API Reference: :py:mod:`paramspace`
===================================
This page includes an *extract* of the API reference of the :py:mod:`paramspace` package, which is used throughout :py:mod:`utopya`, the Utopia frontend.
In its own description, the package does the following:

    .. automodule:: paramspace
        :members:

In everyday usage within Utopia, the :py:mod:`paramspace` objects are most easily defined directly inside the configuration files by using the specified YAML tags.

.. contents::
    :local:
    :depth: 2

----


The :py:class:`ParamSpace`
--------------------------
Available YAML tags: ``!pspace``

.. autoclass:: paramspace.paramspace.ParamSpace
    :members:


Parameter Dimensions
--------------------
The following classes are used to defined parameter dimensions inside the parameter space.


The :py:class:`~paramspace.ParamDimBase`: A common base class
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. autoclass:: paramspace.paramdim.ParamDimBase
    :members:


The :py:class:`~paramspace.ParamDim`: A proper parameter dimension
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Available YAML tags: ``!pdim`` and ``!sweep``

.. autoclass:: paramspace.paramdim.ParamDim
    :members:
    :show-inheritance:


The :py:class:`~paramspace.CoupledParamDim`: attached to another dimension
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Available YAML tags: ``!coupled-pdim`` and ``!coupled-sweep``

.. autoclass:: paramspace.paramdim.CoupledParamDim
    :members:
    :show-inheritance:


