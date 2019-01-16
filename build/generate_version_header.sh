#!/usr/bin/env bash

if [ 2 -ne $# ]; then
    echo "usage: $(basename $0) <version> <filename>"
    exit 1
fi

VERSION=$1
FILENAME=$2

mkdir -p $(dirname $FILENAME)

case $VERSION in
    *-SNAPSHOT) is_snapshot=true;;
    *) is_snapshot=false;;
esac

base_version=${VERSION%-SNAPSHOT}

IFS=$'.'
read -r major_version minor_version point_version <<< "${base_version}"
unset IFS

cat >$FILENAME <<EOF
#pragma once

#include <stdbool.h>

#define COPYRIGHT "Copyright (c) $(date "+%Y") Kevin Birch <kmb@pobox.com>.  All rights reserved."
static const char * const COPYRIGHT_c = "Copyright (c) $(date "+%Y") Kevin Birch <kmb@pobox.com>.  All rights reserved.";
#define VERSION "${VERSION}"
static const char * const VERSION_c = "${VERSION}";
#define MAJOR_VERSION ${major_version}
static const unsigned int MAJOR_VERSION_c = ${major_version};
#define MINOR_VERSION ${minor_version}
static const unsigned int MINOR_VERSION_c = ${minor_version};
#define POINT_VERSION ${point_version}
static const unsigned int POINT_VERSION_c = ${point_version};
#define IS_SNAPSHOT ${is_snapshot}
static const bool IS_SNAPSHOT_c = ${is_snapshot};

#define BUILD_COMPILER "$(${CC} --version | head -1)"
static const char * const BUILD_COMPILER_c = "$(${CC} --version | head -1)";
#define BUILD_DATE "$(date)"
static const char * const BUILD_DATE_c = "$(date)";
#define BUILD_TIMESTAMP $(date "+%s")
static const unsigned long BUILD_TIMESTAMP_c = $(date "+%s");
#define BUILD_HOSTNAME "$(hostname -s)"
static const char * const BUILD_HOSTNAME_c = "$(hostname -s)";
#define BUILD_HOST_ARCHITECHTURE "$(uname -m)"
static const char * const BUILD_HOST_ARCHITECHTURE_c = "$(uname -m)";
#define BUILD_HOST_OS "$(uname -s)"
static const char * const BUILD_HOST_OS_c = "$(uname -s)";
#define BUILD_HOST_OS_VERSION "$(uname -r)"
static const char * const BUILD_HOST_OS_VERSION_c = "$(uname -r)";

EOF
