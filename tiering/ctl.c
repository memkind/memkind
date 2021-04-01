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

#define MAX_TIERS 64
static struct memtier_tier *tiers[MAX_TIERS] = {NULL};

typedef struct ctl_tier_cfg {
    char *kind_name;
    char *pmem_path;
    size_t pmem_size;
    unsigned ratio_value;
} ctl_tier_cfg;

// TODO: Remove this variable and struct in a code cleanup
static ctl_tier_cfg *tier_cfgs;

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

static int ctl_validate_kind_name(const char *kind_name)
{
    if (strcmp(kind_name, "DRAM") && strcmp(kind_name, "FS_DAX")) {
        log_err("Unsupported kind: %s", kind_name);
        return -1;
    }

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
static int ctl_parse_query(char *qbuf, unsigned tier_num)
{
    char *sptr = NULL;
    ctl_tier_cfg *tier_cfg = &tier_cfgs[tier_num];

    tier_cfg->kind_name = strtok_r(qbuf, CTL_VALUE_SEPARATOR, &sptr);
    if (tier_cfg->kind_name == NULL) {
        log_err("Kind name string not found in: %s", qbuf);
        return -1;
    }
    int ret = ctl_validate_kind_name(tier_cfg->kind_name);
    if (ret != 0) {
        return -1;
    }

    if (!strcmp(tier_cfg->kind_name, "FS_DAX")) {
        tier_cfg->pmem_path = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
        if (tier_cfg->pmem_path == NULL) {
            return -1;
        }

        ret = ctl_parse_pmem_size(&sptr, &tier_cfg->pmem_size);
        if (ret != 0) {
            return -1;
        }
    }

    ret = ctl_parse_ratio(&sptr, &tier_cfg->ratio_value);
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

static memkind_t ctl_get_kind(unsigned tier_num)
{
    memkind_t kind = NULL;
    ctl_tier_cfg tier_cfg = tier_cfgs[tier_num];

    if (strcmp(tier_cfg.kind_name, "DRAM") == 0) {
        kind = MEMKIND_DEFAULT;
        log_debug("kind_name: memkind_default");
    } else if (strcmp(tier_cfg.kind_name, "FS_DAX") == 0) {
        int res =
            memkind_create_pmem(tier_cfg.pmem_path, tier_cfg.pmem_size, &kind);
        if (res || ctl_add_pmem_kind_to_fs_dax_reg(kind)) {
            return NULL;
        }
        log_debug("kind_name: FS-DAX");
        log_debug("pmem_path: %s", tier_cfg.pmem_path);
        log_debug("pmem_size: %zu", tier_cfg.pmem_size);
    }

    return kind;
}

static const char *ctl_policy_to_str(memtier_policy_t policy)
{
    if (policy < MEMTIER_POLICY_STATIC_THRESHOLD ||
        policy >= MEMTIER_POLICY_MAX_VALUE) {
        log_err("Unknown policy: %d", policy);
        return NULL;
    }

    const char *policies[] = {"POLICY_STATIC_THRESHOLD"};

    return policies[policy];
}

static void ctl_destroy_tiers(void)
{
    unsigned i;
    for (i = 0; i < MAX_TIERS; ++i) {
        if (tiers[i])
            memtier_tier_delete(tiers[i]);
    }
    memkind_free(MEMKIND_DEFAULT, tier_cfgs);
}

struct memtier_kind *ctl_create_tier_kind_from_env(char *env_var_string)
{
    struct memtier_kind *tier_kind;
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

    if (tier_count > MAX_TIERS) {
        log_err("Too much memory tiers %u", tier_count);
        return NULL;
    }

    tier_cfgs =
        memkind_calloc(MEMKIND_DEFAULT, tier_count, sizeof(ctl_tier_cfg));
    if (!tier_cfgs) {
        log_err("Error during allocation of memory.");
        return NULL;
    }

    for (i = 0; i < tier_count; ++i) {
        ret = ctl_parse_query(qbuf, i);

        if (ret != 0) {
            log_err("Failed to parse query: %s", qbuf);
            memkind_free(MEMKIND_DEFAULT, tier_cfgs);
            return NULL;
        }

        qbuf = strtok_r(NULL, CTL_STRING_QUERY_SEPARATOR, &sptr);
    }

    ret = ctl_parse_policy(qbuf, &policy);
    if (ret != 0) {
        log_err("Failed to parse policy: %s", qbuf);
        memkind_free(MEMKIND_DEFAULT, tier_cfgs);
        return NULL;
    }

    struct memtier_builder *builder = memtier_builder_new();
    if (!builder) {
        goto tiers_delete;
    }

    for (i = 0; i < tier_count; ++i) {

        memkind_t kind = ctl_get_kind(i);
        if (kind == NULL) {
            goto builder_delete;
        }

        log_debug("ratio_value: %u", tier_cfgs[i].ratio_value);
        log_debug("policy: %s", ctl_policy_to_str(policy));

        tiers[i] = memtier_tier_new(kind);
        if (tiers[i] == NULL) {
            goto builder_delete;
        }

        ret = memtier_builder_add_tier(builder, tiers[i],
                                       tier_cfgs[i].ratio_value);
        if (ret != 0) {
            goto builder_delete;
        }
    }

    ret = memtier_builder_set_policy(builder, policy);
    if (ret != 0) {
        goto builder_delete;
    }

    ret = memtier_builder_construct_kind(builder, &tier_kind);
    if (ret != 0) {
        goto builder_delete;
    }

    memtier_builder_delete(builder);
    return tier_kind;

builder_delete:
    memtier_builder_delete(builder);

tiers_delete:
    ctl_destroy_tiers();
    ctl_destroy_fs_dax_reg();

    return NULL;
}

void ctl_destroy_kind(struct memtier_kind *kind)
{
    ctl_destroy_fs_dax_reg();
    ctl_destroy_tiers();
    memtier_delete_kind(kind);
}
