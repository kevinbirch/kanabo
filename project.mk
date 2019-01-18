# -*- mode: Makefile-gmake -*-

owner = io.node6
package = kanabo
version = 0.4.0-SNAPSHOT
artifact = program
build = debug

DEPENDENCIES = yaml
TEST_DEPENDENCIES = check yaml

CFLAGS += -std=c11 -fstrict-aliasing -Wall -Wextra -Werror -Wformat -Wformat-security -Wformat-y2k -Winit-self -Wmissing-include-dirs -Wswitch-default -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wconversion -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Wunreachable-code -Wno-switch-default -Wno-unknown-pragmas -Wno-gnu -fms-extensions -Wno-microsoft -Wno-unused-parameter
debug_CFLAGS = -DUSE_LOGGING -g -fno-omit-frame-pointer -fsanitize=address,undefined
release_CFLAGS = -DUSE_LOGGING -O3
debug_LDFLAGS = -fno-omit-frame-pointer -fsanitize=address,undefined
release_LDFLAGS = -flto

system = $(shell uname -s)

ifeq ($(system),Linux)
LDLIBS := -lm
TEST_LDLIBS := $(LDLIBS) -pthread -lrt -lsubunit
TEST_ENV := CK_FORK=no ASAN_OPTIONS=detect_leaks=1
AR = ar rcs
else ifeq ($(system),Darwin)
AR := libtool -static -o
endif

VERSION_H = $(GENERATED_HEADERS_DIR)/version.h
CONFIG_H = $(GENERATED_HEADERS_DIR)/config.h

$(VERSION_H): $(GENERATED_HEADERS_DIR)
	@echo "Generating $(VERSION_H)"
	@CC=$(CC) build/generate_version_header.sh $(version) $(VERSION_H)

$(CONFIG_H): $(GENERATED_HEADERS_DIR)
	@echo "Generating $(CONFIG_H)"
	@build/generate_config_header.sh $(CONFIG_H) PREFX=$(prefix) LIBEXECDIR=$(package_libexecdir) DATADIR=$(package_datadir) LOGDIR=$(package_logdir) RUNDIR=$(package_rundir) MANDIR=$(man1dir) HTMLDIR=$(htmldir) INFODIR=$(infodir)

generate-version-header: $(VERSION_H)
generate-config-header: $(CONFIG_H)
GENERATE_SOURCES_HOOKS = generate-version-header generate-config-header
