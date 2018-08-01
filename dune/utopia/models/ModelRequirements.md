# Model Requirements
Below, you find a few requirements that your code should fulfill in order to be merged into the `master` branch of _Utopia_.


### Trust Is Good – Control Is Better
Your code seems to work fine? We are convinced, it does!
But with automatic tests you can objectively proof to the _Utopia_ community that it really does what you want it to do!
This will not only assert correct functionality, but it will also make it easier to maintain.
For example, if some parts of _Utopia_ that you rely on change, a breaking change will lead to a failing test – subsequently, either the change can be revoked or your model can be adjusted to still work after the change.

So...
- [ ] Have you added tests to check whether the different parts of your model really work?

### Everyone wants to really understand what you built, including yourself!

- [ ] The code is commented. Here, three aspects are important: 
    1.  Write the comments for a stranger (could be you in a year), who needs to understand what the code does, and 
    2.  Don't comment obvious actions that one can extract by just reading the code itself, but rather explain the underlying ideas
    3. Check whether you have overwritten, deleted or adapted the comments refering to the `CopyMe` model
- [ ] Your code is well-readable
    - It need not adhere to a specific style guide, but it should be consistently formatted and well-readable, e.g.: have reasonably named variables, be wrapped at 80 columns, ...
- [ ] You have filled the `README.md` with content, so that everyone understands what the code does and what your thoughts and motivations behind the modeling decisions were.

There are many reasons to require the above from you and your model. We just want to emphasize two of them:

1. We understand your ideas and your model more easily. Therefore, it is easier to maintain, reuse and expand your model.
2. Perhaps more importantly: _You_ understand better what you are designing and building because you constantly need to reflect on your decisions.
