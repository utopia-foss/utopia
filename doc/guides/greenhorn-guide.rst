
Greenhorn Guide
===============

**This guide is work in progress!**

You do not know anything about Git, GitLab or the general workflow when working with Utopia? You have come to the right place! ✌

This guide should help you to get to know the basic tools needed to work with Utopia.\ [#fn-1]_ The aim is to show you a selection of most important features and commands you will use most of the time. This guide does not intend to show the full range of possibilities, but rather it should be a guide for you to immediately be able to start exploring the Utopia world, and to have one place to look up the most important commands you need.
That automatically implies that often we will not go into deeper explanations about how it works. As a user you should have a good starting point to explore more on yourself by just trying out or using Google to dive deeper into the topics.

So... to get you into the mind set, please keep in mind the following when doing *anything* with Utopia:

Philosophy
^^^^^^^^^^

A selection of short sentences you can live by (at least within the group). Feel free to write them as an inspirational quote on an image and share them on your favourite social network. 😉 


* **Just try it out.**
* **Learning by doing.**
* **You cannot break anything.** Really!
* **Ask questions and let others review your code!**

  * ... *especially* if you think that your code is ugly, bad or slow. This is normal in the beginning, so no worries! It can be easily changed if you harvest the knowledge of the group. 😃
  * No one will kill you. Or even judge you.

Now, let's start with Git and GitLab!

Git & GitLab
------------


* Have you ever wondered how big software projects are managed?
* How do hundreds of people work on the same code without breaking it, although just one missing semicolon would suffice to do that?
* You want to explore an earlier version of a project?
* You just want to try out something, but do not know whether you want to keep the changes or throw them away?

Git and GitLab provide answers to these kind of questions. Git is a version control system. So, basically you can keep a stable version of your code that works all the time while you develop the code in another place. You can think of these different stages of the code as a tree, which has a root, one big master branch and many small outgoing branches. However, in Git outgoing branches can be merged again into the stable master branch. ... tell that to a tree! 🤔 

For Utopia this means that there is always a master branch (a stable version), which you can download and which is guaranteed to work. All the development, so the things people are working on separately at the same time, happens on individual branches. 

So, how do you create a new branch on which to work on?

Gitlab
^^^^^^

Before we can explain this question, we first need to roughly understand what the `GitLab <https://about.gitlab.com/>`_ is. GitLab is a platform that helps to manage large software projects. This embraces a lot of features. First of all the whole Code that is controlled by Git is stored on a group server. You have a `project page <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia>`_\ , where you can directly see all the files and below some information about the project, e.g. how to install and use it.

Now: Check out the `About GitLab <https://about.gitlab.com/>`_ page and also Utopia's project page to get yourself familiarised with the purpose and interface of GitLab.

Issues: The planning phase
~~~~~~~~~~~~~~~~~~~~~~~~~~

Everyone who works with Utopia needs to know what topics are up for discussion, which features are planned, which model should be implemented, and so on. All of these things can be looked up in the `issues <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues>`_. Just go there, click around, look at different issues, the discussions that sometimes emerge and if you have an idea or comment, just feel free to write it in the comment section! :)

Let's say you want to create a new model. Just click on the top right hand corner on the ``New issue`` button. A new window will appear. In the field ``choose a template`` select ``task``. Now, some text will appear. Fill in the sections and keep in mind that in an issue you try to plan what you would like to do and tell others (and of course yourself) about it. Everyone should be able to know, what is going on in Utopia. Do not worry if you cannot really fill in every section. The description can also be changed later.

If you create an issue you should also look at the labels you can give it. Select the appropriate labels such that everyone who sees the issue immediately roughly knows what the issue is about.

Now click on the ``Submit issue`` button and congratulations you have created your first issue! 😃
At the bottom of the issue, there is the possibility to write comments and discussions. So others can look at your issue, encourage you to go for it, add their ideas, and so on. Ideally, this is the place to discuss everything that is related to the issue. So, also if you have doubts about certain aspects or details just start a discussion. You will always get fruitful input from others! You can even refer to others by writing typing ``@`` followed by the name.
We encourage you to try it out and profit from the exchange with others.

During your model development, whenever you have the impression that some feature is missing in Utopia, something does not work the way you need it in your model, etc. ... *just write an issue.*
Even if you do not exactly know, whether it is a good idea, you just want to discuss something, or bring something up: Writing an issue – even if brief – is the best way to discuss something, include others when needed, use code examples, connect to other issues, let actual code changes follow from your issue...

Of course, it makes sense to make this process efficient, both for you and for others: If there is a minor bug that you don't want to spend time on right now, let the issue description be brief as well.
If it is a larger thing you would like to discuss, it might make sense to invest fifteen minutes, such that others know exactly what you would like to discuss.

Merge Request: The development phase
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want to start working on an issue, click on the ``Create merge request`` button. This will redirect you automatically to a new page with your merge request. Further, this will create a new branch which departs from the master branch. If you do not remember, what this means, look it up above. 😉

There are two things you should do before you start actually writing code:


#. 
   Provide information about what you want to actually implement. For this, click on the ``Edit`` button in the top right hand corner of the page. As for the issue, you can also ``Choose a template``. Choose the ``Model-MR`` and fill in what you can already fill in. You should update this description alongside your work on the merge request, the latest when you are nearing the merge.

#. 
   Your local machine still does not know that there is a new branch and, of course, is also not yet on this new branch. To change this search the ``checkout branch`` button and follow only the first two commands of it. Now, you should be on the right branch. You can check it by typing ``git status`` and make sure the first line of the output tells you that you are on your newly created branch.

Well done! You are now on your own development branch. 🎉

Remove *WIP*\ : The "this should be merged into the master!" phase
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You feel confident that your project (or some completed intermediate version) is ready to be integrated into the master? If you implemented a new model first make sure that you have met all `model requirements <https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/blob/master/dune/utopia/models/ModelRequirements.md>`_. If everything is fine just remove the *WIP* in the title of the merge request and perhaps mention someone in the comments to have a look at the things that are added. You should know that nothing will be merged into the Utopia master that has not been reviewed by at least one other developer.

This is a great opportunity to enhance your code and with it: your coding abilities.
Importantly, note that all annotations are there to *help* you and to guarantee a high quality in the Utopia master branch. Their purpose is not at all to criticize you or your work. Everyone knows that starting to code is really hard, so be assured that we work *together* with you and never against you.

Comments and Discussions
~~~~~~~~~~~~~~~~~~~~~~~~

Just use the possibility to write comments and discussions a lot! Especially if you are unsure about something, you think that your code is bad, ugly, and does not work correctly, or before you would invest a lot of time without a lot of progress just ask the others! We are a really open group and want to help you wherever we can do! So just go ahead and ask. ☺

Automatic Testing Pipeline
~~~~~~~~~~~~~~~~~~~~~~~~~~

How do we ensure that everything that is implemented in Utopia works correctly? We write tests!

With tests we can for example check that a function returns exactly what it should return in all possible cases. So, if you automatically check every subpart of your code with a test you can be quite sure that your code does what you want it to do. Even more, if someone changes something in the future, which would break your code, it will lead to failing tests because it will always be automatically checked if your code still works. 


* Where do the tests come from? You write them. You can write tests in Cpp and/or Python. Just look at existing model tests

  * Cpp: Look at the files in the ``test/`` directory inside of each model within ``utopia/dune/models/``.
  * Python: Look at the files in `utopia/python/model_tests/

* How do you set up your own testing system for your model? Look at the description in the Beginner's guide.
* How do I execute tests? 

Note that if you have set up the testing infrastructure for your model, your tests will automatically be build and executed if you push something to the GitLab. If your test fails, you will receive a mail notification and you will see in your merge request that the test failed. But don't worry if this happens! :) Just fix the error, commit it, and push it again. Only when you want your merge request to get integrated in the Utopia master branch, all tests need to work without problems.

Git
^^^

There are a lot of guides and tutorials online where you can learn how to use Git. So, here we provide a small selection of commands that you will use a lot during your code development. Also, some beginner's issues will be mentioned.

Store added, removed, or changed parts of the code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: shell

   git status                                     # Check the status; most importantly on which branch you are and which files are tracked or untracked.
   git add path/to/file                           # Add a file which has changes that should be stored
   git commit -m 'Your personal commit message'   # The message that describes what has been changed. 
                                                  # Always think about the sentence: "If applied this commit will ..."
                                                  # Your commit message should start where the three dots begin.
                                                  # A valid example would be:
                                                  # git commit -m 'Implement the basic interaction mechanism'
   git push                                       # Store the changes in our server

Merge the master into your branch
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: shell

   git checkout master
   git pull
   git checkout <your_branch>   # replace <your_branch> by the name of your branch
   git fetch origin
   git merge master             # A text editor should open with a commit message. 
                                # In general, it is ok to leave the message as it is, thus just save and exit the editor (in vim type: :x )

Don't forget to re-build the code afterwards. 😉 

Revert changes that are not added yet
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: shell

   git checkout path/to/file

Global .gitignore
~~~~~~~~~~~~~~~~~

You run the ``git status`` command and see a lot of files that you have not created e.g. ``.DS_Store``\ , ``./vscode``\ , or similar? This sometimes are fiels created by your operating system or by your IDE. You can create a global ``.gitignore`` file to not see them again. Either google it, or look `here <http://egorsmirnov.me/2015/05/04/global-gitignore-file.html>`_.

Problems with git?
~~~~~~~~~~~~~~~~~~

`Oh shit, Git! <https://ohshitgit.com/>`_

----


.. [#fn-1] Actually, these are the tools which software engineers also use. So, if you think about a career outside of the scientific world after your work in this group, it really is useful to start getting to know the workflow. :)
