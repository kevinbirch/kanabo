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

# Compatibility target
all: help

PROJECT_CONFIG_FILE ?= project.mk

include $(PROJECT_CONFIG_FILE)

## Defaults for common shell commands
CC ?= cc
AR ?= ar
RM ?= rm -f
DIFF ?= diff
TAR ?= tar
FIND ?= find
INSTALL ?= install
MKTEMP ?= mktemp

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
SOURCE_DIR ?= src
MAIN_DIR ?= $(SOURCE_DIR)/main
C_SOURCES_DIR ?= $(MAIN_DIR)/c
CC_SOURCES_DIR ?= $(MAIN_DIR)/c++
M_SOURCES_DIR ?= $(MAIN_DIR)/m
C_INCLUDE_DIR ?= $(C_SOURCES_DIR)/include
CC_INCLUDE_DIR ?= $(CC_SOURCES_DIR)/include
M_INCLUDE_DIR ?= $(M_SOURCES_DIR)/include
RESOURCES_DIR ?= $(MAIN_DIR)/resources

## Defaults for project test source directories
TEST_DIR ?= $(SOURCE_DIR)/test
C_TEST_SOURCES_DIR ?= $(TEST_DIR)/c
CC_TEST_SOURCES_DIR ?= $(TEST_DIR)/c++
M_TEST_SOURCES_DIR ?= $(TEST_DIR)/m
C_TEST_INCLUDE_DIR ?= $(C_TEST_SOURCES_DIR)/include
CC_TEST_INCLUDE_DIR ?= $(CC_TEST_SOURCES_DIR)/include
M_TEST_INCLUDE_DIR ?= $(M_TEST_SOURCES_DIR)/include
TEST_RESOURCES_DIR ?= $(TEST_DIR)/resources

## Defaults for project output directories
TARGET_DIR  ?= target
OBJECT_DIR  ?= $(TARGET_DIR)/objects
TEST_OBJECT_DIR  ?= $(TARGET_DIR)/test-objects
GENERATED_SOURCE_DIR ?= $(TARGET_DIR)/generated-sources
GENERATED_HEADERS_DIR ?= $(GENERATED_SOURCE_DIR)/include
GENERATED_DEPEND_DIR ?= $(GENERATED_SOURCE_DIR)/depend
GENERATED_TEST_SOURCE_DIR ?= $(TARGET_DIR)/generated-test-sources
GENERATED_TEST_DEPEND_DIR ?= $(TARGET_DIR)/generated-test-sources/depend
RESOURCES_TARGET_DIR ?= $(TARGET_DIR)/resources
TEST_RESOURCES_TARGET_DIR ?= $(TARGET_DIR)/test-resources

## Set this variable to any value to skip executing the test harness
skip_tests ?=

## Project target artifact settings
PROGRAM_NAME ?= $(package)
PROGRAM_TARGET = $(TARGET_DIR)/$(PROGRAM_NAME)
LIBRARY_NAME_BASE ?= $(package)
LIBRARY_NAME ?= lib$(LIBRARY_NAME_BASE).a
LIBRARY_TARGET = $(TARGET_DIR)/$(LIBRARY_NAME)
TEST_PROGRAM = $(package)_test
TEST_PROGRAM_TARGET = $(TARGET_DIR)/$(TEST_PROGRAM)

## Build mode
## Set this to the CFLAGS to be used in release build mode
release_CFLAGS ?=

## Set this to the CFLAGS to be used in debug build mode
debug_CFLAGS ?= -g

ifeq ($(build),debug)
else ifeq ($(build),release)
else
$(error "Unsupported value of `build`: '$(build)', must be 'debug' or 'release'")
endif

## Project installation directories (according to GNU conventions)
srcdir := $(SOURCE_DIR)
mansrcdir := $(srcdir)/man
infosrcdir := $(srcdir)/info
htmlsrcdir := $(srcdir)/html
pdfsrcdir := $(srcdir)/pdf
pssrcdir := $(srcdir)/ps
dvisrcdir := $(srcdir)/dvi

prefix ?= /usr/local

exec_prefix := $(prefix)
bindir := $(exec_prefix)/bin
sbindir := $(exec_prefix)/sbin
libexecdir := $(exec_prefix)/libexec
libdir := $(exec_prefix)/lib
package_libexecdir := $(libexecdir)/$(package)/$(version)

datarootdir := $(prefix)/share
datadir := $(datarootdir)
package_datadir := $(datadir)/$(package)
lispdir := $(datarootdir)/emacs/site-lisp
localedir := $(datarootdir)/locale

mandir := $(datarootdir)/man
man1dir := $(mandir)/man1
man2dir := $(mandir)/man2
man3dir := $(mandir)/man3
man4dir := $(mandir)/man4
man5dir := $(mandir)/man5
man6dir := $(mandir)/man6
man7dir = $(mandir)/man7

man1ext = .1
man2ext = .2
man3ext = .3
man4ext = .4
man5ext = .5
man6ext = .6
man6ext = .7
manext := $(man1ext)

sysconfdir := $(prefix)/etc

sharedstatedir := $(prefix)/com
localstatedir := $(prefix)/var
logdir := $(localstatedir)/log
package_logdir := $(logdir)/$(package)
rundir := $(localstatedir)/run
package_rundir := $(rundir)/$(package)
tmpdir := $(localstatedir)/tmp

includedir := $(prefix)/include
oldincludedir :=

docdir := $(datarootdir)/doc/$(package)
infodir := $(datarootdir)/info
htmldir := $(docdir)
dvidir := $(docdir)
pdfdir := $(docdir)
psdir := $(docdir)

## Hooks
GENERATE_SOURCES_HOOKS ?=
GENERATE_TEST_SOURCES_HOOKS ?=
PROCESS_SOURCES_HOOKS ?=
PROCESS_TEST_SOURCES_HOOKS ?=
GENERATE_RESOURCES_HOOKS ?=
GENERATE_TEST_RESOURCES_HOOKS ?=
PROCESS_RESOURCES_HOOKS ?=
PROCESS_TEST_RESOURCES_HOOKS ?=
DEPENDENCY_HOOK ?=
TEST_DEPENDENCY_HOOK ?= $(DEPENDENCY_HOOK)

## Dependency handling
DEPENDENCY_VALIDATIONS := $(addprefix dependency/,$(DEPENDENCIES))
DEPENDENCY_INCLUDES ?=
DEPENDENCY_LDFLAGS ?=
TEST_DEPENDENCY_VALIDATIONS := $(addprefix test-dependency/,$(TEST_DEPENDENCIES))
TEST_DEPENDENCY_INCLUDES ?= $(DEPENDENCY_INCLUDES)
TEST_DEPENDENCY_LDFLAGS ?= $(DEPENDENCY_LDFLAGS)

## Project compiler settings
INCLUDES := $(INCLUDES) -I$(GENERATED_HEADERS_DIR) -I$(C_INCLUDE_DIR)
CFLAGS := $(CFLAGS) $($(build)_CFLAGS)
LDFLAGS ?=
LDLIBS ?=

## Project test compiler settings
TEST_INCLUDES ?= $(INCLUDES) -I$(C_TEST_INCLUDE_DIR)
TEST_CFLAGS ?= $(CFLAGS)
TEST_LDFLAGS ?= $(LDFLAGS)
TEST_LDLIBS ?= $(LDLIBS)

## Automation helper functions
source_to_target = $(foreach s, $(1), $(2)/$(basename $(s)).$(3))
source_to_object = $(call source_to_target,$(1),$(2),o)
source_to_depend = $(call source_to_target,$(1),$(2),d)
find_source_files = $(shell cd $(1) && $(FIND) . -type f \( -name '*.c' -or -name '*.C' \) | sed 's|\./||')

## Project source file locations
SOURCES := $(call find_source_files,$(C_SOURCES_DIR))
OBJECTS := $(call source_to_object,$(SOURCES),$(OBJECT_DIR))
PROGRAM_SOURCES ?= $(shell grep -l -E '(int|void)\s+main' $(addprefix $(C_SOURCES_DIR)/,$(SOURCES)) | sed 's|$(C_SOURCES_DIR)/||')
PROGRAM_OBJECTS := $(call source_to_object,$(PROGRAM_SOURCES),$(OBJECT_DIR))
LIBRARY_OBJECTS := $(filter-out $(PROGRAM_OBJECTS),$(OBJECTS))
vpath %.c $(C_SOURCES_DIR)
vpath %.h $(C_INCLUDE_DIR)
DEPENDS := $(call source_to_depend,$(SOURCES),$(GENERATED_DEPEND_DIR))
find_resources = $(foreach r, $(wildcard $(1)/*), $(subst $(1),$(2),$(r)))
RESOURCES := $(call find_resources,$(RESOURCES_DIR),$(RESOURCES_TARGET_DIR))

ifneq (1,$(words $(PROGRAM_SOURCES)))
$(warning "Multiple sources containing `main' detected: $(PROGRAM_SOURCES)")
endif

## Project test source file locations
TEST_SOURCES := $(call find_source_files,$(C_TEST_SOURCES_DIR))
TEST_OBJECTS := $(call source_to_object,$(TEST_SOURCES),$(TEST_OBJECT_DIR))
vpath %.c $(C_TEST_SOURCES_DIR)
vpath %.h $(C_TEST_INCLUDE_DIR)
TEST_DEPENDS := $(call source_to_depend,$(TEST_SOURCES),$(GENERATED_TEST_DEPEND_DIR))
TEST_RESOURCES := $(call find_resources,$(TEST_RESOURCES_DIR),$(TEST_RESOURCES_TARGET_DIR))

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

ifeq ($(strip $(shell if [ -d $(GENERATED_TEST_DEPEND_DIR) ]; then echo "true"; fi)),true)
ifneq ($(MAKECMDGOALS),clean)
-include $(TEST_DEPENDS)
endif
endif

# Compatibility target
check: test

define help =
usage: make <goal>

The value of <goal> can be one of:

clean    - remove all build artifacts
compile  - build object files
target   - build the target library or program
test     - build and run the test harness
package  - collect the target artifacts info a distributable bundle
install  - install the target artifacts onto the local system
endef

define build_message =

 Buidling $(owner):$(package):$(version))
endef

define announce_phase_message =

------------------------------------------------------------------------
 $(1) phase
------------------------------------------------------------------------
endef

define announce_section_message =

 -- $(1)
------------------------------------------------------------------------

endef

define announce_section_detail_message =

 -- $(1)
------------------------------------------------------------------------
$(2)

endef

help:
	@$(info $(help))
	@:

## Suport Emacs flymake syntax checker
check-syntax: create-build-directories $(GENERATE_SOURCES_HOOKS) $(GENERATE_TEST_SOURCES_HOOKS)
	$(CC) $(TEST_CFLAGS) -fsyntax-only $(TEST_INCLUDES) $(INCLUDES) $(CHK_SOURCES)

define define_dependency_variables =
 $(dependency_prefix)DEPENDENCY_$$(@F)_INCLUDES ?= $$($$(dependency_prefix)DEPENDENCY_INCLUDES)
 $(dependency_prefix)DEPENDENCY_$$(@F)_LDFLAGS ?= $$($$(dependency_prefix)DEPENDENCY_LDFLAGS)
 $(dependency_prefix)DEPENDENCY_$$(@F)_HEADER ?= $(@F).h
 $(dependency_prefix)DEPENDENCY_$$(@F)_LIB ?= $(@F)
 $(dependency_prefix)LDLIBS += -l$$($(dependency_prefix)DEPENDENCY_$(@F)_LIB)
 dependency_$$(@F)_infile := $$(shell $(MKTEMP) -t dependency_$(@F)_XXXXXX.c)
 dependency_$$(@F)_outfile := $$(shell $(MKTEMP) -t dependency_$(@F)_XXXXXX.o)
endef

define dependency_test_template =
#include <$($(dependency_prefix)DEPENDENCY_$(@F)_HEADER)>
int main(void) {return 0;}
endef

ifeq ($(strip $(DEPENDENCY_HOOK)),)
define dependency_test_canned_recipe =
@$(info resolving depencency: $(@F))
@$(eval $(define_dependency_variables))
@$(file > $(dependency_$(@F)_infile),$(dependency_test_template))
@$(CC) $($(dependency_prefix)DEPENDENCY_$(@F)_INCLUDES) $(dependency_$(@F)_infile) $($(dependency_prefix)DEPENDENCY_$(@F)_LDFLAGS) -l$($(dependency_prefix)DEPENDENCY_$(@F)_LIB) -o $(dependency_$(@F)_outfile); \
	if [ "0" != "$$?" ]; \
	  then echo "build: *** The dependency \"$(@F)\" was not found."; \
	  exit 1; \
	fi
endef
else
define dependency_test_canned_recipe =
@$(info invoking depencency hook: $(DEPENDENCY_HOOK))
@$(DEPENDENCY_HOOK) $(@F)
endef
endif

## Confirm the availability of one dependency
dependency/%:
	$(dependency_test_canned_recipe)

## Confirm the availability of one test dependency
test-dependency/%: dependency_prefix := TEST_
test-dependency/%:
	$(dependency_test_canned_recipe)

## Generate depened rule files
$(GENERATED_HEADERS_DIR):
	@mkdir -p $(GENERATED_HEADERS_DIR)

$(GENERATED_DEPEND_DIR):
	@mkdir -p $(GENERATED_DEPEND_DIR)

$(GENERATED_DEPEND_DIR)/%.d: %.c | $(GENERATED_DEPEND_DIR)
	@mkdir -p $(dir $@)
	@$(CC) -MM -MG -MT '$(OBJECT_DIR)/$(*F).o $@' $(CFLAGS) $(INCLUDES) $< > $@

$(GENERATED_TEST_DEPEND_DIR):
	@mkdir -p $(GENERATED_TEST_DEPEND_DIR)

$(GENERATED_TEST_DEPEND_DIR)/%.d: %.c | $(GENERATED_TEST_DEPEND_DIR)
	@mkdir -p $(dir $@)
	@$(CC) -MM -MG -MT '$(TEST_OBJECT_DIR)/$(*F).o $@' $(TEST_CFLAGS) $(TEST_INCLUDES) $< > $@

## Main build rules
$(OBJECT_DIR):
	@mkdir -p $(OBJECT_DIR)

$(TEST_OBJECT_DIR):
	@mkdir -p $(TEST_OBJECT_DIR)

$(OBJECT_DIR)/%.o: %.c | $(OBJECT_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_OBJECT_DIR)/%.o: %.c | $(TEST_OBJECT_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) $(TEST_INCLUDES) -c $< -o $@

$(RESOURCES_TARGET_DIR)/%: $(RESOURCES_DIR)/%
	@mkdir -p $(dir $@)
	cp -r $< $(RESOURCES_TARGET_DIR)

$(TEST_RESOURCES_TARGET_DIR)/%: $(TEST_RESOURCES_DIR)/%
	@mkdir -p $(dir $@)
	cp -r $< $(TEST_RESOURCES_TARGET_DIR)

$(LIBRARY_TARGET): $(LIBRARY_OBJECTS)
	@$(info $(call announce_section_detail_message,Builing library,Creating $(LIBRARY_TARGET)))
	$(AR) rcs $(LIBRARY_TARGET) $(shell $(FIND) $(OBJECT_DIR) -type f -name '*.o')

$(LIBRARY_NAME): $(LIBRARY_TARGET)

$(PROGRAM_TARGET): $(LIBRARY_TARGET) $(PROGRAM_OBJECTS)
	@$(info $(call announce_section_detail_message,Building program,Creating $(PROGRAM_TARGET)))
	$(CC) -L$(TARGET_DIR) $(PROGRAM_OBJECTS) -l$(LIBRARY_NAME_BASE) $(LDFLAGS) $(LDLIBS) -o $(PROGRAM_TARGET)

$(PROGRAM_NAME): $(PROGRAM_TARGET)

$(TEST_PROGRAM_TARGET): $(LIBRARY_TARGET) $(TEST_OBJECTS)
	@$(info $(call announce_section_detail_message,Building test harness,Creating $(TEST_PROGRAM_TARGET)))
	$(CC) -L$(TARGET_DIR) $(TEST_OBJECTS) -l$(LIBRARY_NAME_BASE) $(TEST_LDFLAGS) $(TEST_LDLIBS) -o $(TEST_PROGRAM_TARGET)

$(TEST_PROGRAM): $(TEST_PROGRAM_TARGET)

clean:
	@$(info Deleting directory "$(TARGET_DIR)")
	@$(RM) -r $(TARGET_DIR)

validate:
ifeq ($(strip $(owner)),)
	$(error "Please set a value for 'owner' in project.mk")
else ifeq ($(strip $(package)),)
	$(error "Please set a value for 'package' in project.mk")
endif

announce-build:
	@$(info $(build_message))

announce-initialize-phase:
	@$(info $(call announce_phase_message,Initialize))

announce-create-build-directories:
	@$(info $(call announce_section_message,Creating build directories))

create-build-directories: announce-create-build-directories
	mkdir -p $(OBJECT_DIR)
	mkdir -p $(TEST_OBJECT_DIR)
	mkdir -p $(GENERATED_DEPEND_DIR)
	mkdir -p $(GENERATED_TEST_DEPEND_DIR)

initialize: validate announce-build announce-initialize-phase create-build-directories

announce-build-phase:
	@$(info $(call announce_phase_message,Build))

announce-ensure-dependencies:
ifneq ($(strip $(DEPENDENCY_VALIDATIONS)),)
	@$(info $(call announce_section_detail_message,Finding dependencies,Resolving $(words $(DEPENDENCY_VALIDATIONS)) dependencies))
endif

ensure-dependencies: initialize announce-build-phase announce-ensure-dependencies $(DEPENDENCY_VALIDATIONS)

announce-generate-sources:
ifneq ($(strip $(GENERATE_SOURCES_HOOKS)),)
	@$(info $(call announce_section_detail_message,Generating sources,Executing $(words $(GENERATE_SOURCES_HOOKS)) source hooks))
endif

announce-generate-source-dependencies:
	$(info $(call announce_section_detail_message,Generating source dependencies,Evaluating $(words $(DEPENDS)) source files))

generate-source-dependencies: announce-generate-source-dependencies $(DEPENDS)

generate-sources: ensure-dependencies announce-generate-sources $(GENERATE_SOURCES_HOOKS) generate-source-dependencies

process-sources: generate-sources $(PROCESS_SOURCES_HOOKS)

announce-generate-resources:
ifneq ($(strip $(GENERATE_RESOURCES_HOOKS)),)
	@$(info $(call announce_section_detail_message,Generating resources,Executing $(words $(GENERATE_RESOURCES_HOOKS)) resource hooks))
endif

generate-resources: process-sources announce-generate-resources $(GENERATE_RESOURCES_HOOKS)

announce-process-resources: count = $(shell if [ -d $(RESOURCES_DIR) ]; then ls $(RESOURCES_DIR) | wc -l; fi)
announce-process-resources:
ifeq ($(shell if [ -d $(RESOURCES_DIR) ]; then echo "true"; fi),true)
	@$(info $(call announce_section_detail_message,Copying resources,Evaluating $(strip $(count)) files))
endif

process-resources: generate-resources $(PROCESS_RESOURCES_HOOKS) announce-process-resources $(RESOURCES)

announce-compile-sources:
	@$(info $(call announce_section_detail_message,Compiling sources,Evaluating $(words $(OBJECTS)) source files))

compile: process-resources announce-compile-sources $(OBJECTS)

process-objects: compile

library: process-objects $(LIBRARY_TARGET)

target: library $(TARGET)

announce-test-phase:
	@$(info $(call announce_phase_message,Test))

announce-ensure-test-dependencies:
ifneq ($(strip $(DEPENDENCY_VALIDATIONS)),)
	@$(info $(call announce_section_detail_message,Finding test dependencies,Resolving $(words $(TEST_DEPENDENCY_VALIDATIONS)) dependencies))
endif

ensure-test-dependencies: target announce-test-phase announce-ensure-test-dependencies $(TEST_DEPENDENCY_VALIDATIONS)

announce-generate-test-sources:
ifneq ($(strip $(GENERATE_TEST_SOURCES_HOOKS)),)
	@$(info $(call announce_section_detail_message,Generating test sources,Executing $(words $(GENERATE_TEST_SOURCES_HOOKS)) test source hooks))
endif

announce-test-generate-source-dependencies:
	$(info $(call announce_section_detail_message,Generating test source dependencies,Evaluating $(words $(TEST_DEPENDS)) source files))

generate-test-source-dependencies: announce-test-generate-source-dependencies $(TEST_DEPENDS)

generate-test-sources: ensure-test-dependencies announce-generate-test-sources $(GENERATE_TEST_SOURCES_HOOKS) announce-test-generate-source-dependencies

process-test-sources: generate-test-sources $(PROCESS_TEST_SOURCES_HOOKS)

announce-generate-test-resources:
ifneq ($(strip $(GENERATE_TEST_RESOURCES_HOOKS)),)
	@$(info $(call announce_section_detail_message,Generating test resources,Executing $(words $(GENERATE_TEST_RESOURCES_HOOKS)) test resource hooks))
endif

generate-test-resources: process-test-sources announce-generate-test-sources $(GENERATE_TEST_RESOURCES_HOOKS)

announce-process-test-resources: count = $(shell if [ -d $(TEST_RESOURCES_DIR) ]; then ls $(TEST_RESOURCES_DIR) | wc -l; fi)
announce-process-test-resources:
ifeq ($(shell if [ -d $(TEST_RESOURCES_DIR) ]; then echo "true"; fi),true)
	@$(info $(call announce_section_detail_message,Copying test resources,Evaluating $(strip $(count)) files))
endif

process-test-resources: generate-test-resources $(PROCESS_TEST_RESOURCES_HOOKS) announce-process-test-resources $(TEST_RESOURCES)

announce-compile-test-sources:
	@$(info $(call announce_section_detail_message,Compiling test sources,Evaluating $(words $(TEST_OBJECTS)) source files))

test-compile: process-test-resources announce-compile-test-sources $(TEST_OBJECTS)

process-test-objects: test-compile

test-target: library process-test-objects $(TEST_PROGRAM_TARGET)

test: test-target
ifeq ($(strip $(skip_tests)),)
	@$(info $(call announce_section_detail_message,Executing test harness))
	@cd $(TARGET_DIR); ./$(TEST_PROGRAM)
else
	@$(info $(call announce_section_detail_message,Skipping tests))
	@:
endif

announce-package-phase:
	@$(info $(call announce_phase_message,Package))

prepare-package: test announce-package-phase

package: prepare-package

announce-install-phase:
	@$(info $(call announce_phase_message,Install))

verify: package announce-install-phase

install: verify
	$(INSTALL) -d -m 755 $(DESTDIR)$(bindir)
	$(INSTALL) $(TARGET) $(DESTDIR)$(bindir)

.PHONY: all check help check-syntax clean validate announce-initialize-phase announce-ensure-dependencies announce-create-build-directories create-buid-directories announce-build ensure-dependencies initialize announce-build-phase announce-generate-sources announce-generate-source-dependencies generate-source-dependencies generate-sources process-sources announce-generate-resources generate-resources process-resources announce-compile-sources compile process-objects library target ensure-test-dependencies announce-test-phase announce-generate-test-sources announce-generate-test-source-dependencies generate-test-source-dependencies generate-test-sources process-test-sources announce-generate-test-resources generate-test-resources process-test-resources announce-compile-test-sources test-compile process-test-objects test-target test announce-package-phase prepare-package package verify announce-install-phase install $(PROGRAM_NAME) $(LIBRARY_NAME) $(TEST_PROGRAM) $(GENERATE_SOURCES_HOOKS) $(PROCESS_SOURCES_HOOKS) $(GENERATE_RESOURCES_HOOKS) $(PROCESS_SOURCES_HOOKS) $(GENERATE_TEST_SOURCES_HOOKS) $(PROCESS_TEST_SOURCES_HOOKS) $(GENERATE_TEST_RESOURCES_HOOKS) $(PROCESS_TEST_SOURCES_HOOKS)
