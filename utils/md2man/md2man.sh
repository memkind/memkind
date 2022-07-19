#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2021, Intel Corporation

#
# md2man.sh -- converts markdown file into groff man page using pandoc.
#	It performs some pre- and post-processing for cleaner man page.
#
# usage: md2man.sh md_input_file man_template outfile API_version
#

set -e
set -o pipefail

# read input params (usually passed from doc/CMakeLists.txt)
filename=${1}
template=${2}
outfile=${3}
version=${4}

# parse input file for YAML metadata block and read man page titles and section
title=$(sed -n 's/^title:\ \([A-Za-z_-]*\)$/\1/p' ${filename})
section=$(sed -n 's/^section:\ \([0-9]\)$/\1/p' ${filename})



# set proper date/year
dt="$(date --utc --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%F)"
year="$(date --utc --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%Y)"

# output dir may not exist
mkdir -p $(dirname $outfile)

# 1. cut-off metadata block and license (everything up to "NAME" section)
# 2. prepare man page based on template, using given parameters
# 3. unindent code blocks
cat $filename | sed -n -e '/# NAME #/,$p' |\
	pandoc -s -t man -o $outfile --template=$template \
	-V title=$title -V section=$section \
	-V date="$dt" -V version="$version" \
	-V year="$year" -V secondary_title="$secondary_title" |
sed '/^\.IP/{
N
/\n\.nf/{
	s/IP/PP/
    }
}'
