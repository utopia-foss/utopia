.. _dev_workflow:

Development Workflow
====================

You do not know anything about Git, GitLab or the general workflow when working with Utopia? You have come to the right place! ✌

This guide should help you to get to know the basic tools needed to work with Utopia.\ [#fn-1]_ The aim is to show you a selection of most important features and commands you will use most of the time. This guide does not intend to show the full range of possibilities, but rather it should be a guide for you to immediately be able to start exploring the Utopia world and to have one place to look up the most important commands you need.
That automatically implies that often we will not go into deeper explanations about how it works. As a user, you should have a good starting point to explore more on yourself by just trying out or using Google to dive deeper into the topics.

.. contents::
   :local:
   :depth: 2

----

Philosophy
----------

So... to get you into the right mind-set for working with *Utopia*, please keep in mind the following philosophy when doing *anything* with Utopia.
This basically is a selection of short sentences you can live by (at least within the group).
Feel free to write them as an inspirational quote on an image and share them on your favorite social network. 😉

* **Just try it out.**
* **Learning by doing.**
* **You cannot break anything.** Really!
* **Ask questions and let others review your code!**

   * ... *especially* if you think that your code is ugly, bad or slow. This is normal in the beginning, so no worries! It can be easily changed if you harvest the knowledge of the group. 😃
   * No one will kill you. Or even judge you.

Now, let's start with the workflow!


Workflow
--------

* Have you ever wondered how big software projects are managed?
* How do hundreds of people work on the same code without breaking it, although just one missing semicolon would suffice to do that?
* You want to explore an earlier version of a project?
* You just want to try out something, but do not know whether you want to keep the changes or throw them away?

Git and GitLab provide answers to these kinds of questions. Git is a version control system. So, you can keep a stable version of your code that works all the time while you develop the code in another place. You can think of these different stages of the code as a tree, which has a root, one big master branch, and many small outgoing branches.
However, in Git outgoing branches can be merged again into the stable master branch. ... tell that to a tree! 🤔

For Utopia this means that there is always a master branch (a stable version), which you can download and which is guaranteed to work. All the development, so the things people are working on separately at the same time, happens on individual branches.

So, how do you create a new branch on which to work on?

Gitlab
------

Before we can explain this question, we first need to roughly understand what the `GitLab <https://about.gitlab.com/>`_ is. GitLab is a platform that helps to manage large software projects. This embraces a lot of features. First of all the whole Code that is controlled by Git is stored on a group server. You have a `project page <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia>`_\  where you can directly see all the files and below some information about the project, e.g. how to install and use it.

Now: Check out the `About GitLab <https://about.gitlab.com/>`_ page and also Utopia's project page to get yourself familiarised with the purpose and interface of GitLab.

For members of the CCEES group, development takes place not only in the
``utopia/utopia`` project, but also in the ``utopia/models`` project, which you
can access `here <https://ts-gitlab.iup.uni-heidelberg.de/utopia/models>`_.
The distinction exists to separate framework code (developed in the
``utopia`` project) from personal model implementations (in the ``models``
project).

Issues: The planning phase
^^^^^^^^^^^^^^^^^^^^^^^^^^
Everyone who works with Utopia needs to know what topics are up for discussion, which features are planned, which model should be implemented, and so on. All of these things can be looked up in the `issues <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues>`_ page of the respective GitLab projects.
Just go there, click around, look at different issues, the discussions that sometimes emerge and if you have an idea or comment, just feel free to write it in the comment section! :)

Let's say you want to create a new model. If this is a model for your personal
project visit the `issues page of the models project <https://ts-gitlab.iup.uni-heidelberg.de/utopia/models/issues>`_ and click on the "New issue" button in the top right-hand corner of the page.
A new page will open. In the field ``Choose a template`` you can select a template and use the structure that is already given. For posting a model idea, we recommend the ``task`` template.
Now, some text will appear. Fill in the sections and keep in mind that in an issue you try to plan what you would like to do and tell others (and of course yourself) about it. Everyone should be able to know what is going on in Utopia.
Do not worry if you cannot fill in every section. The description can also be changed later.

If you create an issue you should also look at the labels you can give it. Select the appropriate labels such that everyone who sees the issue immediately roughly knows what the issue is about.

Now click on the ``Submit issue`` button and congratulations you have created your first issue! 😃
At the bottom of the issue, there is the possibility to write comments and discussions. So others can look at your issue, encourage you to go for it, add their ideas, and so on. Ideally, this is the place to discuss everything related to the issue. So, also if you have doubts about certain aspects or details just start a discussion. You will always get fruitful input from others! You can even refer to others by writing typing ``@`` followed by the name.
We encourage you to try it out and profit from the exchange with others.

During your model development, whenever you have the impression that some feature is missing in Utopia, something does not work the way you need it in your model, etc. ... *just write an issue*.
Even if you do not exactly know, whether it is a good idea, you just want to discuss something, or bring something up: Writing an issue – even if brief – is the best way to discuss something, include others when needed, use code examples, connect to other issues, let actual code changes follow from your issue...

Of course, it makes sense to make this process efficient, both for you and for others: If there is a minor bug that you don't want to spend time on right now, let the issue description be brief as well.
If it is a larger thing you would like to discuss, it might make sense to invest fifteen minutes, such that others know exactly what you would like to discuss.

.. note::

  If you are wondering whether an issue should go into the ``utopia`` framework project or the ``models`` project, the answer is simple:

  If the issue relates solely to the implementation or enhancement of your *personal* project, e.g. a model you investigate as part of your MSc project,
  it goes into ``models``.
  Otherwise, your issue probably belongs into the ``utopia`` framework project;
  for example, a bug report or a suggestion of a new *general* feature.

  If you are uncertain about this, don't despair. Issues can also be moved around; just ask someone what they think.
  However, the issue should be in the right project *before* you create a merge request in the next step.


Merge Request: The development phase
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you have planned out your issue to a sufficient extent (*you* decide what that means) and want to start working on an issue, click on the "Create merge request" button you find in the issue.
This will redirect you automatically to a new page with your merge request. Further, this will create a new branch that departs from the master branch. If you do not remember, what this means, look it up above. 😉

There are two things you should do before you start writing code:

#.
   Provide information about what you want to implement. For this, click on the ``Edit`` button in the top right-hand corner of the page. As for the issue, you can also ``Choose a template``. Choose the ``Model-MR`` and fill in what you can already fill in. You should update this description alongside your work on the merge request, the latest when you are nearing the merge.

#.
   Your local machine still does not know that there is a new branch and, of course, is also not yet on this new branch. To change this search the ``checkout branch`` button and follow only the first two commands of it. Now, you should be on the right branch. You can check it by typing ``git status`` and make sure the first line of the output tells you that you are on your newly created branch.

Well done! You are now on your own development branch. 🎉

In the `How-to-build-a-model Guide <how-to-build-a-model>` you learn how to use the models that already exist in utopia, so you don't have to start your model implementation from scratch.

Remove *WIP*\ : The "this should be merged into the master!" phase
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You feel confident that your project (or some completed intermediate version) is ready to be integrated into the master? If you implemented a new model first make sure that you have met all :doc:`model requirements <model-requirements>`. If everything is fine just remove the *WIP* in the title of the merge request and perhaps mention someone in the comments to have a look at the things that are added. You should know that nothing will be merged into the Utopia master that has not been reviewed by at least one other developer.

This is a great opportunity to enhance your code and with it: your coding abilities.
Importantly, note that all annotations are there to *help* you and to guarantee a high quality in the Utopia master branch. Their purpose is not at all to criticize you or your work. Everyone knows that starting to code is really hard, so be assured that we work *together* with you and never against you.

Comments and Discussions
""""""""""""""""""""""""
Just use the possibility to write comments and discussions a lot! Especially if you are unsure about something, you think that your code is bad, ugly, and does not work correctly, or before you would invest a lot of time without a lot of progress just ask the others! We are a very open group and want to help you wherever we can do! So just go ahead and ask. ☺

Automatic Testing Pipeline
""""""""""""""""""""""""""
How do we ensure that everything that is implemented in Utopia works correctly? We write tests!

With tests we can for example check that a function returns exactly what it should return in all possible cases. So, if you automatically check every subpart of your code with a test you can be quite sure that your code does what you want it to do. Even more, if someone changes something in the future, which would break your code, it will lead to failing tests because it will always be automatically checked if your code still works.

Where do the tests come from? You write them. You can write tests in C++ and/or Python. For examples, look at existing model tests:

* Python: Look at the files in ``utopia/python/model_tests/``
* C++: Look at the files in the ``test`` directory inside of each model within
  the ``src/utopia/models`` directory.

For setting up the testing system for your model, look at the description in :doc:`how-to-build-a-model`. For running your tests, see the :doc:`README <../readme>`.

Note that if you have set up the testing infrastructure for your model, your tests will automatically be built and executed if you push something to the GitLab. If your test fails, you will receive a mail notification and you will see in your merge request that the test failed. But don't worry if this happens! :)
Just fix the error, commit it, and push it again. Only when you want your merge request to get integrated into the Utopia master branch, all tests need to work without problems.


Git
---
There are a lot of guides and tutorials online where you can learn how to use Git. A quick-start would be `this one <https://git-scm.com/book/en/v2/Getting-Started-Git-Basics>`_; a longer one can be found on `git-tower <https://www.git-tower.com/learn/git/ebook>`_. Also, the `git documentation <https://git-scm.com/doc>`_ is quite comprehensive and well-written.

Here, for that reason, we provide only a small selection of commands that you will use a lot during your code development and address some frequent issues encountered when starting to use git are mentioned.

If you have questions or run into problems, it is always worth consulting the ``git <command> --help`` or checking out the guides linked above.
Also, searching the internet for what you want to do is usually quite effective when it comes to questions with git.


Stage files
^^^^^^^^^^^
The commands below demonstrate a typical way of committing files to the repository.

.. code-block:: shell

   git status              # Check the status of the repository
                           # Most importantly: Check you are on the desired branch
   git add path/to/file    # Add a file which has changes that should be stored
   git status              # Check that you did not stage any undesired files
   git commit -m '<...>'   # The message that describes what has been changed.
                           # Always think about the sentence: "If applied this commit will ..."
                           # Your commit message should start where the three dots end.
                           # A valid example would be:
                           # git commit -m 'Implement the basic interaction mechanism'
   git push                # Push the changes to the GitLab

.. note::

  If you are wondering when to commit and how to write a good commit message, have a look at these `Version Control Best Practices <https://www.git-tower.com/learn/git/ebook/en/command-line/appendix/best-practices>`_ and this `blog post <https://jasonmccreary.me/articles/when-to-make-git-commit/>`_.

  Before you can commit anything, git prompts you to provide your name and email address with the given commands.
  When choosing your name, be aware that this name is immortalized in the git history (please choose your full name and a decent email address ;)).

Global .gitignore
"""""""""""""""""
You run the ``git status`` command and see a lot of files that you have not created e.g. ``.DS_Store``\ , ``./vscode``\ , or similar?
These sometimes are files created by your operating system or by your IDE.
You can and should create a global ``.gitignore`` file to not see them again. Either google it or look `here <http://egorsmirnov.me/2015/05/04/global-gitignore-file.html>`_.


Merge the master into your branch
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you want to get updates that are available on the master branch, you can follow the commands below.

First, we need to get the updates for the master branch:

.. code-block:: shell

   git checkout master
   git pull

Now, we need to go back to the feature branch and merge them:

.. code-block:: shell

   git checkout <your_branch>   # replace <your_branch> by the name of your branch
   git merge master             # A text editor should open with a commit message.
                                # In general, it is ok to leave the message as it is, thus just save and exit the editor (in vim type: :x )

Don't forget to re-build the code afterward. 😉

.. note::

  If you made changes in the part of the code that was being updated by the master, you will encounter so-called *merge conflicts*. There are several ways to resolve them, for a good overview, have a look `at this SO answer <https://stackoverflow.com/q/161813/1827608>`_.


Problems with git?
^^^^^^^^^^^^^^^^^^
`Oh shit, Git! <https://ohshitgit.com/>`_

----

.. rubric:: Footnotes

.. [#fn-1] Actually, these are the tools that software engineers also use. So, if you think about a career outside of the scientific world after your work in this group, it really is useful to start getting to know the workflow. :)
