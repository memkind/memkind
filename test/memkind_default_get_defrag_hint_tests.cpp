/*
 * Copyright (c) 2019 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memkind.h>
#include <memkind/internal/memkind_default.h>
#include <stdio.h>
#include "common.h"

#define PMEM_MAX_SIZE (1024 * 1024 * 64)
extern const char *PMEM_DIR;

class MemkindDefaultGetDefragHintTests : public ::testing::Test
{};

bool StartDefrag(int bin, int run)
{
	// condition from Redis
	if (run > bin || run == 1 << 16)
		return false;
	else
		return true;
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_MEMKIND_GetDefragHintReturnFalse)
{
	struct memkind *kind = nullptr;

	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);

	int bin = -1;
	int run = -1;
	int hint = -1;
	int alloc = 50000;
	void *ptr[alloc];

	for (int i = 0; i < alloc; i++) {
		ptr[i] = memkind_malloc(kind, 128);
		ASSERT_NE(ptr[i], nullptr);
	}

	hint = memkind_default_get_defrag_hint(ptr[alloc-1], &bin, &run);
	ASSERT_EQ(hint, 0);

	for (int i = 0; i < alloc; i++) {
		memkind_free(kind, ptr[i]);
	}

	ptr[0] = memkind_malloc(kind, 50 * KB);
	ASSERT_NE(ptr[0], nullptr);
	hint = memkind_default_get_defrag_hint(ptr[0], &bin, &run);
	ASSERT_EQ(hint, 0);
	memkind_free(kind, ptr[0]);

	err = memkind_destroy_kind(kind);
	ASSERT_EQ(err, 0);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_MEMKIND_GetDefragHintReturnTrue)
{
	struct memkind *kind = nullptr;

	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);

	int bin = -1;
	int run = -1;
	int hint = -1;
	int alloc = 50000;
	void *ptr[alloc];

	for (int i = 0; i < alloc; i++) {
		ptr[i] = memkind_malloc(kind, 128);
		ASSERT_NE(ptr[i], nullptr);
	}

	hint = memkind_default_get_defrag_hint(ptr[alloc / 2], &bin, &run);
	ASSERT_EQ(hint, 1);

	for (int i = 0; i < alloc; i++) {
		memkind_free(kind, ptr[i]);
	}

	err = memkind_destroy_kind(kind);
	ASSERT_EQ(err, 0);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_MEMKIND_GetDefragHintCheckBinUtilizationReturnFalse)
{
	struct memkind *kind = nullptr;

	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);
	
	int bin = -1;
	int run = -1;
	int hint = -1;
	int alloc = 50000;
	void *ptr[alloc];
	bool startDefrag;

	for (int i = 0; i < alloc; i++) {
		ptr[i] = memkind_malloc(kind, 1 * KB);
		ASSERT_NE(ptr[i], nullptr);
	}

	hint = memkind_default_get_defrag_hint(ptr[alloc / 2], &bin, &run);
	ASSERT_EQ(hint, 1);
	startDefrag = StartDefrag(bin, run);
	ASSERT_EQ(startDefrag, 0);

	for (int i = 0; i < alloc; i++) {
		memkind_free(kind, ptr[i]);
	}

	err = memkind_destroy_kind(kind);
	ASSERT_EQ(err, 0);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_MEMKIND_GetDefragHintCheckBinUtilizationReturnTrue)
{
	struct memkind *kind = nullptr;

	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);

	int bin = -1;
	int run = -1;
	int hint = -1;
	int alloc = 50000;
	void *ptr[alloc];
	bool startDefrag;

	for (int i = 0; i < alloc; i++) {
		ptr[i] = memkind_malloc(kind, 1 * KB);
		ASSERT_NE(ptr[i], nullptr);
	}

	for (int i = 1; i < alloc;) {
		memkind_free(kind, ptr[i]);
		ptr[i] = nullptr;
		// Free memory in irregular pattern
		if (i % 2 == 0)
			i += 3;
		else
			i += 5;
	}
	
	hint = memkind_default_get_defrag_hint(ptr[10], &bin, &run);
	ASSERT_EQ(hint, 1);
	startDefrag = StartDefrag(bin, run);
	ASSERT_EQ(startDefrag, 1);

	for (int i = 0; i < alloc; i++) {
		memkind_free(kind, ptr[i]);
	}

	err = memkind_destroy_kind(kind);
	ASSERT_EQ(err, 0);
}

