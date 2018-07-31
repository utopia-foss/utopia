## A Beginner's Guide for Setting Up a New Model

This is the beginner's guide for creating a new model in _Utopia_. If you go through all the steps, you will end up with a model that can profit from all _Utopia_ features and can do ... basically nothing interesting yet. It is a starting point for your own expedition in the _Utopia_ model world. You will be the one who afterwards defines the rules, the entities etc. of this world. But before this fun part can start, the framework needs to be set up...

To avoid problems, go chronologically through the following sections.

### Setting Up The Infrastructure

We assume that you want to build your own model, called `MyFancyModel`. Probably, you will give it a more suited name. So, keep in mind to replace every instance of `MyFancyModel` with your actual chosen model name. If you follow all the steps explained in the following, you will end up with your own utopia model.

- [ ] Move to the `utopia/dune/utopia/models/` directory, copy the `CopyMe` directory paste it in the same directory. Rename the copied directory to `MyFancyModel` (or rather your chosen name). 
A remark concerning the naming convention: Your model name should consist of words that start with Capital Letters and are `DirectlyConcatenatedWithoutSeparatingSymbols`.
- [ ] Rename all the files inside of the newly created directory such that all `CopyMe`s are replaced by `MyFancyModel`s
- [ ] Tell _Utopia_ that there is a new model, e.g. include your model in the Utopia build routine (based on `CMake`):
    - [ ] Within the `utopia/dune/utopia/models/` directory you find a `CMakeLists.txt` file. Open it and let `CMake` find your model directory by including the command: `add_subdirectory(MyFancyModel)` 
    - [ ] Within the `utopia/dune/utopia/models/MyFancyModel/` directory, there is another `CMakeLists.txt` file. Open it and change the line `add_model(CopyMe CopyMe.cc)` to `add_model(MyFancyModel MyFancyModel.cc)`. With this command you tell `CMake` that you include a new model should be added.
- [ ] Open the file `MyFancyModel.cc` in the `utopia/dune/utopia/models/MyFancyModel/` directory and do the following:
  - [ ] Throughout the file replace all `CopyMe`'s by `MyFancyModel`'s.
- [ ] Open the file `MyFancyModel.hh` in the `utopia/dune/utopia/models/MyFancyModel/` directory and do the following:
  - [ ] Throughout the file replace all `CopyMe`'s by `MyFancyModel`'s.
  - [ ] Throughout the file replace all `COPYME`'s by `MYFANCYMODEL`'s.

#### The Testing Framework
- [ ] Set up the testing framework
  - [ ] Move to the directory `utopia/python/model_tests`, copy the `CopyMe` directory and rename it to `MyFancyModel`. Be sure that there is a file named `__init__.py` inside the directory. 
  - [ ] Inside the created `MyFancyModel` directory, rename the `test_CopyMe.py` file to `test_MyFancyModel.py`.
  - [ ] Open the `test_MyFancyModel.py` file and replace all `CopyMe`'s by `MyFancyModel`'s.

In this `test_MyFancyModel.py` file you can add tests to your model. 

ATTENTION: Keep in mind to remove the provided example tests if you remove unneeded parts of the former `CopyMe` model. Otherwise, you will get error messages if you run the model.

#### The Plotting Framework
- [ ] Set up the plotting framework
  - [ ] Move to the directory `utopia/python/model_plots`, copy the `CopyMe` directory and rename it to `MyFancyModel`. Be sure that there is a file named `__init__.py` inside the directory.

The `state.py` script is provided to show you how a model specific plotting script could look like. Remember to remove it (comment it out) if you start removing or changing parts of the former `CopyMe` model code. Otherwise, you will get error messages.

### Adapting your code 
Depending on what model you want to implement, you will need to delete or adapt some provided functions. So, feel free to remove anything, you do not need.

- [ ] All variables, functions, etc. that are just there to show how you would use and implement them are denoted with the prefix 'some_ ' or '_ some', e.g. '_ some_ variable', 'some_ function', 'some_interaction', ...
If you write your model, you should change these.

- [ ] Keep in mind to adapt the plotting and testing functions such that they belong to your model.

### Inspiration From Other Models
If you want to learn more about the capabilities of Utopia and how models can look like, we recommend that you have a look at the already implemented models, e.g.:

* `SimpleEG`: A simple evolutionary game model with cells on a grid
<!--2. MovingAgents: A simple model with agents that move on a grid-->

### Some Final Remarks and Advices

#### `log->debug` instead of `std::cout`

If you are used to writing `C++` code you probably often use `std::cout` to print information or to debug your code. We advice you to use the functionality of `spdlog` if you work with _Utopia_. This has at least two advantages:

1. If you run your model, your information is stored in a `out.log` for each universe, so you can have a look at the logger information later.
2. If you do big parameter sweeps, your terminal will not be flooded with information.

As a rough guideline:

- Use `log->info("Some info")` for information that is not repetitive, e.g. not inside a loop, and contains rather general information.
- Use `log->debug("Some more detailed info, e.g. for helping you debug")` 

More information about how to use `spdlog` and what functionality is provided can be found [here](https://github.com/gabime/spdlog).

### Finished! 

Congratulations, you have build a new model! :) So now, just one thing remains:

- [ ] Delete the `BeginnersGuide.md` - you do not need it anymore and anything that you do not need anymore in your model should not be part of the model anymore!

Your next guide will be the `MyModelWorksGuide.md` guide. It contains information what requirements your code must fulfill such that it can be accepted as a model within _Utopia_, e.g. that it can be merged into _Utopia_'s master branch. 

Have fun implementing your own _Utopia_ model! :) 
