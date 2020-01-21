<!-- Set the title to: "Update support/<tag>" -->
<!-- Replace TAG with the actual version numbers everywhere -->

### What does this MR do?

This MR collects MRs that have already been merged into the `master` branch and
merges them collectively into the `support/TAG` branch.

#### List of MRs to be included

List the MRs and the commit SHA of their respective merge commits here.
Placing them in chronological order here ensures fewer issues when
cherry-picking them!

The MRs are indicated by the ~"pick-into: TAG" label.

| MR | Merge Commit SHA |
| -- | ---------------- |
| !  |  ...  |

### Can this MR be merged?

- [ ] Created branch of this MR from `support/TAG`
- [ ] Cherry-picked the listed commits into this branch
- [ ] Pipeline passing without warnings
- [ ] Approved by @ ... <!-- Add reviewer(s) here once no longer WIP -->

### Help on Cherry-Picking

Cherry-picking merge commits requires specifying the "mainline" parent, which
should always be number 1. Append a line indicating a cherry-pick to the commit
message with the `-x` argument. The following should do the trick:

```bash
git cherry-pick -m 1 -x <commits>
```

Replace `<commits>` with the Commit SHAs listed above, separated by a single
whitespace each. Make sure they are in chronological order to reduce the
number of merge conflicts! Fix such conflicts as unintrusively as possible.

### Related Issues

Closes #
