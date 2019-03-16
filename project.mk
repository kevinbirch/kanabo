# -*- mode: Makefile-gmake -*-

owner = io.node6
package = kanabo
version = 0.5.0-SNAPSHOT
artifact = program
build = debug

DEPENDENCIES = yaml
TEST_DEPENDENCIES = check yaml

CC ?= cc
GCC_COV ?= gcov
LLVM_COV ?= llvm-profdata

system := $(shell uname -s)
is_clang := $(shell if [[ `$(CC) --version` =~ (clang|LLVM) ]]; then echo "true"; fi)

INCLUDES = -I$(SOURCES_DIR)/vendor/linenoise -I$(SOURCES_DIR)/vendor/spacecadet

CFLAGS = -std=c11 -Wall -Wextra -Werror -Wformat -Wformat-security -Wformat-y2k -Winit-self -Wmissing-include-dirs -Wswitch-default -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wconversion -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Wunreachable-code -Wno-switch-default -Wno-unknown-pragmas -Wno-unused-parameter -fstrict-aliasing -fms-extensions -fstack-protector
debug_CFLAGS = -DUSE_LOGGING -g -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1 -O1 -fno-omit-frame-pointer -fno-common -fsanitize=undefined -fsanitize=address
release_CFLAGS = -DUSE_LOGGING -O2 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fno-omit-frame-pointer -pie -fPIE

debug_LDFLAGS = -fstack-protector -fno-omit-frame-pointer -fno-common -fsanitize=undefined -fsanitize=address
release_LDFLAGS = -fstack-protector -fno-omit-frame-pointer -flto -pie -fPIE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

ifeq ($(is_clang),true)
CFLAGS += -Wno-gnu -Wno-microsoft
TEST_CFLAGS := $(CFLAGS) $($(build)_CFLAGS)
debug_CFLAGS += -fprofile-instr-generate -fcoverage-mapping
debug_LDFLAGS += -fprofile-instr-generate
TEST_ENV += LLVM_PROFILE_FILE=target/coverage/llvm/kanabo.profraw
else
# N.B. capture current values of CFLAGS so test code is not profiled for coverage
TEST_CFLAGS := $(CFLAGS) $($(build)_CFLAGS)
debug_CFLAGS += --coverage
debug_LDFLAGS += --coverage
endif

ifeq ($(system),Linux)
LDLIBS = -lm
TEST_LDLIBS = $(LDLIBS) -pthread -lrt -lsubunit
AR := ar rcs
TEST_ENV += CK_LOG_FILE_NAME=- CK_FORK=no LSAN_OPTIONS=report_objects=1 UBSAN_OPTIONS=print_stacktrace=true ASAN_OPTIONS=detect_leaks=true:check_initialization_order=true:symbolize_inline_frames=true:strict_string_checks=true
else ifeq ($(system),Darwin)
AR := libtool -static -o
ifeq ($(is_clang),true)
TEST_ENV += CK_LOG_FILE_NAME=- CK_FORK=no UBSAN_OPTIONS=print_stacktrace=true ASAN_OPTIONS=check_initialization_order=true:symbolize_inline_frames=true
endif
endif

VERSION_H = $(GENERATED_HEADERS_DIR)/version.h
CONFIG_H = $(GENERATED_HEADERS_DIR)/config.h

generate-version-header: $(GENERATED_HEADERS_DIR)
	@$(info Generating $(VERSION_H))
	@CC=$(CC) build/generate_version_header.sh $(version) $(VERSION_H)

generate-config-header: $(GENERATED_HEADERS_DIR)
	@$(info Generating $(CONFIG_H))
	@build/generate_config_header.sh $(CONFIG_H) PREFX=$(prefix) LIBEXECDIR=$(package_libexecdir) DATADIR=$(package_datadir) LOGDIR=$(package_logdir) RUNDIR=$(package_rundir) MANDIR=$(man1dir) HTMLDIR=$(htmldir) INFODIR=$(infodir)

GENERATE_SOURCES_HOOKS := generate-version-header generate-config-header

hash-package:
	sha512sum $(PACKAGE_TARGET) > $(TARGET_DIR)/$(PACKAGE_TARGET_BASE).sha512

PACKAGE_PHASE_HOOKS := hash-package

verify-package:
	@sha512sum -c $(TARGET_DIR)/$(PACKAGE_TARGET_BASE).sha512

VERIFY_PHASE_HOOKS := verify-package

gcc-coverage:
	$(GCC_COV) --branch-probabilities --branch-counts --preserve-paths --unconditional-branches $(LIBRARY_OBJECTS)
	mkdir -p target/coverage/gcov
	mv *.gcov target/coverage/gcov

llvm-coverage:
	$(LLVM_COV) merge -sparse target/coverage/llvm/kanabo.profraw -o target/coverage/llvm/kanabo.profdata

.PHONY: gcov
