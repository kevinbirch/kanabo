# -*- mode: Makefile-gmake -*-

include project.mk

# xxx - precompile headers?

PROGRAM ?= $(shell basename `pwd`)
VERSION ?= 1.0.0-SNAPSHOT
DEPENDENCIES ?=

LIBRARY = $(PROGRAM).a

SOURCE_DIR  ?= src/main/c
INCLUDE_DIR ?= $(SOURCE_DIR)/include

TARGET_DIR  ?= target
OBJECT_DIR  ?= $(TARGET_DIR)/objects/$(PROGRAM)

# xxx - this must be configurable
CFLAGS := -std=c11 -Wall -pedantic -Wextra -Werror -O2 -I$(INCLUDE_DIR) $(CFLAGS)
LDLIBS := $(addprefix -l, $(DEPENDENCIES))

ifeq ($(BUILD_DEBUG),yes)
	CFLAGS := $(CFLAGS) -g
# xxx - add strip to default target if not debug
# else
# strip: $(PROGRAM)
# 	strip --strip-all $(PROGRAM)
endif

SOURCE_FILE_GLOBS = '*c' '*C' '*cpp'

# xxx - fix dep checking, currently always does a rebuild
SOURCES := $(shell find $(SOURCE_DIR) -type f \( -name '*.c' -or -name '*.cpp' -or -name '*.C' \))
OBJECTS := $(foreach s, $(SOURCES), $(basename $(notdir $(s))).o)

VPATH = $(shell find $(SOURCE_DIR) -type d | tr '\n' :)

# xxx - can this be done another way?
%.o: %.c
	$(CC) $(CFLAGS) $(CDEFS) -c $< -o $(OBJECT_DIR)/$@

default: $(PROGRAM)

$(LIBRARY): output $(OBJECTS)
	$(AR) rcs $(TARGET_DIR)/$(LIBRARY) $(wildcard $(OBJECT_DIR)/*.o)

$(PROGRAM): $(LIBRARY)
	$(CC) -o $(TARGET_DIR)/$(PROGRAM) $(LDLIBS) $(TARGET_DIR)/$(LIBRARY)

output:
	mkdir -p $(OBJECT_DIR)

clean:
	rm -rf $(OBJECT_DIR)

reallyclean:
	rm -rf $(TARGET_DIR)

# dist: clean
# 	sed -i "s/#define JSHONVER .*/#define JSHONVER ${VERSION}/" jshon.c
# 	mkdir jshon-${VERSION}
# 	cp jshon.c jshon.1 Makefile jshon-${VERSION}
# 	tar czf jshon-${VERSION}.tar.gz jshon-${VERSION}
# 	${RM} -r jshon-${VERSION}

.PHONY: all clean dist strip