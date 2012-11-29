# -*- mode: Makefile-gmake -*-

# 金棒 (kanabō)
# Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
#
# 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
# made stronger.
#
# For more information, consult the README file in the project root.
#
# Distributed under an [MIT-style][license] license.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal with
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# - Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimers.
# - Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimers in the documentation and/or
#   other materials provided with the distribution.
# - Neither the names of the copyright holders, nor the names of the authors, nor
#   the names of other contributors may be used to endorse or promote products
#   derived from this Software without specific prior written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
#
# [license]: http://www.opensource.org/licenses/ncsa

include project.mk

## Defaults for project settings
VERSION ?= 1.0.0-SNAPSHOT
PACKAGING ?= program
DEPENDENCIES ?=
TEST_DEPENDENCIES ?=
EXTRA_INCLUDES ?=
CFLAGS ?=

## Defaults for project source directories
SOURCE_DIR  ?= src/main/c
INCLUDE_DIR ?= $(SOURCE_DIR)/include

## Defaults for project test source directories
TEST_SOURCE_DIR  ?= src/test/c
TEST_INCLUDE_DIR ?= $(TEST_SOURCE_DIR)/include

## Defaults for project output directories
TARGET_DIR  ?= target
OBJECT_DIR  ?= $(TARGET_DIR)/objects
TEST_OBJECT_DIR  ?= $(TARGET_DIR)/test-objects
GENERATED_SOURCE_DIR ?= $(TARGET_DIR)/generated-sources
GENERATED_DEPEND_DIR ?= $(TARGET_DIR)/generated-sources/depend
GENERATED_TEST_SOURCE_DIR ?= $(TARGET_DIR)/generated-test-sources
GENERATED_TEST_DEPEND_DIR ?= $(TARGET_DIR)/generated-test-sources/depend

## Project target artifact settings
PROGRAM_NAME ?= $(ARTIFACT_ID)
PROGRAM_TARGET = $(TARGET_DIR)/$(PROGRAM_NAME)
LIBRARY_NAME_BASE ?= $(ARTIFACT_ID)
LIBRARY_NAME ?= lib$(LIBRARY_NAME_BASE).a
LIBRARY_TARGET = $(TARGET_DIR)/$(LIBRARY_NAME)
ifeq ($(strip $(SKIP_TESTS)),)
TEST_PROGRAM_TARGET = $(TARGET_DIR)/$(ARTIFACT_ID)_test
endif

## Project source compiler settings
INCLUDES := $(addprefix -I, $(EXTRA_INCLUDES))
CFLAGS := -I$(INCLUDE_DIR) $(INCLUDES) $(CFLAGS)
LDLIBS := $(addprefix -l, $(DEPENDENCIES))

ifeq ($(BUILD_DEBUG),yes)
CFLAGS := $(CFLAGS) -g
endif

## Project test source compiler settings
TEST_CFLAGS := -I$(TEST_INCLUDE_DIR) $(CFLAGS)
TEST_LDLIBS := $(addprefix -l, $(DEPENDENCIES)) $(addprefix -l, $(TEST_DEPENDENCIES))

## Project source file locations
vpath %.c $(shell find $(SOURCE_DIR) -type d | tr '\n' :)
SOURCES := $(shell find $(SOURCE_DIR) -type f \( -name '*.c' -or -name '*.C' \))
OBJECTS := $(foreach s, $(SOURCES), $(OBJECT_DIR)/$(basename $(notdir $(s))).o)
vpath %.h $(INCLUDE_DIR)
DEPENDS := $(foreach s, $(SOURCES), $(GENERATED_DEPEND_DIR)/$(basename $(notdir $(s))).d)

## Project test source file locations
ifeq ($(strip $(SKIP_TESTS)),)
vpath %.c $(shell find $(TEST_SOURCE_DIR) -type d | tr '\n' :)
TEST_SOURCES := $(shell find $(TEST_SOURCE_DIR) -type f \( -name '*.c' -or -name '*.C' \))
TEST_OBJECTS := $(foreach s, $(TEST_SOURCES), $(TEST_OBJECT_DIR)/$(basename $(notdir $(s))).o)
vpath %.h $(TEST_INCLUDE_DIR)

TEST_DEPENDS := $(foreach s, $(TEST_SOURCES), $(GENERATED_TEST_DEPEND_DIR)/$(basename $(notdir $(s))).d)
endif

vpath %.a $(TARGET_DIR)

# compatibility target
all: help

# compatibility target
check: test

help:
	@echo "usage: make <goal>"
	@echo ""
	@echo "The value of <goal> can be one of: clean compile target test package install"
	@echo ""
	@echo "clean    - remove all build artifacts"
	@echo "compile  - build object files"
	@echo "target   - build the target library or program"
	@echo "test     - build and run the test harness"
	@echo "package  - collect the target artifacts info a distributable bundle"
	@echo "install  - install the target artifacts onto the local system"

ifneq ($(MAKECMDGOALS),clean)
include $(DEPENDS)
endif

ifeq ($(strip $(SKIP_TESTS)),)
ifneq ($(MAKECMDGOALS),clean)
include $(TEST_DEPENDS)
endif
endif

$(GENERATED_DEPEND_DIR):
	@mkdir -p $(GENERATED_DEPEND_DIR)

$(GENERATED_DEPEND_DIR)/%.d: %.c | $(GENERATED_DEPEND_DIR)
	@$(CC) -MM -MG -MT '$(OBJECT_DIR)/$(*F).o $@' $(CFLAGS) $(CDEFS) $< > $@

$(GENERATED_TEST_DEPEND_DIR):
	@mkdir -p $(GENERATED_TEST_DEPEND_DIR)

$(GENERATED_TEST_DEPEND_DIR)/%.d: %.c | $(GENERATED_TEST_DEPEND_DIR)
	@$(CC) -MM -MG -MT '$(TEST_OBJECT_DIR)/$(*F).o $@' $(TEST_CFLAGS) $(CDEFS) $< > $@

$(OBJECT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(CDEFS) -c $< -o $@

$(TEST_OBJECT_DIR)/%.o: %.c
	$(CC) $(TEST_CFLAGS) $(CDEFS) -c $< -o $@

$(LIBRARY_TARGET): $(OBJECTS)
	@echo ""
	@echo " -- Builing library $(LIBRARY_TARGET)"
	@echo "------------------------------------------------------------------------"
	$(AR) rcs $(LIBRARY_TARGET) $?

$(PROGRAM_TARGET): $(LIBRARY_TARGET)
	@echo ""
	@echo " -- Building program $(PROGRAM_TARGET)"
	@echo "------------------------------------------------------------------------"
	$(CC) -o $(PROGRAM_TARGET) $(LDLIBS) -L$(TARGET_DIR) -l$(LIBRARY_NAME_BASE)

ifeq ($(strip $(SKIP_TESTS)),)
$(TEST_PROGRAM_TARGET): $(LIBRARY_TARGET) $(TEST_OBJECTS)
	@echo ""
	@echo " -- Building test harness $(TEST_PROGRAM_TARGET)"
	@echo "------------------------------------------------------------------------"
	$(CC) -o $(TEST_PROGRAM_TARGET) $(TEST_LDLIBS) -L$(TARGET_DIR) -l$(LIBRARY_NAME_BASE) $(wildcard $(TEST_OBJECT_DIR)/*.o)
endif

clean:
	@echo ""
	@echo " Deleting directory `pwd`/$(TARGET_DIR)"
	@rm -rf $(TARGET_DIR)

validate:
ifeq ($(strip $(GROUP_ID)),)
	$(error "Please set a value for GROUP_ID in project.mk")
endif
ifeq ($(strip $(ARTIFACT_ID)),)
	$(error "Please set a value for ARTIFACT_ID in project.mk")
endif
ifeq ($(PACKAGING),program)
TARGET = $(PROGRAM_TARGET)
else ifeq ($(PACKAGING),library)
TARGET = $(LIBRARY_TARGET)
else
$(error "Unsupported value of PACKAGING: $(PACKAGING)")
endif

initialize: validate
	@echo ""
	@echo " Buidling $(GROUP_ID):$(ARTIFACT_ID):$(VERSION)"
	@mkdir -p $(OBJECT_DIR)
	@mkdir -p $(TEST_OBJECT_DIR)
	@mkdir -p $(GENERATED_DEPEND_DIR)
	@mkdir -p $(GENERATED_TEST_DEPEND_DIR)

announce-compile-phase:
	@echo ""
	@echo "------------------------------------------------------------------------"
	@echo " Compile phase"
	@echo "------------------------------------------------------------------------"


# xxx - add some way to generate the version.h file
generate-sources: initialize announce-compile-phase $(DEPENDS)

process-sources: generate-sources

generate-resources: process-sources

process-resources: generate-resources

announce-compile-sources:
	@echo ""
	@echo " -- Compiling sources"
	@echo "------------------------------------------------------------------------"

compile: process-resources announce-compile-sources $(OBJECTS)

process-objects: compile $(LIBRARY_TARGET)

target: process-objects $(TARGET)

announce-test-phase:
	@echo ""
	@echo "------------------------------------------------------------------------"
	@echo " Test phase"
	@echo "------------------------------------------------------------------------"

generate-test-sources: target announce-test-phase

process-test-sources: generate-test-sources

generate-test-resources: process-test-sources

process-test-resources: generate-test-resources

announce-compile-test-sources:
ifeq ($(strip $(SKIP_TESTS)),)
	@echo ""
	@echo " -- Compiling test sources"
	@echo "------------------------------------------------------------------------"
endif

test-compile: process-test-resources announce-compile-test-sources $(TEST_OBJECTS)

process-test-objects: test-compile $(TEST_PROGRAM_TARGET)

test: process-test-objects
ifeq ($(strip $(SKIP_TESTS)),)
	@echo ""
	@echo " -- Executing test harness"
	@echo "------------------------------------------------------------------------"
	@$(TEST_PROGRAM_TARGET)
else
	@echo ""
	@echo " -- Skipping tests"
	@echo "------------------------------------------------------------------------"
endif

announce-package-phase:
	@echo ""
	@echo "------------------------------------------------------------------------"
	@echo " Package phase"
	@echo "------------------------------------------------------------------------"

prepare-package: test announce-package-phase

package: prepare-package
	$(error "Not implemented yet")

announce-install-phase:
	@echo ""
	@echo "------------------------------------------------------------------------"
	@echo " Install phase"
	@echo "------------------------------------------------------------------------"

verify: test announce-install-phase

install: verify
	$(error "Not implemented yet")

.PHONY: all check help clean initialize announce-compile-phase generate-sources process-sources generate-resources process-resources announce-compile-sources compile process-objects announce-test-phase generate-test-sources process-test-sources generate-test-resources process-test-resources announce-compile-test-sources test-compile process-test-objects test announce-package-phase prepare-package package verify announce-install-phase install
