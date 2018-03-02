<!--
PLEASE READ THIS!

A merge request from a meta task has the aim to bundle all merge operations of
the associated tasks and then merge into the master branch.
Thus, the target of the meta task MR is the master.

In the description below, make a checkbox list of the merge requests, that are to be merged into this branch.

Usually, the merge will only take place once all tasks have been merged into this.

IMPORTANT: Make sure to set this merge request WIP.
-->
<Link to meta task and brief description>

#### To be merged into this branch before merging into `master`:
* [ ] <link to MR1> — <Brief description 1>
* [ ] <link to MR2> — <Brief description 2>
* [ ] <link to MR3> — <Brief description 3>
* [ ] ...

Closes <add issue numbers of tasks here>
<!-- Example: (Do not put words or comma between the numbers)
Closes #1 #23 #42
-->

/target master
/label ~meta