
C++ documentation
-----------------

Just have a look at the `Utopia C++ documentation <../../doxygen/html/index.html>`_. 😃

Writing documentation
^^^^^^^^^^^^^^^^^^^^^
The C++ documentation is created with 
`Doxygen <https://www.stack.nl/~dimitri/doxygen/manual/index.html>`_.
If you want to contribute to Utopia, you will need to write documentation, too.
For this, we provide :doc:`coding guidelines </guides/coding-guidelines>`, as 
well as the additional information below.


How to write the Doxygen documentation
""""""""""""""""""""""""""""""""""""""

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
"""""""""""""""""""""

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
