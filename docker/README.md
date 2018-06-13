# Utopia Testing Image

Utopia uses a public Docker Image for its Continuous Integration (CI) system.

The image is located on [Docker Hub](https://hub.docker.com/r/ccees/utopia/).
The subdirectories denote the respective tag of the image.

The image has all dependencies and DUNE preinstalled.
A new version of Utopia can be loaded into it an installed directly.

## Image Build Instructions
For building a new image, enter this directory and call

    docker build --no-cache -f dune-env.dockerfile -t ccees/utopia:bionic .

Do not forget the dot (`.`) at the end of this call!

The `--no-cache` flag leads to already existing layers of this image not being taken from cache but built anew. This seems to reduce timeout/rejection issues when pushing the image later on.

Afterwards, `docker login -u ccees` and `docker push` the new image to Docker Hub.

__The required credentials are kept secret for the time being.__

## Automated Build Update
The CI job `setup:update-dune` is regularly executed on the `master` branch.
It updates the image based on the instructions in `dune-env-update.dockerfile`
and pushes it to the Docker Hub again.

If name or tag of the image change, do not forget to adapt the `DUNE_ENV_IMAGE`
variable in the `gitlab-ci.yml`, telling the job which image to pull and push.

You can also manually perform the update by calling

    docker build --no-cache -f dune-env-update.dockerfile -t ccees/utopia:bionic .