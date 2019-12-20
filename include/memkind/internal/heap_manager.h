// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2019 Intel Corporation. */

#pragma once

#include <memkind.h>

void heap_manager_init(struct memkind *kind);
void heap_manager_free(void *ptr);
size_t heap_manager_malloc_usable_size(void *ptr);
void *heap_manager_realloc(void *ptr, size_t size);
struct memkind *heap_manager_detect_kind(void *ptr);
int heap_manager_update_cached_stats(void);
int heap_manager_get_stat(memkind_stat_type stat, size_t *value);
void *heap_manager_defrag_reallocate(void *ptr);
