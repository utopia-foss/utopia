<!-- _Set the title to: "Update supported Ubuntu version to X.Y" -->
<!-- Replace X.Y with the Ubuntu version and TAG with the release name -->

### What does this MR do?

We're incrementing the supported Ubuntu version to `X.Y`! :tada:

**Note:** This will drop support for all older Ubuntu versions in the `master`
branch. Support for selected Ubuntu versions might continue in maintained
`support/TAG` branches.

### Tasks

- [ ] Update the Docker base image: `ccees/utopia-base:TAG-v1`
  - [ ] In `utopia-base.dockerfile`
  - [ ] In `.gitlab-ci.yml`
- [ ] Enforce most recent software versions in:
  - [ ] `cmake/modules/UtopiaMacros.cmake`
  - [ ] `cmake/modules/UtopiaEnv.cmake`
  - [ ] `cmake/UtopiaConfig.cmake.in`
- [ ] Update `README.md`: Installation instructions and dependency list

### Can this MR be merged?

- [ ] New Docker base image correctly built
- [ ] Reasonably up-to-date with current master
- [ ] Pipeline passing without warnings
- [ ] Squash option set <!-- unless there's a good reason not to squash -->
- [ ] Approved by @ ...

### Notes and help

- If applicable, create a new base image `ccees/utopia-base:TAG-v1` with the
  name of the Ubuntu release as additional tag. Base the image off the
  respective `ubuntu:TAG`. We start with version `v1` for every new Ubuntu
  release.
- The Ubuntu packages available in the different OS versions can be found on
  the [Ubuntu Packages][ubuntu-packages] website.

### Related issues

Closes #

[ubuntu-packages]: https://packages.ubuntu.com/
