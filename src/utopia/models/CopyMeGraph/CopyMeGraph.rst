
``CopyMeGraph`` - A good place to start with your graph-based model
===================================================================

This is an *example* for the documentation of your model implementation. 
Use your own creativity and structure to convey the important information about your model in a nice and catchy way.

**Important:** Before your model can be merged into the master, the following steps are required:

1. Fill in this documentation file
2. ``git mv`` this file to the ``doc/models`` directory. Make sure it is named exactly as your model is named.
3. After the move, make sure the link in the :ref:`Default Model Configuration` section points to your model configuration.

For the reStructuredText syntax, see `here <http://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html>`_, but there are also some examples below.


Model Fundamentals
------------------

Here, you can add some description of the fundamental ideas of your model and the questions it potentially can address.

A subsection
^^^^^^^^^^^^

Here, important subaspects of the model can be addressed. Feel free to use math blocks:

.. math::

   W = \begin{pmatrix}w_{00} & w_{01} \cr w_{10} & w_{11} \end{pmatrix}

…or some inline math, if you like: :math:`\sigma = 42`.

Implementation Details
----------------------

This section provides a few important implementation details...

You can use some short code blocks to convey some information. Adjust the language to your need (``c++``, ``python``, ...)

.. code-block:: yaml

   # A YAML code block
   # My parameters
   foo:
     bar: baz
   
   spam: 42


Default Model Configuration
---------------------------

Below are the default configuration parameters for the ``CopyMeGraph`` model.

.. literalinclude:: ../../src/utopia/models/CopyMeGraph/CopyMeGraph_cfg.yml
   :language: yaml
   :start-after: ---

.. todo::  Make sure the path is correct here.


Possible Future Extensions
--------------------------

You have a nice idea, but

* that would open a completely new storyline and thus would need the creation of a new model that is based on this one.
* you just did not have the time to explore everything, however, it would probably be a good idea to look into this in future work.

Well... write it down here, maybe someone picks it up.


References
----------

* Adams, Douglas (1979). *A Hitchhiker's Guide to the Galaxy*. Pan books.
