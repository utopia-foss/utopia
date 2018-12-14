# Utopia Testing Image

Utopia uses a public Docker Image for its Continuous Integration (CI) system.

The image is located on [Docker Hub](https://hub.docker.com/r/ccees/utopia/) and  has all dependencies and DUNE preinstalled.
The current version of Utopia can be loaded into it an installed directly.

### Automated Building in GitLab CI
All necessary actions to build an image can be performed via the `setup` stage of the `.gitlab-ci.yml`, which a maintainer can trigger from the GitLab web interface.

Additionally, the `prep` stage of the CI updates the image based on the instructions in `dune-env-update.dockerfile`, carrying out e.g. updates to DUNE. It is triggered automatically on every push to the `master` branch, e.g. a merge operation.


## Local Image Build Instructions
For building the image locally, enter this directory and call

    docker build --no-cache -f dune4utopia.dockerfile .

Do not forget the dot (`.`) at the end of this call!

The `--no-cache` flag leads to already existing layers of this image not being taken from cache but built anew. This seems to reduce timeout/rejection issues when pushing the image later on.

The following build arguments are available and can be passed to `docker build`via `--build-arg`:
* `CC`, `GCC`: the compilers used, default to `gcc` and `g++`, respectively
* `DUNE_VERSION`: the DUNE version to use, defaults to 2.6
* `PROCNUM`: number of processes used for building

For building the clang version of the image, pass `clang` and `clang++` to the respective arguments.
