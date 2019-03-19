
Model Requirements
==================

Below, you find a few requirements that your code should fulfill in order to be merged into the ``master`` branch of *Utopia*.

Trust Is Good – Control Is Better
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Your code seems to work fine? We are convinced, it does!
But with automatic tests you can objectively proof to the *Utopia* community that it really does what you want it to do!
This will not only assert correct functionality, but it will also make it easier to maintain.
For example, if some parts of *Utopia* that you rely on change, a breaking change will lead to a failing test – subsequently, either the change can be revoked or your model can be adjusted to still work after the change.

So: Have you added tests to check whether the different parts of your model really work?

Everyone wants to really understand what you built, including yourself!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The code is commented. Here, three aspects are important: 

  #. Write the comments for a stranger (could be you in a year), who needs to understand what the code does, and 
  #. Don't comment obvious actions that one can extract by just reading the code itself, but rather explain the underlying ideas
  #. Check whether you have overwritten, deleted or adapted the comments referring to the ``CopyMe`` model

* Your code is well-readable

  * It need not adhere to a specific style guide, but it should be consistently formatted and well-readable, e.g.: have reasonably named variables, be wrapped at 80 columns, ...

* You have filled the associated model documentation (see ``CopyMe.rst``) with content, so that everyone understands what the code does and what your thoughts and motivations behind the modeling decisions were.


There are many reasons to require the above from you and your model. We just want to emphasize two of them:

#. We understand your ideas and your model more easily. Therefore, it is easier to maintain, reuse and expand your model.
#. Perhaps more importantly: *You* understand better what you are designing and building because you constantly need to reflect on your decisions.
