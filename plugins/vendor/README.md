# Developers' short introduction to Git Submodules

Submodules work like every other Git repository, but are tracked in their
'superproject'. The superproject stores information on its submodules in the
`.gitmodules` file in the top-level directory and simply records a commit SHA
for any submodule.

### Updating a submodule locally
If you updated the code base of your 'superproject' Utopia by calling `git pull`
or `git checkout <branch>`, the submodule commit SHA might have changed. Git
_will not_ automatically update the submodule, but you have to do so manually
by calling

    git submodule update

### Upgrading the submodule
If you wish to use a new commit/branch/tag of a submodule, you can enter it and
use the regular Git commands. If you return to your superproject, it will notice
your changes. Use

    git diff [--cached] --submodule

to see a list of commits that update the submodule. To always enable this view
when calling `git diff`, update your machine's global git config via

    git config --global diff.submodule log

You can add the changes in the submodule via the usual Git workflow by calling

    git add <path/to/submodule>

and commit them afterwards. Other users pulling your commit will then need to
call `git submodule update` to receive the changes.

### Submodules tracking a branch
Submodules always resemble a repository in 'detached-HEAD' state. _Even when
tracking a certain branch, submodules will always point at a certain commit._
If you which to pull updates from upstream, enter the submodule and call
`git pull`. _Notice that this change has to be committed in the superproject!_

## Current submodule setup

Submodules checking out tags instead of branches to not need regular remote
updates. See the list below for the current setup.

| Software | Checkout | Branch? |
| -------- | -------- | ------- |
| yaml-cpp | `yaml-cpp-0.6.2` | No |
| spdlog | `v1.3.1` | No |
