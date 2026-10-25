# Кореневий Makefile (Linux / macOS / CI). На Windows без make: scripts/dev.ps1
#   make image | build | test | coverage | check | all | clean
IMAGE ?= stm32-build
ifeq ($(OS),Windows_NT)
  USER_FLAG :=
else
  USER_FLAG := --user $$(id -u):$$(id -g)
endif
DOCKER_RUN = docker run --rm -v "$(CURDIR)":/workspace -w /workspace $(USER_FLAG) $(IMAGE)

.PHONY: image build test coverage check all clean
image:
	docker build -t $(IMAGE) docker/
build:
	$(DOCKER_RUN) make -C firmware all
test:
	$(DOCKER_RUN) make -C tests test
coverage:
	$(DOCKER_RUN) make -C tests coverage
check:
	$(DOCKER_RUN) python3 scripts/check_deps.py
all: check test build
clean:
	$(DOCKER_RUN) sh -c "make -C firmware clean; make -C tests clean"
