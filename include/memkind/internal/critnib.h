/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2018, Intel Corporation */

#ifndef CRITNIB_H
#define CRITNIB_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct critnib;
typedef struct critnib critnib;

enum find_dir_t {
	FIND_L  = -2,
	FIND_LE = -1,
	FIND_EQ =  0,
	FIND_GE = +1,
	FIND_G  = +2,
};

critnib *critnib_new(void);
void critnib_delete(critnib *c);

int critnib_insert(critnib *c, uintptr_t key, void *value, int update);
void *critnib_remove(critnib *c, uintptr_t key);
void *critnib_get(critnib *c, uintptr_t key);
void *critnib_find_le(critnib *c, uintptr_t key);
int critnib_find(critnib *c, uintptr_t key, enum find_dir_t dir,
	uintptr_t *rkey, void **rvalue);
void critnib_iter(critnib *c, uintptr_t min, uintptr_t max,
       int (*func)(uintptr_t key, void *value, void *privdata), void *privdata);

#ifdef __cplusplus
}
#endif

#endif
