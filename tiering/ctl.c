// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind_memtier.h>
#include <tiering/memtier_log.h>

#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CTL_VALUE_SEPARATOR        ":"
#define CTL_STRING_QUERY_SEPARATOR ","

typedef struct fs_dax_registry {
    unsigned size;
    memkind_t *kinds;
} fs_dax_registry;

static struct fs_dax_registry fs_dax_reg_g = {0, NULL};

static int ctl_add_pmem_kind_to_fs_dax_reg(memkind_t kind)
{
    memkind_t *new_kinds =
        memkind_realloc(MEMKIND_DEFAULT, fs_dax_reg_g.kinds,
                        sizeof(fs_dax_reg_g.kinds) * (fs_dax_reg_g.size + 1));
    if (!new_kinds)
        return -1;
    fs_dax_reg_g.kinds = new_kinds;
    fs_dax_reg_g.kinds[fs_dax_reg_g.size] = kind;
    fs_dax_reg_g.size++;
    return 0;
}

static void ctl_destroy_fs_dax_reg(void)
{
    int i;
    for (i = 0; i < fs_dax_reg_g.size; ++i) {
        memkind_destroy_kind(fs_dax_reg_g.kinds[i]);
    }
    memkind_free(MEMKIND_DEFAULT, fs_dax_reg_g.kinds);
}

/*
 * ctl_parse_u -- (internal) parses and returns an unsigned integer
 */
static int ctl_parse_u(const char *str, unsigned *dest)
{
    if (str[0] == '-') {
        return -1;
    }

    char *endptr;
    int olderrno = errno;
    errno = 0;
    unsigned long val_ul = strtoul(str, &endptr, 0);

    if (endptr == str || errno != 0 || val_ul > UINT_MAX) {
        errno = olderrno;
        return -1;
    }

    errno = olderrno;
    *dest = (unsigned)val_ul;
    return 0;
}

/*
 * ctl_parse_size_t -- (internal) parses string and returns a size_t integer
 */
static int ctl_parse_size_t(const char *str, size_t *dest)
{
    if (str[0] == '-') {
        return -1;
    }

    char *endptr;
    int olderrno = errno;
    errno = 0;
    unsigned long long val_ul = strtoull(str, &endptr, 0);

    if (endptr == str || errno != 0) {
        errno = olderrno;
        return -1;
    }

    errno = olderrno;
    *dest = (size_t)val_ul;
    return 0;
}

/*
 * ctl_parse_pmem_size -- parse size from string
 */
static int ctl_parse_pmem_size(char **sptr, size_t *sizep)
{
    struct suff {
        const char *suff;
        size_t mag;
    };
    const struct suff suffixes[] = {
        {"K", 1ULL << 10}, {"M", 1ULL << 20}, {"G", 1ULL << 30}};

    char size[32] = {0};
    char unit[3] = {0};

    const char *pmem_size_str = strtok_r(NULL, CTL_VALUE_SEPARATOR, sptr);
    if (pmem_size_str == NULL) {
        return -1;
    }

    if (pmem_size_str[0] == '-') {
        goto parse_failure;
    }

    int ret = sscanf(pmem_size_str, "%31[0-9]%2s", size, unit);
    if (ctl_parse_size_t(size, sizep)) {
        goto parse_failure;
    }
    if (ret == 1) {
        return 0;
    } else if (ret == 2) {
        unsigned i;
        for (i = 0; i < sizeof(suffixes) / sizeof((suffixes)[0]); ++i) {
            if (strcmp(suffixes[i].suff, unit) == 0) {
                if (SIZE_MAX / suffixes[i].mag >= *sizep) {
                    *sizep *= suffixes[i].mag;
                } else {
                    log_err("Provided pmem size is too big: %s", size);
                    goto parse_failure;
                }
                return 0;
            }
        }
    }

parse_failure:
    log_err("Failed to parse pmem size: %s", pmem_size_str);
    return -1;
}

/*
 * ctl_parse_policy -- (internal) returns memtier_policy that matches value in
 * query buffer
 */
static int ctl_parse_policy(char *qbuf, memtier_policy_t *policy)
{
    if (strcmp(qbuf, "POLICY_STATIC_THRESHOLD") == 0) {
        *policy = MEMTIER_POLICY_STATIC_THRESHOLD;
    } else {
        log_err("Unknown policy: %s", qbuf);
        return -1;
    }

    return 0;
}

static int ctl_parse_ratio(char **sptr, unsigned *dest)
{
    const char *ratio_str = strtok_r(NULL, CTL_VALUE_SEPARATOR, sptr);
    if (ratio_str == NULL) {
        log_err("Ratio not provided");
        return -1;
    }

    int ret = ctl_parse_u(ratio_str, dest);
    if (ret != 0 || *dest == 0) {
        log_err("Unsupported ratio: %s", ratio_str);
        return -1;
    }

    return 0;
}

/*
 * ctl_parse_query -- (internal) splits an entire query string
 * into single queries
 */
static int ctl_parse_query(char *qbuf, memkind_t *kind, unsigned *ratio)
{
    char *sptr = NULL;
    char *pmem_path = NULL;
    size_t pmem_size = 1;
    int ret = -1;

    char *kind_name = strtok_r(qbuf, CTL_VALUE_SEPARATOR, &sptr);
    if (kind_name == NULL) {
        log_err("Kind name string not found in: %s", qbuf);
        return -1;
    }

    if (!strcmp(kind_name, "DRAM")) {
        *kind = MEMKIND_DEFAULT;
    } else if (!strcmp(kind_name, "FS_DAX")) {
        pmem_path = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
        if (pmem_path == NULL) {
            return -1;
        }

        ret = ctl_parse_pmem_size(&sptr, &pmem_size);
        if (ret != 0) {
            return -1;
        }

        ret = memkind_create_pmem(pmem_path, pmem_size, kind);
        if (ret || ctl_add_pmem_kind_to_fs_dax_reg(*kind)) {
            return -1;
        }
    } else {
        log_err("Unsupported kind: %s", kind_name);
        return -1;
    }

    ret = ctl_parse_ratio(&sptr, ratio);
    if (ret != 0) {
        return -1;
    }

    /* the value itself mustn't include CTL_VALUE_SEPARATOR */
    char *extra = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
    if (extra != NULL) {
        return -1;
    }

    return 0;
}

struct memtier_memory *ctl_create_tier_memory_from_env(char *env_var_string)
{
    struct memtier_memory *tier_memory;
    memtier_policy_t policy = MEMTIER_POLICY_MAX_VALUE;
    unsigned i;

    int ret;
    char *sptr = NULL;
    char *qbuf = env_var_string;

    unsigned query_count = 1;
    while (*qbuf)
        if (*qbuf++ == *CTL_STRING_QUERY_SEPARATOR)
            ++query_count;

    unsigned tier_count = query_count - 1;

    qbuf = strtok_r(env_var_string, CTL_STRING_QUERY_SEPARATOR, &sptr);
    if (qbuf == NULL) {
        log_err("No valid query found in: %s", env_var_string);
        return NULL;
    }

    if (query_count < 2) {
        log_err("Too low number of queries in configuration string: %s",
                env_var_string);
        return NULL;
    }

    struct memtier_builder *builder = memtier_builder_new();
    if (!builder) {
        return NULL;
    }

    for (i = 0; i < tier_count; ++i) {
        memkind_t kind = NULL;
        unsigned ratio = 0;

        ret = ctl_parse_query(qbuf, &kind, &ratio);
        if (ret != 0) {
            log_err("Failed to parse query: %s", qbuf);
            goto cleanup_after_failure;
        }

        qbuf = strtok_r(NULL, CTL_STRING_QUERY_SEPARATOR, &sptr);

        ret = memtier_builder_add_tier(builder, kind, ratio);
        if (ret != 0) {
            goto cleanup_after_failure;
        }
    }

    ret = ctl_parse_policy(qbuf, &policy);
    if (ret != 0) {
        log_err("Failed to parse policy: %s", qbuf);
        goto cleanup_after_failure;
    }

    ret = memtier_builder_set_policy(builder, policy);
    if (ret != 0) {
        goto cleanup_after_failure;
    }

    ret = memtier_builder_construct_memtier_memory(builder, &tier_memory);
    if (ret != 0) {
        goto cleanup_after_failure;
    }

    memtier_builder_delete(builder);
    return tier_memory;

cleanup_after_failure:
    memtier_builder_delete(builder);
    ctl_destroy_fs_dax_reg();

    return NULL;
}

void ctl_destroy_tier_memory(struct memtier_memory *memory)
{
    ctl_destroy_fs_dax_reg();
    memtier_delete_memtier_memory(memory);
}
