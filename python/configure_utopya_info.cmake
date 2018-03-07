set(UTOPYA_INFO "/Users/Yunus/Documents/Science/Coding/utopia/utopia/python/utopya/utopya/info.py")
set(RANDOM_STR "xNW1iamiZUyibHipjM0yRu5Wn6JELOhzBkT05rEZRM")

set(UTOPIA_MODEL_TARGETS "dummy")
set(UTOPIA_MODEL_BINPATHS "/Users/Yunus/Documents/Science/Coding/utopia/utopia/build-cmake/dune/utopia/models/dummy")

# Configure the actual utopya info module
message(STATUS "Configuring utopya info module ...")
configure_file(${UTOPYA_INFO}.in ${UTOPYA_INFO} @ONLY) 
