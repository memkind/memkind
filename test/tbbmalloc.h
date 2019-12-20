// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2016 - 2018 Intel Corporation. */

#include <stdio.h>
#include <dlfcn.h>

void *(*scalable_malloc)(size_t);
void *(*scalable_realloc)(void *, size_t);
void *(*scalable_calloc)(size_t, size_t);
void  (*scalable_free)(void *);

int load_tbbmalloc_symbols();
