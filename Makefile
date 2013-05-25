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

## Defaults for common shell commands
CC = cc
AR = ar
RM = rm -f
DIFF = diff
TAR = tar
FIND = find
INSTALL = install

## Defaults for project settings
version ?= 1.0.0-SNAPSHOT
artifact ?= program
build ?= debug
INCLUDES ?=
CFLAGS ?=
DEPENDENCIES ?=
TEST_DEPENDENCIES ?=

## Defaults for lifecycle hooks
GENERATE_SOURCES_HOOKS ?=
PROCESS_SOURCES_HOOKS ?=
GENERATE_RESOURCES_HOOKS ?=
PROCESS_SOURCES_HOOKS ?=
GENERATE_TEST_SOURCES_HOOKS ?=
PROCESS_TEST_SOURCES_HOOKS ?=
GENERATE_TEST_RESOURCES_HOOKS ?=
PROCESS_TEST_SOURCES_HOOKS ?=

## Defaults for project source directories
SOURCE_DIR  ?= src/main/c
INCLUDE_DIR ?= $(SOURCE_DIR)/include
RESOURCE_DIR ?= src/main/resources

## Defaults for project test source directories
TEST_SOURCE_DIR  ?= src/test/c
TEST_INCLUDE_DIR ?= $(TEST_SOURCE_DIR)/include
TEST_RESOURCE_DIR ?= src/test/resources

## Defaults for project output directories
TARGET_DIR  ?= target
OBJECT_DIR  ?= $(TARGET_DIR)/objects
TEST_OBJECT_DIR  ?= $(TARGET_DIR)/test-objects
GENERATED_SOURCE_DIR ?= $(TARGET_DIR)/generated-sources
GENERATED_HEADERS_DIR ?= $(GENERATED_SOURCE_DIR)/include
GENERATED_DEPEND_DIR ?= $(GENERATED_SOURCE_DIR)/depend
GENERATED_TEST_SOURCE_DIR ?= $(TARGET_DIR)/generated-test-sources
GENERATED_TEST_DEPEND_DIR ?= $(TARGET_DIR)/generated-test-sources/depend

## Project target artifact settings
PROGRAM_NAME ?= $(package)
PROGRAM_TARGET = $(TARGET_DIR)/$(PROGRAM_NAME)
LIBRARY_NAME_BASE ?= $(package)
LIBRARY_NAME ?= lib$(LIBRARY_NAME_BASE).a
LIBRARY_TARGET = $(TARGET_DIR)/$(LIBRARY_NAME)
ifeq ($(strip $(skip_tests)),)
TEST_PROGRAM = $(package)_test
TEST_PROGRAM_TARGET = $(TARGET_DIR)/$(TEST_PROGRAM)
endif

ifeq ($(build),debug)
debug_CFLAGS := $(debug_CFLAGS) -g
endif

## Project installation directories (according to GNU conventions)
srcdir = $(SOURCE_DIR)
mansrcdir = $(srcdir)/man
infosrcdir = $(srcdir)/info
htmlsrcdir = $(srcdir)/html
pdfsrcdir = $(srcdir)/pdf
pssrcdir = $(srcdir)/ps
dvisrcdir = $(srcdir)/dvi

prefix ?= /usr/local

exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
sbindir = $(exec_prefix)/sbin
libexecdir = $(exec_prefix)/libexec
libdir = $(exec_prefix)/lib
package_libexecdir = $(libexecdir)/$(package)/$(version)

datarootdir = $(prefix)/share
datadir = $(datarootdir)
package_datadir = $(datadir)/$(package)
lispdir = $(datarootdir)/emacs/site-lisp
localedir = $(datarootdir)/locale

mandir = $(datarootdir)/man
man1dir = $(mandir)/man1
man2dir = $(mandir)/man2
man3dir = $(mandir)/man3
man4dir = $(mandir)/man4
man5dir = $(mandir)/man5
man6dir = $(mandir)/man6
man7dir = $(mandir)/man7

man1ext = .1
man2ext = .2
man3ext = .3
man4ext = .4
man5ext = .5
man6ext = .6
man6ext = .7
manext = $(man1ext)

sysconfdir = $(prefix)/etc

sharedstatedir = $(prefix)/com
localstatedir = $(prefix)/var
logdir = $(localstatedir)/log
package_logdir = $(logdir)/$(package)
rundir = $(localstatedir)/run
package_rundir = $(rundir)/$(package)
tmpdir = $(localstatedir)/tmp

includedir = $(prefix)/include
oldincludedir =

docdir = $(datarootdir)/doc/$(package)
infodir = $(datarootdir)/info
htmldir = $(docdir)
dvidir = $(docdir)
pdfdir = $(docdir)
psdir = $(docdir)

## Project source compiler settings
DEPENDENCY_VALIDATIONS := $(addprefix dependency/,$(DEPENDENCIES) $(TEST_DEPENDENCIES))
INCLUDES := $(INCLUDES) -I$(GENERATED_HEADERS_DIR)
CFLAGS := -I$(INCLUDE_DIR) $(INCLUDES) $(CFLAGS) $($(build)_CFLAGS)
LDLIBS := $(addprefix -l, $(DEPENDENCIES))

## Automation helper functions
source_to_target = $(foreach s, $(1), $(2)/$(basename $(s)).$(3))
source_to_object = $(call source_to_target,$(1),$(2),o)
source_to_depend = $(call source_to_target,$(1),$(2),d)
find_source_files = $(shell cd $(1) && $(FIND) . -type f \( -name '*.c' -or -name '*.C' \) | sed 's|\./||')

## Project test source compiler settings
TEST_CFLAGS := -I$(TEST_INCLUDE_DIR) $(CFLAGS) $(TEST_CFLAGS)
TEST_LDLIBS := $(LDLIBS) $(addprefix -l, $(TEST_DEPENDENCIES))

## Project source file locations
SOURCES := $(call find_source_files,$(SOURCE_DIR))
OBJECTS := $(call source_to_object,$(SOURCES),$(OBJECT_DIR))
PROGRAM_SOURCES ?= $(shell grep -l -E '(int|void)\s+main' $(addprefix $(SOURCE_DIR)/,$(SOURCES)) | sed 's|$(SOURCE_DIR)/||')
PROGRAM_OBJECTS := $(call source_to_object,$(PROGRAM_SOURCES),$(OBJECT_DIR))
LIBRARY_OBJECTS := $(filter-out $(PROGRAM_OBJECTS),$(OBJECTS))
vpath %.c $(SOURCE_DIR)
vpath %.h $(INCLUDE_DIR)
DEPENDS := $(call source_to_depend,$(SOURCES),$(GENERATED_DEPEND_DIR))

## Project test source file locations
ifeq ($(strip $(skip_tests)),)
TEST_SOURCES := $(call find_source_files,$(TEST_SOURCE_DIR))
TEST_OBJECTS := $(call source_to_object,$(TEST_SOURCES),$(TEST_OBJECT_DIR))
vpath %.c $(TEST_SOURCE_DIR)
vpath %.h $(TEST_INCLUDE_DIR)
TEST_DEPENDS := $(call source_to_depend,$(TEST_SOURCES),$(GENERATED_TEST_DEPEND_DIR))
endif

vpath %.a $(TARGET_DIR)

## Define the build target type
ifeq ($(artifact),program)
TARGET = $(PROGRAM_TARGET)
else ifeq ($(artifact),library)
TARGET = $(LIBRARY_TARGET)
else
$(error "Unsupported value of 'artifact': $(artifact)")
endif

## Include generated dependency rules
# N.B. - the dependency makefiles should not be implicity generated on the first run
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(strip $(shell if [ -d $(GENERATED_DEPEND_DIR) ]; then echo "true"; fi)),true)
-include $(DEPENDS)
endif
endif

ifeq ($(strip $(skip_tests)),)
ifeq ($(strip $(shell if [ -d $(GENERATED_TEST_DEPEND_DIR) ]; then echo "true"; fi)),true)
ifneq ($(MAKECMDGOALS),clean)
-include $(TEST_DEPENDS)
endif
endif
endif

# Compatibility target
all: help

# Compatibility target
check: test

help:
	@echo "usage: make <goal>"; \
	echo ""; \
	echo "The value of <goal> can be one of:"; \
	echo ""; \
	echo "clean    - remove all build artifacts"; \
	echo "compile  - build object files"; \
	echo "target   - build the target library or program"; \
	echo "test     - build and run the test harness"; \
	echo "package  - collect the target artifacts info a distributable bundle"; \
	echo "install  - install the target artifacts onto the local system"

## Suport Emacs flymake syntax checker
check-syntax: create-build-directories $(GENERATE_SOURCES_HOOKS) $(GENERATE_TEST_SOURCES_HOOKS)
	$(CC) $(TEST_CFLAGS) $(CDEFS) -fsyntax-only $(CHK_SOURCES)

## Confirm the avialability of one library dependency
dependency/%: in:=$(shell mktemp -t dependencyXXXXXX).c
dependency/%:
ifeq ($(strip $(DEPENDENCY_HELPER)),)
	@$(RM) $(in)
	@echo "#include <$(@F).h>" > $(in); echo "int main(void) {return 0;}" >> $(in); \
	$(CC) $(in) -l$(@F) -o `mktemp -t objXXXXXX`; \
	if [ "0" != "$$?" ]; then echo "build: *** The dependency \"$(@F)\" was not found."; exit 1; fi
else
	$(DEPENDENCY_HELPER) $(@F)
endif

## Generate depened rule files
$(GENERATED_HEADERS_DIR):
	@mkdir -p $(GENERATED_HEADERS_DIR)

$(GENERATED_DEPEND_DIR):
	@mkdir -p $(GENERATED_DEPEND_DIR)

$(GENERATED_DEPEND_DIR)/%.d: %.c | $(GENERATED_DEPEND_DIR)
	@mkdir -p $(dir $@)
	@$(CC) -MM -MG -MT '$(OBJECT_DIR)/$(*F).o $@' $(CFLAGS) $(CDEFS) $< > $@

$(GENERATED_TEST_DEPEND_DIR):
	@mkdir -p $(GENERATED_TEST_DEPEND_DIR)

$(GENERATED_TEST_DEPEND_DIR)/%.d: %.c | $(GENERATED_TEST_DEPEND_DIR)
	@$(CC) -MM -MG -MT '$(TEST_OBJECT_DIR)/$(*F).o $@' $(TEST_CFLAGS) $(CDEFS) $< > $@

## Main build rules
$(OBJECT_DIR):
	@mkdir -p $(OBJECT_DIR)

$(TEST_OBJECT_DIR):
	@mkdir -p $(TEST_OBJECT_DIR)

$(OBJECT_DIR)/%.o: %.c | $(OBJECT_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CDEFS) -c $< -o $@

$(TEST_OBJECT_DIR)/%.o: %.c  | $(TEST_OBJECT_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) $(CDEFS) -c $< -o $@

$(LIBRARY_TARGET): $(LIBRARY_OBJECTS)
	@echo ""
	@echo " -- Builing library $(LIBRARY_TARGET)"
	@echo "------------------------------------------------------------------------"
	$(AR) rcs $(LIBRARY_TARGET) $?

$(LIBRARY_NAME): $(LIBRARY_TARGET)

$(PROGRAM_TARGET): $(LIBRARY_TARGET) $(PROGRAM_OBJECTS)
	@echo ""
	@echo " -- Building program $(PROGRAM_TARGET)"
	@echo "------------------------------------------------------------------------"
	$(CC) -o $(PROGRAM_TARGET) $(LDLIBS) -L$(TARGET_DIR) -l$(LIBRARY_NAME_BASE) $(PROGRAM_OBJECTS)

$(PROGRAM_NAME): $(PROGRAM_TARGET)

ifeq ($(strip $(skip_tests)),)
$(TEST_PROGRAM_TARGET): $(LIBRARY_TARGET) $(TEST_OBJECTS)
	@echo ""
	@echo " -- Building test harness $(TEST_PROGRAM_TARGET)"
	@echo "------------------------------------------------------------------------"
	$(CC) -o $(TEST_PROGRAM_TARGET) $(TEST_LDLIBS) -L$(TARGET_DIR) -l$(LIBRARY_NAME_BASE) $(TEST_OBJECTS)

$(TEST_PROGRAM): $(TEST_PROGRAM_TARGET)
endif

clean:
	@echo ""
	@echo " Deleting directory `pwd`/$(TARGET_DIR)"
	@$(RM) -r $(TARGET_DIR)

validate:
ifeq ($(strip $(owner)),)
	$(error "Please set a value for 'owner' in project.mk")
endif
ifeq ($(strip $(package)),)
	$(error "Please set a value for 'package' in project.mk")
endif

ensure-dependencies: $(DEPENDENCY_VALIDATIONS)

announce-build:
	@echo ""
	@echo " Buidling $(owner):$(package):$(version)"

create-build-directories:
	@mkdir -p $(OBJECT_DIR)
	@mkdir -p $(TEST_OBJECT_DIR)
	@mkdir -p $(GENERATED_DEPEND_DIR)
	@mkdir -p $(GENERATED_TEST_DEPEND_DIR)

initialize: validate ensure-dependencies announce-build create-build-directories

announce-compile-phase:
	@echo ""
	@echo "------------------------------------------------------------------------"
	@echo " Compile phase"
	@echo "------------------------------------------------------------------------"

announce-generate-sources:
ifneq ($(strip $(GENERATE_SOURCES_HOOKS)),)
	@echo ""
	@echo " -- Generating sources"
	@echo "------------------------------------------------------------------------"
endif

generate-sources: initialize announce-compile-phase announce-generate-sources $(GENERATE_SOURCES_HOOKS) $(DEPENDS)

process-sources: generate-sources $(PROCESS_SOURCES_HOOKS)

generate-resources: process-sources $(GENERATE_RESOURCES_HOOKS)

process-resources: count = $(shell if [ -d $(RESOURCE_DIR) ]; then ls $(RESOURCE_DIR) | wc -l; fi)
process-resources: generate-resources $(PROCESS_RESOURCES_HOOKS)
ifeq ($(shell if [ -d $(RESOURCE_DIR) ]; then echo "true"; fi),true)
	@echo ""
	@echo " -- Copying resources..."
	@echo "------------------------------------------------------------------------"
	@echo "Copying $(strip $(count)) files to $(TARGET_DIR)"
	@cp -r $(RESOURCE_DIR)/* $(TARGET_DIR)
endif

announce-compile-sources:
	@echo ""
	@echo " -- Compiling sources"
	@echo "------------------------------------------------------------------------"

compile: process-resources announce-compile-sources $(OBJECTS)

process-objects: compile

library: process-objects $(LIBRARY_TARGET)

target: library $(TARGET)

announce-test-phase:
	@echo ""
	@echo "------------------------------------------------------------------------"
	@echo " Test phase"
	@echo "------------------------------------------------------------------------"

generate-test-sources: target announce-test-phase $(GENERATE_TEST_SOURCES_HOOKS)

process-test-sources: generate-test-sources $(PROCESS_TEST_SOURCES_HOOKS)

generate-test-resources: process-test-sources $(GENERATE_TEST_RESOURCES_HOOKS)

process-test-resources: count = $(shell if [ -d $(TEST_RESOURCE_DIR) ]; then ls $(TEST_RESOURCE_DIR) | wc -l; fi)
process-test-resources: generate-test-resources $(PROCESS_TEST_RESOURCES_HOOKS)
ifeq ($(shell if [ -d $(TEST_RESOURCE_DIR) ]; then echo "true"; fi),true)
	@echo ""
	@echo " -- Copying test resources..."
	@echo "------------------------------------------------------------------------"
	@echo "Copying $(strip $(count)) files to $(TARGET_DIR)"
	@cp -r $(TEST_RESOURCE_DIR)/* $(TARGET_DIR)
endif

announce-compile-test-sources:
ifeq ($(strip $(skip_tests)),)
	@echo ""
	@echo " -- Compiling test sources"
	@echo "------------------------------------------------------------------------"
endif

test-compile: process-test-resources announce-compile-test-sources $(TEST_OBJECTS)

process-test-objects: test-compile

test-target: library process-test-objects $(TEST_PROGRAM_TARGET)

test: test-target
ifeq ($(strip $(skip_tests)),)
	@echo ""
	@echo " -- Executing test harness"
	@echo "------------------------------------------------------------------------"
	@cd $(TARGET_DIR); ./$(TEST_PROGRAM)
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

announce-install-phase:
	@echo ""
	@echo "------------------------------------------------------------------------"
	@echo " Install phase"
	@echo "------------------------------------------------------------------------"

verify: package announce-install-phase

install: verify
	$(INSTALL) -d -m 755 $(DESTDIR)$(bindir)
	$(INSTALL) $(TARGET) $(DESTDIR)$(bindir)

.PHONY: all check help check-syntax clean create-buid-directories announce-build initialize announce-compile-phase generate-sources process-sources generate-resources process-resources announce-compile-sources compile process-objects target announce-test-phase generate-test-sources process-test-sources generate-test-resources process-test-resources announce-compile-test-sources test-compile process-test-objects test-target test announce-package-phase prepare-package package verify announce-install-phase install $(PROGRAM_NAME) $(LIBRARY_NAME) $(TEST_PROGRAM) $(GENERATE_SOURCES_HOOKS) $(PROCESS_SOURCES_HOOKS) $(GENERATE_RESOURCES_HOOKS) $(PROCESS_SOURCES_HOOKS) $(GENERATE_TEST_SOURCES_HOOKS) $(PROCESS_TEST_SOURCES_HOOKS) $(GENERATE_TEST_RESOURCES_HOOKS) $(PROCESS_TEST_SOURCES_HOOKS) $(VALIDATION_HOOKS) $(DEPENDENCY_VADLIDATIONS)
