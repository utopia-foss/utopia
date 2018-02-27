# Utopia Testing Image

Utopia uses a public Docker Image for its Continuous Integration (CI) system.

The image is located on [Docker Hub](https://hub.docker.com/r/citcat/dune-env/).
For historic reasons, it's called `citcat/dune-env`. The subdirectories denote
the respective tag of the built image.

The image has all dependencies and DUNE preinstalled.
A new version of Utopia can be loaded into it an installed directly.

## Image Build Instructions
For building a new image, enter the respective subdirectory (identifying the `tag`)
and call

    docker build -t citcat/dune-env:<tag> .

Replace `<tag>` with the appropriate tag.
Don't forget the dot (`.`) at the end of this call!

Afterwards, `docker push` the new image to Docker Hub.

__The required credentials are kept secret for the time being.__