## A Beginner's Guide for Setting Up a New Model

Chronological order!

### Setting Up The Infrastructure
- [ ] Copy the `CopyMe` directory inside of the model directory and paste it inside of the model directory. Rename the directory for example to `MyFancyModel`. 
(Remark to the naming convention: Your model name should consist of words that start with Capital Letters and are `DirectlyConcatenatedWithoutSeparatingSymbols`)
- [ ] Rename all file such that all `CopyMe`s are replaced by `MyFancyModel`s
- [ ] Tell _Utopia_ that there is a new model, e.g. include your model in the Utopia build routine (based on `CMake`):
    - [ ] Within the `utopia/dune/utopia/models/` directory you find a `CMakeLists.txt` file. Open it and let `CMake` find your model directory via the command: `add_subdirectory(MyFancyModel)` 
    - [ ] Within the `utopia/dune/utopia/models/MyFancyModel/` directory, there is another `CMakeLists.txt` file. Open it and change the line `add_model(CopyMe CopyMe.cc)` to `add_model(MyFancyModel MyFancyModel.cc)`. With this command you tell `CMake` that you include a new model should be added.

- [ ] Set up the testing framework

- [ ] Set up the plotting framework



Congratulations, you have build a new model! :) So now, just one thing remains:

- [ ] Delete the `BeginnersGuide.md` - you do not need it anymore and anything that you do not need anymore in your model should not be part of the model anymore!

Your next guide will be the `MyModelWorksGuide.md` guide. It contains information what requirements your code must fulfill such that it can be accepted as a model within _Utopia_, e.g. that it can be merged into _Utopia_'s master branch. 

Have fun implementing your model! :) 


## Additional notes 
- how to use spdlog