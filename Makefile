# -*- mode: Makefile-gmake -*-

export SHELL = /usr/bin/env bash

# Compatibility target
all: help

PROJECT_CONFIG_FILE ?= project.mk

include $(PROJECT_CONFIG_FILE)

## Defaults for common shell commands
CC ?= cc
AR ?= ar rcs
RM ?= rm -f
DIFF ?= diff
TAR ?= tar
FIND ?= find
INSTALL ?= install -D -t
MKTEMP ?= mktemp
PACKAGE ?= $(TAR) cf

## Defaults for project settings
version ?= 1.0.0-SNAPSHOT
artifact ?= program
build ?= debug
DEPENDENCIES ?=
TEST_DEPENDENCIES ?=

## Defaults for lifecycle hooks
INITIALIZE_PHASE_HOOKS ?=
DEPENDENCY_CHECK_OVERRIDE ?=
GENERATE_SOURCES_HOOKS ?=
PROCESS_SOURCES_HOOKS ?=
GENERATE_RESOURCES_HOOKS ?=
PROCESS_RESOURCES_HOOKS ?=
GENERATE_TEST_SOURCES_HOOKS ?=
PROCESS_TEST_SOURCES_HOOKS ?=
GENERATE_TEST_RESOURCES_HOOKS ?=
PROCESS_TEST_RESOURCES_HOOKS ?=
BUILD_PHASE_HOOKS ?=
TEST_DEPENDENCY_CHECK_OVERRIDE ?= $(DEPENDENCY_CHECK_OVERRIDE)
TEST_PHASE_HOOKS ?=
PACKAGE_PHASE_OVERRIDE ?=
PACKAGE_PHASE_HOOKS ?=
VERIFY_PHASE_HOOKS ?=

## Defaults for project source directories
SOURCE_DIR ?= src
MAIN_DIR ?= $(SOURCE_DIR)/main
SOURCES_DIR ?= $(MAIN_DIR)/c
INCLUDE_DIR ?= $(SOURCES_DIR)/include
RESOURCES_DIR ?= $(MAIN_DIR)/resources

## Defaults for project test source directories
TEST_DIR ?= $(SOURCE_DIR)/test
TEST_SOURCES_DIR ?= $(TEST_DIR)/c
TEST_INCLUDE_DIR ?= $(TEST_SOURCES_DIR)/include
TEST_RESOURCES_DIR ?= $(TEST_DIR)/resources

## Defaults for project output directories
TARGET_DIR ?= target
OBJECT_DIR ?= $(TARGET_DIR)/objects
TEST_OBJECT_DIR ?= $(TARGET_DIR)/test-objects
GENERATED_SOURCES_DIR ?= $(TARGET_DIR)/generated-sources
GENERATED_HEADERS_DIR ?= $(GENERATED_SOURCES_DIR)/include
GENERATED_DEPEND_DIR ?= $(GENERATED_SOURCES_DIR)/depend
GENERATED_TEST_SOURCES_DIR ?= $(TARGET_DIR)/generated-test-sources
GENERATED_TEST_HEADERS_DIR ?= $(GENERATED_TEST_SOURCES_DIR)/include
GENERATED_TEST_DEPEND_DIR ?= $(TARGET_DIR)/generated-test-sources/depend
RESOURCES_TARGET_DIR ?= $(TARGET_DIR)/resources
TEST_RESOURCES_TARGET_DIR ?= $(TARGET_DIR)/test-resources

## Set this variable to any value to skip executing the test harness
skip_tests ?=

## Project target artifact settings
ARTIFACT_BASE_NAME = $(package)
PROGRAM_NAME ?= $(ARTIFACT_BASE_NAME)
PROGRAM_TARGET = $(TARGET_DIR)/$(PROGRAM_NAME)
LIBRARY_BASE_NAME ?= $(ARTIFACT_BASE_NAME)
LIBRARY_NAME ?= lib$(LIBRARY_BASE_NAME).a
LIBRARY_TARGET = $(TARGET_DIR)/$(LIBRARY_NAME)
TEST_PROGRAM = $(package)_test
TEST_PROGRAM_TARGET = $(TARGET_DIR)/$(TEST_PROGRAM)
PACKAGE_TARGET_BASE ?= $(ARTIFACT_BASE_NAME)_$(version)
PACKAGE_TARGET_DIR ?= $(TARGET_DIR)/$(PACKAGE_TARGET_BASE)
PACKAGE_TARGET ?= $(TARGET_DIR)/$(PACKAGE_TARGET_BASE).tar.gz

COMPILATION_LOG := $(TARGET_DIR)/compiler_log.txt

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

## Dependency handling
DEPENDENCY_RESOLVERS := $(addprefix dependency/,$(DEPENDENCIES))
DEPENDENCY_INCLUDES ?=
DEPENDENCY_LDFLAGS ?=
TEST_DEPENDENCY_RESOLVERS := $(addprefix test-dependency/,$(TEST_DEPENDENCIES))
TEST_DEPENDENCY_INCLUDES ?= $(DEPENDENCY_INCLUDES)
TEST_DEPENDENCY_LDFLAGS ?= $(DEPENDENCY_LDFLAGS)

## Project compiler settings
INCLUDES := $(INCLUDES) -I$(GENERATED_HEADERS_DIR) -I$(INCLUDE_DIR) $($(build)_INCLUDES)
CFLAGS := $(CFLAGS) $($(build)_CFLAGS)
LDFLAGS := $(LDFLAGS) $($(build)_LDFLAGS)
LDLIBS := $(LDLIBS) $($(build)_LDLIBS)

## Project test compiler settings
TEST_INCLUDES ?= $(INCLUDES)
TEST_INCLUDES := $(TEST_INCLUDES) -I$(TEST_INCLUDE_DIR)
TEST_CFLAGS ?= $(CFLAGS)
TEST_CFLAGS := $(TEST_CFLAGS)
TEST_LDFLAGS ?= $(LDFLAGS)
TEST_LDFLAGS := $(TEST_LDFLAGS)
TEST_LDLIBS ?= $(LDLIBS)
TEST_LDLIBS := $(TEST_LDLIBS)
TEST_ENV ?=

## Automation helper functions
source_to_target = $(foreach s, $(1), $(2)/$(basename $(s)).$(3))
source_to_object = $(call source_to_target,$(1),$(2),o)
source_to_depend = $(call source_to_target,$(1),$(2),d)
find_files = $(shell if [ -d $(1) ]; then cd $(1); $(FIND) . -type f | sed 's|\./||'; fi)
find_source_files = $(shell if [ -d $(1) ]; then cd $(1); $(FIND) . -type f \( -name '*.c' -or -name '*.C' \) | sed 's|\./||'; fi)
find_resources = $(foreach r, $(wildcard $(1)/*), $(subst $(1),$(2),$(r)))

## Project source file locations
SOURCES = $(call find_source_files,$(SOURCES_DIR))
ALL_SOURCES = $(SOURCES) $(call find_source_files,$(GENERATED_SOURCES_DIR))
OBJECTS = $(call source_to_object,$(ALL_SOURCES),$(OBJECT_DIR))
PROGRAM_SOURCES ?= $(shell grep -l -E '(int|void)\s+main' $(addprefix $(SOURCES_DIR)/,$(SOURCES)) | sed 's|$(SOURCES_DIR)/||')
PROGRAM_OBJECTS = $(call source_to_object,$(PROGRAM_SOURCES),$(OBJECT_DIR))
LIBRARY_OBJECTS = $(filter-out $(PROGRAM_OBJECTS),$(OBJECTS))
vpath %.c $(SOURCES_DIR) $(GENERATED_SOURCES_DIR)
vpath %.h $(INCLUDE_DIR) $(GENERATED_HEADERS_DIR)
DEPENDS = $(call source_to_depend,$(ALL_SOURCES),$(GENERATED_DEPEND_DIR))
RESOURCES := $(call find_resources,$(RESOURCES_DIR),$(RESOURCES_TARGET_DIR))

ifeq ($(artifact),program)
ifneq (1,$(words $(PROGRAM_SOURCES)))
$(warning "Multiple sources containing `main' detected: $(PROGRAM_SOURCES)")
endif
endif

## Project test source file locations
TEST_SOURCES := $(call find_source_files,$(TEST_SOURCES_DIR)) $(call find_source_files,$(GENERATED_TEST_SOURCES_DIR))
TEST_OBJECTS := $(call source_to_object,$(TEST_SOURCES),$(TEST_OBJECT_DIR))
vpath %.c $(TEST_SOURCES_DIR) $(GENERATED_TEST_SOURCES_DIR)
vpath %.h $(TEST_INCLUDE_DIR) $(GENERATED_TEST_HEADERS_DIR)
TEST_DEPENDS := $(call source_to_depend,$(TEST_SOURCES),$(GENERATED_TEST_DEPEND_DIR))
TEST_RESOURCES := $(call find_resources,$(TEST_RESOURCES_DIR),$(TEST_RESOURCES_TARGET_DIR))

vpath %.a $(TARGET_DIR)

## Configure the build target type
ifeq ($(artifact),program)
ARTIFACT_NAME = $(PROGRAM_NAME)
TARGET = $(PROGRAM_TARGET)
PACKAGE_ARTIFACT_TARGET_DIR = $(PACKAGE_TARGET_DIR)/bin
else ifeq ($(artifact),library)
ARTIFACT_NAME = $(LIBRARY_NAME)
TARGET = $(LIBRARY_TARGET)
PACKAGE_ARTIFACT_TARGET_DIR = $(PACKAGE_TARGET_DIR)/lib
else
$(error "Unsupported value of 'artifact': $(artifact)")
endif

PACKAGE_ARTIFACT_TARGET = $(PACKAGE_ARTIFACT_TARGET_DIR)/$(ARTIFACT_NAME)

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

env      - print the build environment settings and finish
clean    - remove all build artifacts
compile  - build object files
target   - build the target library or program
test     - build and run the test harness
package  - collect the target artifacts info a distributable bundle
verify   - validate the distributable bundle
endef

define build_message =

 Buidling $(owner):$(package):$(version)
endef

define announce_phase_message =

------------------------------------------------------------------------
 $(1) Phase
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

env:
	@echo "--- environment settings for build ---"; \
	echo "* project:"; \
	echo "    identifier: $(owner):$(package):$(version)"; \
	echo "    artifact: $(artifact)"; \
	echo "    build type: $(build)"; \
	echo "    dependencies: $(DEPENDENCIES)"; \
	echo "    test dependencies: $(TEST_DEPENDENCIES)"; \
	echo "    command: $(CC) $(CFLAGS) $(INCLUDES) -c <in> -o <out>"; \
	echo "* hooks:"; \
	echo "    initialize phase: $(INITIALIZE_PHASE_HOOKS)"; \
	echo "    generate sources: $(GENERATE_SOURCES_HOOKS)"; \
	echo "    process sources: $(PROCESS_SOURCES_HOOKS)"; \
	echo "    generate resources: $(GENERATE_RESOURCES_HOOKS)"; \
	echo "    process resources: $(PROCESS_RESOURCES_HOOKS)"; \
	echo "    test dependency check override: $(TEST_DEPENDENCY_CHECK_OVERRIDE)"; \
	echo "    generate test sources: $(GENERATE_TEST_SOURCES_HOOKS)"; \
	echo "    process test sources: $(PROCESS_TEST_SOURCES_HOOKS)"; \
	echo "    generate test resources: $(GENERATE_TEST_RESOURCES_HOOKS)"; \
	echo "    process test resources: $(PROCESS_TEST_RESOURCES_HOOKS)"; \
	echo "    build phase: $(BUILD_PHASE_HOOKS)"; \
	echo "    test phase: $(TEST_PHASE_HOOKS)"; \
	echo "    package phase: $(PACKAGE_PHASE_HOOKS)"; \
	echo "    verify phase: $(VERIFY_PHASE_HOOKS)"; \
	echo "* overrides:"; \
	echo "    dependency check: $(DEPENDENCY_CHECK_OVERRIDE)"; \
	echo "    package phase: $(PACKAGE_PHASE_OVERRIDE)"; \
	echo "* directories:"; \
	echo "    sources: $(SOURCES_DIR)"; \
	echo "    include: $(INCLUDE_DIR)"; \
	echo "    resources: $(RESOURCES_DIR)"; \
	echo "    test sources: $(TEST_SOURCES_DIR)"; \
	echo "    test include: $(TEST_INCLUDE_DIR)"; \
	echo "    test resources: $(TEST_RESOURCES_DIR)"; \
	echo "    target: $(TARGET_DIR)"; \
	echo "    object: $(OBJECT_DIR)"; \
	echo "    test object: $(TEST_OBJECT_DIR)"; \
	echo "    generated sources: $(GENERATED_SOURCES_DIR)"; \
	echo "    generated headers: $(GENERATED_HEADERS_DIR)"; \
	echo "    generated depend: $(GENERATED_DEPEND_DIR)"; \
	echo "    generated test sources: $(GENERATED_TEST_SOURCES_DIR)"; \
	echo "    generated test headers: $(GENERATED_TEST_HEADERS_DIR)"; \
	echo "    generated test depend: $(GENERATED_TEST_DEPEND_DIR)"; \
	echo "* artifacts:"; \
	echo "    program: $(PROGRAM_NAME)"; \
	echo "    program target: $(PROGRAM_TARGET)"; \
	echo "    library: $(LIBRARY_NAME)"; \
	echo "    library target: $(LIBRARY_TARGET)"; \
	echo "    test program: $(TEST_PROGRAM)"; \
	echo "    test program target: $(TEST_PROGRAM_TARGET)"; \


## Suport Emacs flymake syntax checker
check-syntax: create-build-directories $(GENERATE_SOURCES_HOOKS) $(GENERATE_TEST_SOURCES_HOOKS)
	$(CC) $(TEST_CFLAGS) -fsyntax-only $(TEST_INCLUDES) $(INCLUDES) $(CHK_SOURCES)

define define_dependency_variables =
 $(dependency_prefix)DEPENDENCY_$$(@F)_INCLUDES ?= $$($$(dependency_prefix)DEPENDENCY_INCLUDES)
 $(dependency_prefix)DEPENDENCY_$$(@F)_LDFLAGS ?= $$($$(dependency_prefix)DEPENDENCY_LDFLAGS)
 $(dependency_prefix)DEPENDENCY_$$(@F)_HEADER ?= $(@F).h
 $(dependency_prefix)DEPENDENCY_$$(@F)_LIB ?= $(@F)
 $(dependency_prefix)LDLIBS := -l$$($(dependency_prefix)DEPENDENCY_$(@F)_LIB) $($(dependency_prefix)LDLIBS)
 dependency_$$(@F)_infile := $$(shell $(MKTEMP) -t dependency_$(@F)_XXXXXX.c)
 dependency_$$(@F)_outfile := $$(shell $(MKTEMP) -t dependency_$(@F)_XXXXXX.o)
endef

define dependency_test_template =
#include <$($(dependency_prefix)DEPENDENCY_$(@F)_HEADER)>
int main(void) {return 0;}
endef

ifeq ($(strip $(DEPENDENCY_CHECK_OVERRIDE)),)
define dependency_test_canned_recipe =
@$(info resolving: $(@F))
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
@$(info invoking depencency check overide: $(DEPENDENCY_CHECK_OVERRIDE))
@$(DEPENDENCY_CHECK_OVERRIDE) $(@F)
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
	@echo "`date +%Y-%m-%dT%H:%M:%S%:z` $< $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@" >> $(COMPILATION_LOG)

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
	$(AR) $(LIBRARY_TARGET) $(shell $(FIND) $(OBJECT_DIR) -type f -name '*.o')

$(LIBRARY_NAME): $(LIBRARY_TARGET)

$(PROGRAM_TARGET): $(LIBRARY_TARGET) $(PROGRAM_OBJECTS)
	@$(info $(call announce_section_detail_message,Building program,Creating $(PROGRAM_TARGET)))
	$(CC) -L$(TARGET_DIR) $(PROGRAM_OBJECTS) -l$(LIBRARY_BASE_NAME) $(LDFLAGS) $(LDLIBS) -o $(PROGRAM_TARGET)

$(PROGRAM_NAME): $(PROGRAM_TARGET)

$(TEST_PROGRAM_TARGET): $(LIBRARY_TARGET) $(TEST_OBJECTS)
	@$(info $(call announce_section_detail_message,Building test harness,Creating $(TEST_PROGRAM_TARGET)))
	$(CC) -L$(TARGET_DIR) $(TEST_OBJECTS) -l$(LIBRARY_BASE_NAME) $(TEST_LDFLAGS) $(TEST_LDLIBS) -o $(TEST_PROGRAM_TARGET)

$(TEST_PROGRAM): $(TEST_PROGRAM_TARGET)

clean:
	@$(info Deleting directory "$(TARGET_DIR)")
	@$(RM) -r $(TARGET_DIR)

validate:
ifeq ($(strip $(owner)),)
	$(error Please set a value for 'owner' in $(PROJECT_CONFIG_FILE))
else ifeq ($(strip $(package)),)
	$(error Please set a value for 'package' in $(PROJECT_CONFIG_FILE))
endif

announce-build:
	@$(info $(build_message))

announce-initialize-phase:
	@$(info $(call announce_phase_message,Initialize))

announce-create-build-directories:
	@$(info $(call announce_section_message,Creating build directories))

create-build-directories: announce-create-build-directories
	mkdir -p $(OBJECT_DIR)
	mkdir -p $(GENERATED_SOURCES_DIR)
	mkdir -p $(GENERATED_HEADERS_DIR)
	mkdir -p $(GENERATED_DEPEND_DIR)
	mkdir -p $(RESOURCES_TARGET_DIR)
	mkdir -p $(TEST_OBJECT_DIR)
	mkdir -p $(GENERATED_TEST_SOURCES_DIR)
	mkdir -p $(GENERATED_TEST_HEADERS_DIR)
	mkdir -p $(GENERATED_TEST_DEPEND_DIR)
	mkdir -p $(TEST_RESOURCES_TARGET_DIR)

announce-initialize-phase-hooks:
	@$(info $(call announce_section_detail_message,Hooks,Invoking $(words $(INITIALIZE_PHASE_HOOKS)) hooks))

ifeq ($(strip $(INITIALIZE_PHASE_HOOKS)),)
invoke-initialize-phase-hooks:
	@:
else
invoke-initialize-phase-hooks: announce-initialize-phase-hooks $(INITIALIZE_PHASE_HOOKS)
endif

initialize-internal: validate announce-build announce-initialize-phase create-build-directories

initialize: initialize-internal invoke-initialize-phase-hooks

announce-build-phase:
	@$(info $(call announce_phase_message,Build))

announce-resolve-dependencies:
	@$(info $(call announce_section_detail_message,Resolving system dependencies,Resolving $(words $(DEPENDENCY_RESOLVERS)) system dependencies))

ifeq ($(strip $(DEPENDENCY_RESOLVERS)),)
resolve-dependencies:
	@:
else
resolve-dependencies: announce-resolve-dependencies $(DEPENDENCY_RESOLVERS)
endif

ensure-dependencies: initialize announce-build-phase resolve-dependencies

announce-generate-sources-hooks:
	@$(info $(call announce_section_detail_message,Generating sources,Invoking $(words $(GENERATE_SOURCES_HOOKS)) hooks))

ifeq ($(strip $(GENERATE_SOURCES_HOOKS)),)
invoke-generate-source-hooks:
	@:
else
invoke-generate-source-hooks: announce-generate-sources-hooks $(GENERATE_SOURCES_HOOKS)
endif

announce-generate-source-dependencies:
	$(info $(call announce_section_message,Generating source dependencies))

generate-source-dependencies: announce-generate-source-dependencies $(DEPENDS)

generate-sources: ensure-dependencies invoke-generate-source-hooks generate-source-dependencies

announce-process-sources-hooks:
	@$(info $(call announce_section_detail_message,Processing sources,Invoking $(words $(PROCESS_SOURCES_HOOKS)) hooks))

ifeq ($(strip $(PROCESS_SOURCES_HOOKS)),)
invoke-process-sources-hooks:
	@:
else
invoke-process-sources-hooks: announce-process-sources-hooks $(PROCESS_SOURCES_HOOKS)
endif

process-sources: generate-sources invoke-process-sources-hooks

announce-generate-resources-hooks:
	@$(info $(call announce_section_detail_message,Generating resources,Invoking $(words $(GENERATE_RESOURCES_HOOKS)) hooks))

ifeq ($(strip $(GENERATE_RESOURCES_HOOKS)),)
invoke-generate-resources-hooks:
	@:
else
invoke-generate-resources-hooks: announce-generate-resources-hooks $(GENERATE_RESOURCES_HOOKS)
endif

generate-resources: process-sources invoke-generate-resources-hooks

announce-process-resources-hooks:
	@$(info $(call announce_section_detail_message,Processing resources,Invoking $(words $(PROCESS_RESOURCES_HOOKS)) hooks))

ifeq ($(strip $(PROCESS_RESOURCES_HOOKS)),)
invoke-process-resources-hooks:
	@:
else
invoke-process-resources-hooks: announce-process-resources-hooks $(PROCESS_RESOURCES_HOOKS)
endif

announce-process-resources:
ifeq ($(shell if [ -n "`find $(RESOURCES_DIR) -type f -maxdepth 1 2>/dev/null`" ]; then echo "true"; fi),true)
	@$(info $(call announce_section_detail_message,Copying resources,Evaluating $(strip $(shell ls $(RESOURCES_DIR) | wc -l)) files))
endif

process-resources: generate-resources invoke-process-resources-hooks announce-process-resources $(RESOURCES)

announce-compile-sources:
	@$(info $(call announce_section_detail_message,Compiling sources,Evaluating $(words $(OBJECTS)) files))

announce-build-phase-hooks:
	@$(info $(call announce_section_detail_message,Hooks,Invoking $(words $(BUILD_PHASE_HOOKS)) hooks))

ifeq ($(strip $(BUILD_PHASE_HOOKS)),)
invoke-build-phase-hooks:
	@:
else
invoke-build-phase-hooks: announce-build-phase-hooks $(BUILD_PHASE_HOOKS)
endif

compile-internal: process-resources announce-compile-sources $(OBJECTS)

compile: compile-internal invoke-build-phase-hooks

process-objects: compile

library: process-objects $(LIBRARY_TARGET)

target: library $(TARGET)

announce-test-phase:
	@$(info $(call announce_phase_message,Test))

announce-resolve-test-dependencies:
	@$(info $(call announce_section_detail_message,Resolving system test dependencies,Resolving $(words $(TEST_DEPENDENCY_RESOLVERS)) system dependencies))

ifeq ($(strip $(TEST_DEPENDENCY_RESOLVERS)),)
resolve-test-dependencies:
	@:
else
resolve-test-dependencies: announce-resolve-test-dependencies $(TEST_DEPENDENCY_RESOLVERS)
endif

ensure-test-dependencies: target announce-test-phase resolve-test-dependencies

announce-generate-test-sources-hooks:
	@$(info $(call announce_section_detail_message,Generating test sources,Invoking $(words $(GENERATE_TEST_SOURCES_HOOKS)) hooks))

ifeq ($(strip $(GENERATE_TEST_SOURCES_HOOKS)),)
invoke-generate-test-source-hooks:
	@:
else
invoke-generate-test-source-hooks: announce-generate-test-sources-hooks $(GENERATE_TEST_SOURCES_HOOKS)
endif

announce-generate-test-source-dependencies:
	$(info $(call announce_section_message,Generating test source dependencies))

generate-test-source-dependencies: announce-generate-test-source-dependencies $(TEST_DEPENDS)

generate-test-sources: ensure-test-dependencies invoke-generate-test-source-hooks generate-test-source-dependencies

announce-process-test-sources-hooks:
	@$(info $(call announce_section_detail_message,Processing test sources,Invoking $(words $(PROCESS_TEST_SOURCES_HOOKS)) hooks))

ifeq ($(strip $(PROCESS_TEST_SOURCES_HOOKS)),)
invoke-process-test-sources-hooks:
	@:
else
invoke-process-test-sources-hooks: announce-process-test-sources-hooks $(PROCESS_TEST_SOURCES_HOOKS)
endif

process-test-sources: generate-test-sources invoke-process-test-sources-hooks

announce-generate-test-resources-hooks:
	@$(info $(call announce_section_detail_message,Generating test resources,Invoking $(words $(GENERATE_TEST_RESOURCES_HOOKS)) hooks))

ifeq ($(strip $(GENERATE_TEST_RESOURCES_HOOKS)),)
invoke-generate-test-resources-hooks:
	@:
else
invoke-generate-test-resources-hooks: announce-generate-test-resources-hooks $(GENERATE_TEST_RESOURCES_HOOKS)
endif

generate-test-resources: process-test-sources invoke-generate-test-resources-hooks

announce-process-test-resources-hooks:
	@$(info $(call announce_section_detail_message,Processing test resources,Invoking $(words $(PROCESS_TEST_RESOURCES_HOOKS)) hooks))

ifeq ($(strip $(PROCESS_TEST_RESOURCES_HOOKS)),)
invoke-process-test-resources-hooks:
	@:
else
invoke-process-test-resources-hooks: announce-process-test-resources-hooks $(PROCESS_TEST_RESOURCES_HOOKS)
endif

announce-process-test-resources:
ifeq ($(shell if [ -n "`find $(TEST_RESOURCES_DIR) -type f -maxdepth 1 2>/dev/null`" ]; then echo "true"; fi),true)
	@$(info $(call announce_section_detail_message,Copying test resources,Evaluating $(strip $(shell ls $(TEST_RESOURCES_DIR) | wc -l)) files))
endif

process-test-resources: generate-test-resources invoke-process-test-resources-hooks announce-process-test-resources $(TEST_RESOURCES)

announce-compile-test-sources:
	@$(info $(call announce_section_detail_message,Compiling test sources,Evaluating $(words $(TEST_OBJECTS)) source files))

test-compile: process-test-resources announce-compile-test-sources $(TEST_OBJECTS)

process-test-objects: test-compile

test-target: library process-test-objects $(TEST_PROGRAM_TARGET)

announce-test-phase-hooks:
	@$(info $(call announce_section_detail_message,Hooks,Invoking $(words $(TEST_PHASE_HOOKS)) hooks))

ifeq ($(strip $(TEST_PHASE_HOOKS)),)
invoke-test-phase-hooks:
	@:
else
invoke-test-phase-hooks: announce-test-phase-hooks $(TEST_PHASE_HOOKS)
endif

ifeq ($(strip $(skip_tests)),)
test-internal: test-target
	@$(info $(call announce_section_detail_message,Executing test harness,$(TEST_ENV) ./$(TARGET_DIR)/$(TEST_PROGRAM)))
	@cd $(TARGET_DIR); $(TEST_ENV) ./$(TEST_PROGRAM)

test: test-internal invoke-test-phase-hooks
else
test: test-target
	@$(info $(call announce_section_message,Skipping tests))
	@:
endif

announce-package-phase:
	@$(info $(call announce_phase_message,Package))

prepare-package: announce-package-phase

$(PACKAGE_TARGET_DIR):
	@mkdir -p $@

announce-package-assemble-resources:
ifeq ($(shell if [ -n "`find $(RESOURCES_TARGET_DIR) -type f -maxdepth 1 2>/dev/null`" ]; then echo "true"; fi),true)
	@$(info $(call announce_section_detail_message,Assembling resources,Evaluating $(strip $(shell ls $(RESOURCES_TARGET_DIR) | wc -l)) files))
endif

$(RESOURCES_TARGET_DIR):
	@mkdir -p $@

$(PACKAGE_TARGET_DIR)/%: $(RESOURCES_TARGET_DIR)/%
	$(INSTALL) $(PACKAGE_TARGET_DIR)$(subst $(RESOURCES_TARGET_DIR),,$(<D)) $<

package-assemble-resources: announce-package-assemble-resources $(RESOURCES_TARGET_DIR) $(addprefix $(PACKAGE_TARGET_DIR)/,$(call find_files,$(RESOURCES_TARGET_DIR)))

announce-package-assemble-artifact:
	@$(info $(call announce_section_detail_message,Assemble artifact,$(TARGET)))

package-assemble-artifact: announce-package-assemble-artifact
	$(INSTALL) $(PACKAGE_ARTIFACT_TARGET_DIR) $(TARGET)

announce-package-build-package:
	@$(info $(call announce_section_detail_message,Building package,$(PACKAGE_TARGET)))

announce-package-phase-hooks:
	@$(info $(call announce_section_detail_message,Hooks,Invoking $(words $(PACKAGE_PHASE_HOOKS)) hooks))

ifeq ($(strip $(PACKAGE_PHASE_HOOKS)),)
invoke-package-phase-hooks:
	@:
else
invoke-package-phase-hooks: announce-package-phase-hooks $(PACKAGE_PHASE_HOOKS)
endif

ifeq ($(strip $(PACKAGE_PHASE_OVERRIDE)),)
package-internal: test prepare-package $(PACKAGE_TARGET_DIR) package-assemble-resources package-assemble-artifact announce-package-build-package
	$(PACKAGE) $(PACKAGE_TARGET) $(PACKAGE_TARGET_DIR)

package: package-internal invoke-package-phase-hooks
else
package: test announce-package-phase
	@$(info invoking package phase override: $(PACKAGE_PHASE_OVERRIDE))
	@$(PACKAGE_PHASE_OVERRIDE) $(PACKAGE_TARGET)
endif

announce-verify-phase:
	@$(info $(call announce_phase_message,Verify))

prepare-verify: announce-verify-phase

announce-verify-phase-hooks:
	@$(info $(call announce_section_detail_message,Hooks,Invoking $(words $(VERIFY_PHASE_HOOKS)) hooks))

ifeq ($(strip $(VERIFY_PHASE_HOOKS)),)
invoke-verify-phase-hooks:
	@:
else
invoke-verify-phase-hooks: announce-verify-phase-hooks $(VERIFY_PHASE_HOOKS)
endif

verify-internal: package prepare-verify
ifeq ($(strip $(VERIFY_PHASE_HOOKS)),)
	@$(info Verification completed)
endif

verify: verify-internal invoke-verify-phase-hooks

.PHONY: all check help check-syntax clean validate announce-initialize-phase announce-ensure-dependencies announce-create-build-directories create-buid-directories announce-build resolve-dependencies announce-resolve-dependencies resolve-dependencies ensure-dependencies announce-initialize-phase-hooks invoke-initialize-phase-hooks initialize-internal initialize announce-build-phase announce-generate-sources announce-generate-source-dependencies generate-source-dependencies invoke-generate-source-hooks generate-sources process-sources announce-generate-resources generate-resources process-resources announce-compile-sources compile-internal compile process-objects library target ensure-test-dependencies announce-test-phase announce-generate-test-sources announce-generate-test-source-dependencies generate-test-source-dependencies generate-test-sources process-test-sources announce-generate-test-resources generate-test-resources process-test-resources announce-compile-test-sources test-compile process-test-objects test-target test-internal test announce-package-phase prepare-package process-package-resources announce-package-assemble-resources package-assemble-resources announce-package-assemble-artifact package-assemble-artifact announce-package-build-package package-internal package announce-package-hooks package-hooks $(PACKAGE_PHASE_HOOKS) announce-verify-phase prepare-verify announce-verify-hooks verify-hooks $(VERIFY_PHASE_HOOKS) verify-internal verify $(PROGRAM_NAME) $(LIBRARY_NAME) $(TEST_PROGRAM) $(GENERATE_SOURCES_HOOKS) $(PROCESS_SOURCES_HOOKS) $(GENERATE_RESOURCES_HOOKS) $(PROCESS_RESOURCES_HOOKS) $(GENERATE_TEST_SOURCES_HOOKS) $(PROCESS_TEST_SOURCES_HOOKS) $(GENERATE_TEST_RESOURCES_HOOKS) $(PROCESS_TEST_RESOURCES_HOOKS) announce-initialize-hooks initialize-hooks $(INITIALIZE_PHASE_HOOKS) announce-build-hooks build-hooks $(BUILD_PHASE_HOOKS) announce-test-hooks test-hooks $(TEST_PHASE_HOOKS) announce-generate-sources-hooks invoke-generate-source-hooks announce-process-sources-hooks invoke-process-sources-hooks announce-generate-resources-hooks invoke-generate-resources-hooks announce-process-resources-hooks invoke-process-resources-hooks announce-build-phase-hooks invoke-build-phase-hooks announce-resolve-test-dependencies resolve-test-dependencies announce-generate-test-sources-hooks invoke-generate-test-source-hooks announce-generate-test-source-dependencies generate-test-source-dependencies generate-test-sources announce-process-test-sources-hooks invoke-process-test-sources-hooks process-test-sources announce-generate-test-resources-hooks invoke-generate-test-resources-hooks announce-process-test-resources-hooks invoke-process-test-resources-hooks announce-test-phase-hooks invoke-test-phase-hooks announce-package-phase-hooks invoke-package-phase-hooks announce-verify-phase-hooks invoke-verify-phase-hooks
