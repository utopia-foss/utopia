
Contribute
----------
This documentation is generated with `Sphinx <http://www.sphinx-doc.org/en/master/>`_ 
using the `reStructuredText syntax <http://docutils.sourceforge.net/rst.html>`_. 
Here, you find some examples that help you write your own documentation entry.

Examples
^^^^^^^^
Here, you find a few examples on how to write the Sphinx documentation.

.. contents::
   :depth: 2

A loose collection of directives and their usage.

.. note::

    This is an extremely dumb page!

.. warning::

    I mean **really** dumb!

Math and Formulae
"""""""""""""""""

Let :math:`x \rightarrow \infty`, then

.. math::

    \lim_{x \rightarrow \infty} \frac{1}{x} = 0

Very smart!

Python Documentation Bits
"""""""""""""""""""""""""

The typical use case of Sphinx is rendering a documentation of Python modules.

.. autoclass:: utopya.Multiverse

Doxygen Documentation Bits
""""""""""""""""""""""""""

Wondering what the class template ``Cell`` looks like? Well, here you go:

.. doxygenclass:: Utopia::Cell
   :members:
   :protected-members:
   :private-members:
   :undoc-members:

The Logging Module
""""""""""""""""""

This is a documentation of the entire ``Logging`` module:

.. doxygengroup:: Logging
   :members:
