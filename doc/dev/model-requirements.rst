.. _dev_model_requirements:

Model Requirements
==================

If your code seems to work, and you want to have it merged into the master branch, there are a few requirements it needs to fulfill before that can happen. These are

**1. Your code should be thoroughly tested, and the tests need to pass without warnings.** This will not only assert correct functionality, but it will also make it easier to maintain. For example, if some parts of Utopia that you rely on change, a breaking change will lead to a failing test. The change can then subsequently either be revoked, or your model can be adjusted to still work after the change.

**2. Your code needs to be commented, thorougly.** Here, three aspects are important:

- Write the comments as you would for a stranger (who could be you in a year), who needs to understand what the code does.
- Don't comment obvious functionality that one can deduce by just reading the code itself, but rather explain the *underlying ideas*.
- Check whether you have overwritten, deleted, or adapted the comments referring to the ``CopyMeGrid``, ``CopyMeGraph``, or ``CopyMeBare`` models respectively.

**3. Your code should be readable.**

- Please adhere to the :ref:`coding_guidelines`.

- Make sure you have filled the model documentation template (see ``CopyMe<Grid/Graph/Bare>.rst``) with content, so that everyone can understand what the code does and what your thoughts and motivations behind the modelling decisions were.

Among others, these requirements will help us understand your ideas and your model, making it easier to maintain, reuse, and expand. But perhaps more importantly: *you* will be able to better understand what you are designing and building, because adhering to these guides will push you to constantly reflect on your implementation decisions.
