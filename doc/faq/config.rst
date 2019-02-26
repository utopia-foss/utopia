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

To extract parameters for your model from the configuration, you can use the ``Utopia::get_`` function and the corresponding shortcuts.

.. doxygenfunction:: Utopia::get_

Example usage:

.. code-block:: c++

    auto my_double = get_double("my_double", cfg);
    auto my_str = get_str("my_str", cfg);
    auto my_int = get_int("my_int", cfg);
    auto my_uint = get_<unsigned int>("my_int", cfg);

One way of remembering the order of arguments is: "``get`` the ``double`` called ``my_double`` from the following ``cfg`` node".

For more information and available shortcuts, consult the doxygen documentation.

.. note::

  The ``Utopia::as_`` function is deprecated in favour of ``Utopia::get_``.
