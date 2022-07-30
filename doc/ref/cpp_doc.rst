.. _cpp_docs:

About the C++ Documentation
===========================

Aside from this user manual, there also is a C++ documentation that documents the backend API: the ``utopia/core`` and ``utopia/data_io`` libraries, all things you need to :ref:`implement your own model <impl>`.


→ To the `📚 C++ Documentation <../../doxygen/html/index.html>`_ ←
------------------------------------------------------------------

Click on the above link to go to the C++ documentation.


Tips & Tricks
-------------

You might think to yourself: *This looks super complicated -- how can I even read this??*

If unfamiliar with it, `doxygen <https://www.doxygen.nl/index.html>`_ documentations often appear overwhelming.
A few general hints on how to get a grip on it:

* In doxygen, code that belongs together is sorted into `modules <../../doxygen/html/modules.html>`_.
  There, you will get an overview over the different parts of the Utopia backend and what they are used for.
* In the modules, read the **detailed description** before continuing to read with anything else.
* Functions and classes of the same name may have many different template signatures, thus producing multiple entries in the documentation.
  Don't despair: the first sentence of the function documentation should give a good summary of what this specific specialization does.

As always, the **search functionality** of both the doxygen documentation and your browser (typically: CMD+F or CTRL+F) are a real help here.

.. hint::

    You can use the doxygen documentation to get **a look at the code** itself.
    For that, navigate to the `file list <../../doxygen/html/files.html>`_ in the navigation bar.
    This *can* be useful if some function is not documented to a sufficient level to answer the question you have.
