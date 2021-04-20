
Writing Documentation
=====================

User Manual
-----------

The user manual (including the page you are currently reading) is generated with `Sphinx <http://www.sphinx-doc.org/en/master/>`_
using the `reStructuredText syntax <http://docutils.sourceforge.net/rst.html>`_.
Here, you will find some examples that help you write your own documentation entry (take a look at the "page source" to see the code behind the text).

Examples
^^^^^^^^
Tables of contents are implemented like this:

.. contents::
   :depth: 2

Notes, warnings, and hints can be included thusly:

.. note::

    This is a note.

.. warning::

    This is a warning.

.. hint::

    This is a hint.

Math and Formulae
"""""""""""""""""
Mathematical expressions can be included like so:

Let :math:`x \rightarrow \infty`, then

.. math::

    \lim_{x \rightarrow \infty} \frac{1}{x} = 0

Code snippets and blocks
""""""""""""""""""""""""
Code snippets like ``std::algorithm`` or ``cell_manager`` are indicated like this. Code blocks can be included like this:

    .. code-block:: python

        def id(x):
            return x

Python Documentation Bits
"""""""""""""""""""""""""

The typical use case of Sphinx is rendering a documentation of Python modules.

.. autoclass:: utopya.multiverse.Multiverse
    :noindex:

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

.. _cpp_doc:

C++ Documentation
-----------------
The C++ documentation is created with
`Doxygen <https://www.stack.nl/~dimitri/doxygen/manual/index.html>`_.
If you want to contribute to Utopia, you will need to write documentation, too.
For this, we provide :ref:`coding guidelines <coding_guidelines>`, as
well as the additional information below.


How to write the Doxygen documentation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
`Doxygen <https://www.stack.nl/~dimitri/doxygen/manual/index.html>`_
is a neat and widely used tool to document C++ source code.
The documentation is based on ``docstrings`` that are attached to the actual
source code they should document using the comment syntax of the respective
programming language.

Whenever you write a new function, class, and even when you only add a new
class member or method, write at least a ``\brief`` description, such that this
object can later be referred to via the documentation. Notice that you can
also add ``docstrings`` to a certain object *anywhere* in the source code,
if need be.

Using Doxygen Modules
^^^^^^^^^^^^^^^^^^^^^
Documentations for large code bases quickly become confusing. By default,
Doxygen groups documentation pages by class names and namespaces. However,
a certain set of classes typically closely interact within one, or even across
several, namespaces. For documenting these groupings and interactions, Doxygen
implements `Modules <https://www.stack.nl/~dimitri/doxygen/manual/grouping.html>`_.

Somewhere in the source code, one may define a module using
``\defgroup <label> <title>``, or ``\addtogroup <label>`` together with a set
of brackets ``{ ... }``. Single objects can be added to an existing group with
``\ingroup <label>``. Modules themselves can be nested, i.e., a module
can be grouped into another module.

For documenting modules, we use `modules.dox` files inside the respective
source directory. Doxygen always uses the C++ syntax, therefore all
documentation inside these files must have the syntax of a C++ comment.


Python Documentation
--------------------
When developing utopya or any other part of the Utopia frontend, you will also have to write documentation of the Python code. For this, please consult and follow the :ref:`coding guidelines <coding_guidelines>`.
