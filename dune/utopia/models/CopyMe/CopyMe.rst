
``CopyMe`` - The basis for your new model
=========================================

**Important:** This is an example for the documentation of your model implementation. Before your model can be merged into the master, the following steps are required:

* Fill in this documentation file
* ``git mv`` this file to the ``doc/models`` directory. Make sure it is named exactly as your model is named.

For the reStructuredText syntax, see `here <http://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html>`_, but there are also some examples below.

This template ``CopyMe.rst`` is just a proposition, but you can use your own creativity and structure to convey the important information about your model in a nice and catchy way. Feel free to add or delete new sections.

Fundamentals
------------

Here, you can add some description of the fundamental ideas of your model and the questions it potentially can address.

Subchapter of the Fundamentals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here, important subaspects of the model can be addressed. Feel free to use math blocks:

.. math::

   W = \begin{pmatrix}w_{00} & w_{01} \cr w_{10} & w_{11} \end{pmatrix}

…or some inline math, if you like: :math:`\sigma = 42`.

Implementation Details
----------------------

This section provides a few important implementation details.

.. code-block:: yaml

   # A YAML code block
   # My parameters
   foo:
     bar: baz
   
   spam: 42


More Conceptual and Theoretical Background
------------------------------------------

Models normally are based on a lot of theory. Here, you can mention or explain some of it. Further, you can refer to important papers and/or books.

Theory of Template Models
^^^^^^^^^^^^^^^^^^^^^^^^^

I am actually not so sure, if there is a lot of theory behind this...

Theory of Everything
^^^^^^^^^^^^^^^^^^^^

42 [Adams 1979]

Possible Future Extensions
--------------------------

You have a nice idea, but

#. that would open a completely new storyline and thus would need the creation of a new model that is based on this one.
#. you just did not have the time to explore everything, however, it would probably be a good idea to look into this in future work.

Well... write it down here, maybe someone picks it up.


References
----------

* Adams, Douglas (1979). *A Hitchhiker's Guide to the Galaxy*. Pan books.
