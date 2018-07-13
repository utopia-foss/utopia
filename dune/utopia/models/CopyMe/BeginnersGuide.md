## A Beginner's Guide for Setting Up a New Model

This is the beginner's guide for creating a new model in _Utopia_. If you go through all the steps, you will end up with a model that can profit from all _Utopia_ features and can do ... basically nothing yet. It is a starting point for your own expedition in the _Utopia_ model world. You will be the one who afterwards defines the rules, the entities etc. of this world. But before this fun part can start, the framework needs to be set up...

To avoid problems, go chronologically through the following sections.

### Setting Up The Infrastructure

We assume that you want to build your own model which will be called `MyFancyModel`. Probably, you will give it a more suited name. So, keep in mind to replace every instance of `MyFancyModel` with you actual model name. If you follow all the steps explained in the following, you will end up with your own utopia model.

- [ ] Copy the `CopyMe` directory inside of the model directory and paste it inside of the model directory. Rename the directory for example to `MyFancyModel`. 
(Remark to the naming convention: Your model name should consist of words that start with Capital Letters and are `DirectlyConcatenatedWithoutSeparatingSymbols`)
- [ ] Rename all files such that all `CopyMe`s are replaced by `MyFancyModel`s
- [ ] Tell _Utopia_ that there is a new model, e.g. include your model in the Utopia build routine (based on `CMake`):
    - [ ] Within the `utopia/dune/utopia/models/` directory you find a `CMakeLists.txt` file. Open it and let `CMake` find your model directory via the command: `add_subdirectory(MyFancyModel)` 
    - [ ] Within the `utopia/dune/utopia/models/MyFancyModel/` directory, there is another `CMakeLists.txt` file. Open it and change the line `add_model(CopyMe CopyMe.cc)` to `add_model(MyFancyModel MyFancyModel.cc)`. With this command you tell `CMake` that you include a new model should be added.
- [ ] Open the file `MyFancyModel.cc` in the `utopia/dune/utopia/models/MyFancyModel/` directory and do the following:
  - [ ] Throughout the file replace all `CopyMe`'s by `MyFancyModel`'s.
- [ ] Open the file `MyFancyModel.hh` in the `utopia/dune/utopia/models/MyFancyModel/` directory and do the following:
  - [ ] Throughout the file replace all `CopyMe`'s by `MyFancyModel`'s.
  - [ ] Throughout the file replace all `COPYME`'s by `MYFANCYMODEL`'s.


- [ ] Set up the testing framework
  - [ ] Move to the directory `utopia/python/model_tests`, copy the `CopyMe` directory and rename it to `MyFancyModel`. Be sure that there is a file named `__init__.py` inside the directory. 



- [ ] Set up the plotting framework
  - [ ] Move to the directory `utopia/python/model_plots`

### Adapting your code 
Depending on what model you want to implement, you probably can delete some provided functions that are commented out.
- [ ] All variables, functions, etc. that are just there to show how you would use and implement them are denoted with the prefix 'some_' or '_some', e.g. '_some_variable', 'some_function', 'some_interaction', ...
If you write your model, you should change these.

- [ ] Feel free to remove anything, you do not need.
### Remarks to Building a Model

### Further reading
If you want to know more about how to actually build a model you can have a look at the following rather simple models:

1. SimpleEG: A simple evolutionary game model with cells on a grid
2. MovingAgents: A simple model with agents that move on a grid
3. ...

You can have a look at these models and modularly select what you want to have included in your model, if needed.


Congratulations, you have build a new model! :) So now, just one thing remains:

- [ ] Delete the `BeginnersGuide.md` - you do not need it anymore and anything that you do not need anymore in your model should not be part of the model anymore!

Your next guide will be the `MyModelWorksGuide.md` guide. It contains information what requirements your code must fulfill such that it can be accepted as a model within _Utopia_, e.g. that it can be merged into _Utopia_'s master branch. 

Have fun implementing your model! :) 


## Additional notes 
- how to use spdlog