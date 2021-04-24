.. _dev_workflow:

Development Workflow
====================

If you are unfamiliar with git, GitLab or the general workflow when working with Utopia, you have come to the right place! This guide will help you to get to know the basic tools needed to work as a developer on Utopia. In fact, these are the tools all professional software engineers use, so getting to know them is a very useful skill.

The aim of this guide is to present you with a selection of the most important features and commands you will be using most of the time. It does not intend to show you the full range of possibilities, but rather helps you immediately start exploring the Utopia world, and provides a single place for you to look up the most important commands. For this reason, we will often not go into deeper explanations about how something works. But after reading this guide, you should have a solid understanding of the basics, allowing you to explore more by yourself simply by trying things out or googling.

.. contents::
   :local:
   :depth: 2

----

Introduction
------------

Have you ever wondered how big software projects are managed? How do hundreds of people work on the same code without breaking it, although just one missing semicolon would suffice to do that?

What if you want to explore an earlier version of a project? And what if you just want to try out something, but do not know whether you want to keep the changes or throw them away?

git and GitLab offer solutions to these kinds of questions. git is a *version control system*, allowing you to keep a stable version of your code that works all the time, while you develop the code in another place. You can think of these different stages of the code as a tree, which has a root, one big central branch, and many small outgoing branches. However, in git the outgoing branches can be "merged" back again into each other, and even into the central "master" branch — tell that to a tree! git keeps track of the different branches and their version history, allowing you to easily revert to an earlier stage of your code. When you "merge" your code into another branch, your changes are automatically integrated.

For Utopia, as for any project, this means that there is always a stable version (the "master" branch), which you can download, and which is guaranteed to work. All the development, so the things people are working on separately at the same time, happens on individual branches. When a developer has completed their project, they can ask to have their work merged into the master branch, thereby making it accessible to everyone else.

A common scenario is that several people have simultaneously made changes to the same piece of code, such that your changes can no longer be automatically merged. This is called a `merge conflict`, and resolving it often requires manually selecting the changes to keep. However, more on that later.

The git workflow: Stage, Commit, Push
-------------------------------------

The workflow with Git is as follows:

1. **Create a new branch:** create your own branch on which you are free to do as you please. If you mess up, you can always revert to previous versions of the code. And if the worst comes to the worst, you can start over. The master branch is protected and will never be affected by changes on your branch.
2. **Write or edit the code on your local branch.**
3. **Stage, commit, and push your changes to the remote repository:** if you want to upload your changes to the project repository, e.g. because you want to have it looked at by or make it available to other developers, you need to stage your changes, commit them with a commit message, and push them to the remote branch.
4. **Merge other people's changes into your branch, or merge your changes into another branch.** If you wish to include updates other people have made to the master branch (or any other branch) in your own branch, you can merge these into your own branch.


Create a new branch
^^^^^^^^^^^^^^^^^^^

A simple way to create a new branch is right in the `Utopia Gitlab project <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia>`_: in the left-hand navigation pane, navigate to Repository -> Banches, and click on "New Branch". Give your branch a descriptive name, and select from where you want to branch off (typically the master branch). Click "Create branch" – and the branch is created!

Now, your local machine does not yet know that there is a new branch, and, of course, your local copy of the code repository is also not yet on this new branch. To switch branches, navigate to your ``utopia`` directory, open the terminal, and do

.. code-block:: shell
    
    git fetch
    git checkout <your_branch_name>
    
The first command fetches updates from the remote server, including the information that a new branch has been created. The second command then switches to your new branch.

.. hint::
    Installing `tab completion <https://github.com/bobthecow/git-flow-completion/wiki/Install-Bash-git-completion>`_ is extremely useful here — it makes finding and typing in branch names much easier. Now do

.. code-block:: shell

    git status
    
The first line of your output should confirm that you are now on your new branch. To return to the master branch, do

.. code-block:: shell

    git checkout master
    
.. warning:
    You can only switch branches if you do not have any unsaved changes in your local version. More on this below.

Well done! You have now created your own development branch. Utopia comes with a host of pre-implemented models for you to use and experiment with. And in the :ref:`impl_step_by_step` on how to build a model you can learn how to use these as a starting point, meaning you do not have to start from scratch when developing new models.

Now that you have created a new branch, you are ready to code and upload your changes using git. There are a lot of guides and tutorials online where you can learn how to use git. A quick start would be `this one <https://git-scm.com/book/en/v2/Getting-Started-Git-Basics>`_; a longer one can be found on `git-tower <https://www.git-tower.com/learn/git/ebook>`_. Also, the `git documentation <https://git-scm.com/doc>`_ is quite comprehensive and well-written.

Here, we provide only a small selection of commands that you will use a lot during your code development and address some frequent issues encountered when starting to use git. If you have questions or run into problems, it is always worth consulting the ``git <command> --help`` or checking out the guides linked above. Also, searching the internet for what you want to do is usually quite effective when it comes to questions with git.


Stage, commit, and push changes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To check the status of your repository, do

.. code-block:: shell

    git status
    
This will display any changes to the repository which have not yet been pushed to the GitLab project. To `stage` files that contain changes you wish to store, do

.. code-block:: shell
    
    git add <path/to/file>

If you have a lot of files and don't want to type them all out separately, you can do

.. code-block:: shell

    git add .
    
.. warning::
    By doing ``git add .``, you may inadvertently stage files you don't want to push, e.g. because you have not yet configured a **gitignore file** (more on that below). It is recommended, at least at the beginning, to check which files you have staged by again doing ``git status`` after staging. A list of staged files will then appear in your terminal, and any inadvertantly staged files can be un-staged via
    
        .. code-block:: shell
        
            git restore --staged <path/to/file>
            
To commit, do

.. code-block:: shell

    git commit -m '< ... >'
    
where ``<...>`` should contain a `commit message`, i.e. a brief description of the content of the commit. For example, a valid example would be

.. code-block:: shell
    
   git commit -m 'Implement the basic interaction mechanism'
   
Finally, to push the changes to GitLab, simply do

.. code-block:: shell

    git push
        
.. note::

  If you are wondering when to commit and how to write a good commit message, have a look at these `Version Control Best Practices <https://www.git-tower.com/learn/git/ebook/en/command-line/appendix/best-practices>`_ and this `blog post <https://jasonmccreary.me/articles/when-to-make-git-commit/>`_.

  Before you can commit anything, git prompts you to provide your name and email address with the given commands. When choosing your name, be aware that this name is immortalized in the git history (please choose your full name and a decent email address ;)).
  
.. note::
    If you dislike using the terminal, many code editors and IDEs also include a version-control interface (with buttons).

Creating a global .gitignore
""""""""""""""""""""""""""""
If, after staging some files, you run the ``git status`` command, you might see a lot of files that you have not created, e.g. ``.DS_Store``\ , ``./vscode``\. These sometimes are files created by your operating system or by your IDE. You can and should create a global ``.gitignore`` file to not see them again. This file tells git to ignore these files across the board. Creating one is simple, but will depend on your operating system. `Here <http://egorsmirnov.me/2015/05/04/global-gitignore-file.html>`_ is a handy reference, but there are others — a quick google search should do the trick. Remember to unstage these files before committing.


Merge the master into your branch
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
You can merge updates from the master branch into your own branch in the following way: first, load changes to the remote repository, switch to the master branch, and pull its latest version by doing

.. code-block:: shell

   git fetch   # check for updates
   git checkout master  # switch to master branch
   git pull   # pull latest version

.. hint::

    You can only checkout other branches if there are no unsaved (i.e. unstaged) changes in your working directory!
    
Now, we need to go back to the feature branch and merge them:

.. code-block:: shell

   git checkout <your_branch>   # replace <your_branch> by the name of your branch
   git merge master


A text editor should open with a commit message. In general, it is ok to leave the message as it is, thus just save and exit the editor (in vim type: :x ). Don't forget to re-build Utopia afterwards!

Merge conflicts
^^^^^^^^^^^^^^^

If you made changes to parts of the code that was being updated by the master, you will encounter so-called *merge conflicts*. There are several ways to resolve them; for a good overview, have a look `at this Stack Overflow answer <https://stackoverflow.com/q/161813/1827608>`_.

Problems with git?
^^^^^^^^^^^^^^^^^^
`Oh shit, Git! <https://ohshitgit.com/>`_


The Gitlab workflow: Plan, develop, merge
-----------------------------------------
The Utopia project uses the `GitLab platform <https://about.gitlab.com/>`_ for its version control. GitLab is a platform that helps managing large software projects. It encompasses a lot of features. First of all, all of the code that is controlled by git is stored on a central server. On the `project page <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia>`_ you can see all the files, and below them some information on the project, e.g. how to install and use it. Take a look at the `About GitLab <https://about.gitlab.com/>`_ page, as well as Utopia's project page to get yourself familiarised with the purpose and interface of GitLab.

Issues: The planning phase
^^^^^^^^^^^^^^^^^^^^^^^^^^
About half-way down the left-hand pane, you should see a section titled "`Issues <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues>`_". This is where everyone working with Utopia can suggest new features and improvements, discuss topics, propose new models, and so on. Feel free to take a look around, read the different issues, the discussions that sometimes emerge, and if you have an idea or comment, just add it to the comments section!

Let's say you want to create a new model. Click on the "New issue" button in the top right-hand corner of the page: a new page will open. In the field ``Choose a template`` you can select a template and use the structure that is already given. For posting a model idea, we recommend the ``task`` template. Some text will appear: fill in the sections, and keep in mind that in an issue you try to plan what you would like to do, and tell others (and of course yourself) about it. Do not worry if you cannot fill in every section: the description can be changed later. You can also select the appropriate labels for your issue, such that everyone who sees the issue immediately knows what the issue is roughly about. This also facilitates finding your issue later.

Now click on the ``Submit issue`` button and – congratulations, you have created your first issue! At the bottom of the issue, there is the possibility to write comments. Ideally, this is the place to discuss everything related to the issue; if you have doubts or questions about certain aspects or details, just start a discussion. You will always get fruitful input from others! You can even refer to others by writing typing ``@`` followed by the name. We strongly encourage you to use the issue board and profit from the exchange with others. Whenever you have the impression that a feature is missing, something isn't working way you need it to, etc. — *just write an issue*. It needn't be long: in fact, for minor bugs, a few descriptive lines are perfectly sufficient!


Merge Request: The development phase
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you have planned out your issue to a sufficient extent (*you* decide what that means) and want to start coding, you can open a `merge reques` (MR). A MR is just that: a request to merge your branch into another branch, typically the master branch, though you can specify which branch you want to merge into. Click on the "Create merge request" button you find in the issue: this will automatically redirect you to a new page with your merge request, where you can select your branch (source) and the target branch into which you want to merge your changes.

When you create a MR, you should provide some information about what you want to implement. For this, click on the ``Edit`` button in the top right-hand corner of the page. Just as for the issue, you can also ``Choose a template``. Choose the ``Model-MR`` and fill in what you can already fill in. You should update this description alongside your work on the merge request, at the very latest when you are nearing the merge.

Remove *WIP*: The "this should be merged into the master!" phase
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you feel confident that your project (or some completed intermediate version) is ready to be integrated into the master, just remove the *WIP* in the title of the merge request and perhaps mention someone in the comments to have a look at the things that are added; you can also use the right sidebar to assign a reviewer for the merge request. If you have  implemented a new model, first make sure that you have met all the :ref:`dev_model_requirements`.

You should know that nothing will be merged into the Utopia master that has not been reviewed by at least one other developer. But code review is a great opportunity to enhance your code and with it: your coding abilities. All annotations are there to *help* you and to guarantee a high quality of code in the Utopia master branch. Their purpose is not at all to criticize you or your work. You should make full use of the possibility of commenting and discussing, especially if you are unsure about something, or you think that your code does not work correctly.

Automatic Testing Pipeline
^^^^^^^^^^^^^^^^^^^^^^^^^^

How do we ensure that everything that is implemented in Utopia works correctly? By :ref:`writing tests <impl_unit_tests>`! Every time you push to any branch in the Utopia project, the code will be automatically tested in a so-called `pipeline`. These tests allow us to for instance check that a function returns exactly what it should return in all possible cases. Checking every component of your code with a test allows you to be quite sure that your code does what you want it to do. What is more, if future changes to your code happen to impair its functionality, the tests will fail, thereby alerting you to the error.

Where do these tests come from? You need to write them. You can write tests in C++ and/or Python. For examples, look at existing model tests:

* Python: look at the files in ``utopia/python/model_tests/``
* C++: look at the files in the ``test`` directory inside of each model within
  the ``src/utopia/models`` directory.

For setting up the testing system for your model, look at the description in the :ref:`impl_step_by_step`.
For running your tests, see :ref:`running_tests`.

Note that if you have set up the testing infrastructure for your model, your tests will automatically be built and executed whenever you push something to the GitLab project. If your test fails, you will receive an e-mail notification and you will see in your merge request that the test failed. If this happens, don't worry! Just fix the error, commit it, and push it again. Only when you want your merge request to get integrated into the Utopia master branch do all tests need to pass.


