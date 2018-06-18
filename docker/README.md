# Utopia Testing Image

Utopia uses a public Docker Image for its Continuous Integration (CI) system.

The image is located on [Docker Hub](https://hub.docker.com/r/ccees/utopia/).
The subdirectories denote the respective tag of the image.

The image has all dependencies and DUNE preinstalled.
A new version of Utopia can be loaded into it an installed directly.

## Image Build Instructions
For building a new image, pass a file to the `docker build` command using the `-f` option; this also identifies the `<tag>`:

    docker build --no-cache -t ccees/utopia:<tag> -f <tag>.dockerfile

The `--no-cache` flag leads to already existing layers of this image not being taken from cache but built anew. This seems to reduce timeout/rejection issues when pushing the image later on.

Afterwards, `docker login -u ccees` and `docker push` the new image to Docker Hub.

__The required credentials are kept secret for the time being.__
