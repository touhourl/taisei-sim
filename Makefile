SHELL := /bin/bash
.SHELLFLAGS := -euo pipefail -c
.ONESHELL:

PROJECT_ROOT := $(CURDIR)
BUILD_DIR := $(PROJECT_ROOT)/build
VENV_DIR := $(PROJECT_ROOT)/.venv

build:
	command -v python3 >/dev/null 2>&1

	if [ ! -d "$(VENV_DIR)" ]; then
		python3 -m venv "$(VENV_DIR)"
	fi

	source "$(VENV_DIR)/bin/activate"

	python -m pip install backports.zstd
	git submodule update --init --recursive

	if [ ! -d "$(BUILD_DIR)" ]; then
		meson setup "$(BUILD_DIR)"
	else
		meson setup "$(BUILD_DIR)" --reconfigure
	fi

	meson compile -C "$(BUILD_DIR)"
	meson install -C "$(BUILD_DIR)"

