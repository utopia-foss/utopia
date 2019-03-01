Utopia Configuration FAQs
=========================

This part of the FAQs are concerned with all questions around the configuration of Utopia, its models, plots, and frontend.
It does not cover the general aspects of YAML as a configuration language.

.. contents::
   :local:
   :depth: 1

----

In my model, how can I read entries from the configuration?
-----------------------------------------------------------

To extract parameters for your model from the configuration, you can use the ``Utopia::get_as`` function and the corresponding shortcuts:

.. code-block:: c++

    auto my_double = get_as<double>("my_double", cfg);
    auto my_str = get_as<std::string>("my_str", cfg);
    auto my_int = get_as<int>("my_int", cfg);
    auto my_uint = get_<unsigned int>("my_int", cfg);

One way of remembering the order of arguments is: "``get_as`` an object of type ``double``: the entry ``my_double`` from this ``cfg`` node".

See below for the function signature and more information.

.. note::

  The ``Utopia::as_`` function is deprecated in favour of ``Utopia::get_as``.

.. doxygenfunction:: Utopia::get_as



How can I read linear algebra objects (vectors, matrices, ...) from the config?
-------------------------------------------------------------------------------

For linear algebra operations, the `Armadillo <http://arma.sourceforge.net/>`_ library is a dependency of Utopia.
For example, the ``SpaceVec`` used in ``Utopia::Space`` and related classes is such a type.

The config utilities provide two convenience functions to extract data in this fashion:

.. code-block:: c++

    auto some_pos = get_as_SpaceVec<2>("initial_pos", cfg);
    auto some_midx = get_as_MultiIndex<2>("some_midx", cfg);

For information on these functions and the return types, see below.

To load data into custom ``arma`` objects, you will have to use an intermediate object that can be used to construct them. Please refer to the `Armadillo Documentation <http://arma.sourceforge.net/docs.html#Col>`_, where the supported constructors are specified.
For example, an ``arma::vec`` (double-valued column vector of arbitrary size) can be constructed from an ``std::vector``, so you would use ``get_as<std::vector>`` and then invoke the ``arma::vec`` constructor.
Note that for the ``::fixed`` vectors, not all these constructors are available; items need to be assigned element-wise for such cases.

.. note::

  We are aware that the current implementation is somewhat inconsitent.
  The aim is to implement overloads for ``get_as`` that automatically extract
  the desired type and (if it is a fixed-size object) take care of assigning
  elements.

.. doxygenfunction:: Utopia::get_as_SpaceVec
.. doxygenfunction:: Utopia::get_as_MultiIndex
.. doxygentypedef:: Utopia::SpaceVecType
.. doxygentypedef:: Utopia::MultiIndexType
