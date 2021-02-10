// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include "include/memkind/internal/memkind_pmem.h"
#include "include/memkind/internal/memkind_private.h"
#include "memkind.h"

#include "common.h"

#include <sys/statfs.h>

extern const char *PMEM_DIR;

class MemkindConfigTests: public ::testing::Test
{

protected:
    struct memkind_config *global_test_cfg;
    void SetUp()
    {
        global_test_cfg = memkind_config_new();
        ASSERT_NE(nullptr, global_test_cfg);
    }

    void TearDown()
    {
        memkind_config_delete(global_test_cfg);
    }
};

TEST_F(MemkindConfigTests, test_TC_MEMKIND_CreateAndDestroyEmptyConfig)
{
    struct memkind_config *tmp_cfg = memkind_config_new();
    ASSERT_NE(nullptr, tmp_cfg);
    memkind_config_delete(tmp_cfg);
}

TEST_F(MemkindConfigTests, test_TC_MEMKIND_PmemConSetConfigSizeZero)
{
    memkind_config_set_size(global_test_cfg, 0U);
    ASSERT_EQ(global_test_cfg->pmem_size, 0U);
}

TEST_F(MemkindConfigTests, test_TC_MEMKIND_PmemSetConfigSizeNotZero)
{
    memkind_config_set_size(global_test_cfg, 100 * MB);
    ASSERT_EQ(global_test_cfg->pmem_size, 100 * MB);
}

TEST_F(MemkindConfigTests, test_TC_MEMKIND_PmemSetConfigPath)
{
    memkind_config_set_path(global_test_cfg, PMEM_DIR);

    ASSERT_STREQ(global_test_cfg->pmem_dir, PMEM_DIR);
}

TEST_F(MemkindConfigTests, test_TC_MEMKIND_PmemSetDefaultMemoryUsagePolicy)
{
    memkind_config_set_memory_usage_policy(global_test_cfg,
                                           MEMKIND_MEM_USAGE_POLICY_DEFAULT);

    ASSERT_EQ(global_test_cfg->policy, MEMKIND_MEM_USAGE_POLICY_DEFAULT);
}

TEST_F(MemkindConfigTests, test_TC_MEMKIND_SetConservativeMemoryUsagePolicy)
{
    memkind_config_set_memory_usage_policy(
        global_test_cfg, MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE);

    ASSERT_EQ(global_test_cfg->policy, MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE);
}

TEST_F(MemkindConfigTests,
       test_TC_MEMKIND_PmemCreatePmemWithParamsFailWrongDirectory)
{
    memkind_t pmem_kind = nullptr;

    memkind_config_set_path(global_test_cfg, "/temp/non_exisitng_directory");
    memkind_config_set_size(global_test_cfg, 0U);
    memkind_config_set_memory_usage_policy(global_test_cfg,
                                           MEMKIND_MEM_USAGE_POLICY_DEFAULT);

    int err = memkind_create_pmem_with_config(global_test_cfg, &pmem_kind);
    ASSERT_EQ(err, MEMKIND_ERROR_INVALID);
}

TEST_F(MemkindConfigTests,
       test_TC_MEMKIND_PmemCreatePmemWithParamsFailWrongSize)
{
    memkind_t pmem_kind = nullptr;

    memkind_config_set_path(global_test_cfg, PMEM_DIR);
    memkind_config_set_size(global_test_cfg, MEMKIND_PMEM_MIN_SIZE - 1);
    memkind_config_set_memory_usage_policy(global_test_cfg,
                                           MEMKIND_MEM_USAGE_POLICY_DEFAULT);

    int err = memkind_create_pmem_with_config(global_test_cfg, &pmem_kind);
    ASSERT_EQ(err, MEMKIND_ERROR_INVALID);
}

TEST_F(MemkindConfigTests,
       test_TC_MEMKIND_PmemCreatePmemWithParamsFailWrongPolicy)
{
    memkind_t pmem_kind = nullptr;

    memkind_config_set_path(global_test_cfg, PMEM_DIR);
    memkind_config_set_size(global_test_cfg, MEMKIND_PMEM_MIN_SIZE - 1);
    memkind_config_set_memory_usage_policy(global_test_cfg,
                                           MEMKIND_MEM_USAGE_POLICY_MAX_VALUE);

    int err = memkind_create_pmem_with_config(global_test_cfg, &pmem_kind);
    ASSERT_EQ(err, MEMKIND_ERROR_INVALID);
}

TEST_F(MemkindConfigTests,
       test_TC_MEMKIND_PmemCreatePmemWithParamsSuccessCheckConservativePolicy)
{
    memkind_t pmem_kind = nullptr;
    int err = 0;
    double blocksBeforeFree = 0;
    double blocksAfterFree = 0;
    struct memkind_pmem *priv = nullptr;
    void *temp_ptr = nullptr;
    struct stat st;
    const char *str = getenv("MEMKIND_HOG_MEMORY");
    int skip_test = str && str[0] == '1';
    if (skip_test) {
        GTEST_SKIP();
    }
    memkind_config_set_path(global_test_cfg, PMEM_DIR);
    memkind_config_set_size(global_test_cfg, 0U);
    memkind_config_set_memory_usage_policy(
        global_test_cfg, MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE);
    err = memkind_create_pmem_with_config(global_test_cfg, &pmem_kind);
    ASSERT_EQ(err, 0);

    priv = static_cast<memkind_pmem *>(pmem_kind->priv);

    temp_ptr = memkind_malloc(pmem_kind, 1 * KB);

    err = fstat(priv->fd, &st);
    ASSERT_EQ(err, 0);
    blocksBeforeFree = st.st_blocks;

    memkind_free(pmem_kind, temp_ptr);

    err = fstat(priv->fd, &st);
    ASSERT_EQ(err, 0);
    blocksAfterFree = st.st_blocks;

    // if blocksAfterFree is less than blocksBeforeFree, extent was called.
    ASSERT_LT(blocksAfterFree, blocksBeforeFree);

    err = memkind_destroy_kind(pmem_kind);
    ASSERT_EQ(err, 0);
}
