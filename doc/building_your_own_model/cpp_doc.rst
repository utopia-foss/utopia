
.. _cpp_docs:

C++ documentation
-----------------

Aside from this documentation, there also is a `Utopia C++ documentation <../doxygen/html/index.html>`_ that documents the backend.
Go ahead and click on the link! 😃

By clicking on the above link, you will arrive at the doxygen documentation. It looks different, but is 

----

This looks super complicated -- how can I even read this??
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If unfamiliar with it, doxygen documentations often appear overwhelming. A few general hints on how to get a grip on it:

* In doxygen, code that belongs together is sorted into `modules <../doxygen/html/modules.html>`_. There, you will get an overview over the different parts of the Utopia backend and what they are used for.
* In the modules, read the **detailed description** before continuing to read with anything else.
* Functions and classes of the same name may have many different template signatures, thus producing multiple entries in the documentation. Don't despair: the first sentence of the function documentation should give a good summary of what this specific specialization does.

As always, the **search functionality** of both the doxygen documentation and your browser are a real help here.

.. hint::

  You can use the doxygen documentation to get **a look at the code** itself.
  For that, navigate to the `file list <../doxygen/html/files.html>`_ in the navigation bar.
  This *can* be useful if some function is not documented to a sufficient level to answer the question you have.


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
