# Contributing to Utopia

**Thank you for taking your time and contributing to Utopia!** 👍

## How to Contribute

Utopia is open source software. We strive for making every stage of development
public, sharing our advances, and incorporating community contributions. We
therefore prefer public contributions of any kind via GitLab.

To uphold a welcoming, friendly, and productive experience with Utopia, we
expect all involved parties to adhere to our included
[Code of Conduct](CODE_OF_CONDUCT.md) and
[report](mailto:utopia-dev@iup.uni-heidelberg.de) any misconduct immediately.

Our preferred workflow with Git, GitLab, and Utopia is documented in the
[Greenhorn Guide](https://hermes.iup.uni-heidelberg.de/utopia_doc/master/html/guides/greenhorn-guide.html)
of the Utopia user manual.

### How to Register

The Utopia repository is hosted on the [private GitLab instance](https://ts-gitlab.iup.uni-heidelberg.de/)
of the [TS-CCEES](http://ts.iup.uni-heidelberg.de/) research group at the
[Institute of Environmental Physics (IUP) Heidelberg](http://www.iup.uni-heidelberg.de/).
Use the regular GitLab registration procedure to create your account.

As the GitLab instance hosts many of our group-internal projects, new users
will be registered as
[external users](https://docs.gitlab.com/ee/user/permissions.html#external-users-core-only)
with access to the public repositories only and without rights to create their
own projects. Use the "Request Access" button if you want to become a member
of a certain project.

### Questions and Bug Reports

Before asking questions or reporting bugs, please make sure your issue cannot be
resolved by following the instructions in the
[Troubleshooting](README.md#troubleshooting) section of the `README.md` and that
the FAQ segment of the
[online documentation](https://hermes.iup.uni-heidelberg.de/utopia_doc/master/html/index.html)
does not already cover your question.

We prefer to discuss even minor questions users might have in public, so we
encourage you to
[raise an issue](https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/issues/new?issue)
in GitLab and use the `question` description template.

### Code Development

Feature suggestions and implementation plans are discussed in GitLab Issues.
We provide several description templates you may find useful for structuring
your issue description and providing the required information.

When writing code, please follow our
[Coding Style Guidelines](https://hermes.iup.uni-heidelberg.de/utopia_doc/master/html/guides/coding-guidelines.html).
Any changes to source code should be accompanied by a (unit) test verifying the
functionality. Designing this test ideally happens in the planning phase of
a change.

After discussing the issue and implementing the feature, open a merge request.
Again, we encourage you to use one of the description templates. Provide
information on how your code changes and additions solve the problem or
implement the feature. Make sure that your MR fulfills the criteria for
being merged.
