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

To extract parameters for your model from the configuration, you can use the ``Utopia::get_as`` function and the corresponding shortcuts.

.. doxygenfunction:: Utopia::get_as

Example usage:

.. code-block:: c++

    auto my_double = get_as<double>("my_double", cfg);
    auto my_str = get_as<std::string>("my_str", cfg);
    auto my_int = get_as<int>("my_int", cfg);
    auto my_uint = get_<unsigned int>("my_int", cfg);

One way of remembering the order of arguments is: "``get_as`` an object of type ``double``: the entry ``my_double`` from this ``cfg`` node".

For more information and available shortcuts, consult the doxygen documentation.

.. note::

  The ``Utopia::as_`` function is deprecated in favour of ``Utopia::get_``.
