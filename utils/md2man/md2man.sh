#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2022, Intel Corporation

#
# md2man.sh -- converts markdown file into groff man page using pandoc.
#	It performs some pre- and post-processing for cleaner man page.
#
# usage: md2man.sh md_input_file man_template outfile
#

set -e
set -o pipefail

# read input params (usually passed from Makefile.am)
filename=${1}
template=${2}
outfile=${3}

# parse input file for YAML metadata block and read man page titles, section and copyright date
title=$(sed -n 's/^title:\ \([A-Za-z_-]*\)$/\1/p' "${filename}")
section=$(sed -n 's/^section:\ \([0-9]\)$/\1/p' "${filename}")
copyright=$(grep Copyright "${filename}" -m 1 | cut -d ' ' -f 4 |cut -d ',' -f1)


# set proper date
dt="$(date --utc --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%F)"

# output dir may not exist
mkdir -p '$(dirname "$outfile")'

# check if the pandoc is present in the system
if ! [ -x "$(command -v pandoc)" ]; then
  echo 'Error: pandoc is not installed.' >&2
  exit 1
fi

# 1. cut-off metadata block and license (everything up to "NAME" section)
# 2. prepare man page based on template, using given parameters
# 3. unindent code blocks
cat "$filename" | sed -n -e '/# NAME #/,$p' |\
	pandoc -s -t man -o "$outfile" --template="$template" \
	-V title="$title" -V section="$section" \
	-V date="$dt" -V copyright="$copyrigh"t |
sed '/^\.IP/{
N
/\n\.nf/{
	s/IP/PP/
    }
}'
