# --- Build commands for Utopia docker images ---------------------------------
#
# Requires/assumes the following variables for configuration.
# These can be overwritten via environment variables, but ideally the defaults
# specified here should be respected. (Single source of truth.)
#
UTOPIA_BASE_IMAGE_NAME ?= ccees/utopia-base
UTOPIA_BASE_IMAGE_UBUNTU_VERSION ?= jammy
UTOPIA_BASE_IMAGE_VERSION ?= 2.1
UTOPIA_BASE_IMAGE_BUILD_OPTIONS ?=

UTOPIA_IMAGE_NAME ?= ccees/utopia
UTOPIA_IMAGE_REMOTE_URL ?= https://gitlab.com/utopia-project/utopia.git
UTOPIA_IMAGE_BRANCH ?= master
UTOPIA_IMAGE_BUILD_OPTIONS ?=

#
# Derived variables.
# Don't change these light-heartedly, it will break the pattern!
#
UTOPIA_BASE_IMAGE_TAG ?= $(UTOPIA_BASE_IMAGE_UBUNTU_VERSION)-v$(UTOPIA_BASE_IMAGE_VERSION)
UTOPIA_BASE_IMAGE ?= $(UTOPIA_BASE_IMAGE_NAME):$(UTOPIA_BASE_IMAGE_TAG)

UTOPIA_IMAGE_TAG ?= latest
UTOPIA_IMAGE ?= $(UTOPIA_IMAGE_NAME):$(UTOPIA_IMAGE_TAG)


# -----------------------------------------------------------------------------
# -- Get (fully qualified) image names
get_base_image_name:
	@echo $(UTOPIA_BASE_IMAGE)

get_utopia_image_name:
	@echo $(UTOPIA_IMAGE)


# -- Build targets
build_base_image:
	@echo "Building Utopia base docker image ..."
	docker build . \
		-f utopia-base.dockerfile \
		-t $(UTOPIA_BASE_IMAGE) \
		-t $(UTOPIA_BASE_IMAGE_NAME):latest \
		--build-arg UBUNTU_VERSION=$(UTOPIA_BASE_IMAGE_UBUNTU_VERSION) \
		$(UTOPIA_BASE_IMAGE_BUILD_OPTIONS)
	@echo "Built Utopia base docker image, tagged $(UTOPIA_BASE_IMAGE) and :latest ..."

build_utopia_image:
	@echo "Building Utopia docker image ..."
	docker build . \
		-f utopia.dockerfile \
		-t $(UTOPIA_IMAGE) \
		--build-arg BASE_IMAGE=$(UTOPIA_BASE_IMAGE) \
		--build-arg GIT_REMOTE_URL=$(UTOPIA_IMAGE_REMOTE_URL) \
		--build-arg GIT_CHECKOUT=$(UTOPIA_IMAGE_BRANCH) \
		$(UTOPIA_IMAGE_BUILD_OPTIONS)
	@echo "Built Utopia docker image, tagged $(UTOPIA_IMAGE) ..."


# -- Push targets
push_base_image:
	docker push $(UTOPIA_BASE_IMAGE)
	docker push $(UTOPIA_BASE_IMAGE_NAME):latest

push_utopia_image: build_utopia_image
	docker push $(UTOPIA_IMAGE_NAME):latest


# -- Combined
build_and_push_base_image: build_base_image push_base_image

build_and_push_utopia_image: build_utopia_image push_utopia_image
