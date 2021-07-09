// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind/internal/memkind_memtier.h>
#include <tiering/memtier_log.h>

#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CTL_VALUE_SEPARATOR        ":"
#define CTL_PARAM_SEPARATOR        ","
#define CTL_STRING_QUERY_SEPARATOR ";"

#define MAX_ENV_STRING 1024
#define MAX_KIND       255
#define MAX_CTL_NAME   64

#define CTL_THRES_VAL 0U
#define CTL_THRES_MIN 1U
#define CTL_THRES_MAX 2U

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
 * ctl_parse_size -- parse size (with optional suffix) from string
 */
static int ctl_parse_size(char **sptr, size_t *sizep)
{
    struct suff {
        const char *suff;
        size_t mag;
    };
    const struct suff suffixes[] = {
        {"K", 1ULL << 10}, {"M", 1ULL << 20}, {"G", 1ULL << 30}};

    char size[32] = {0};
    char unit[3] = {0};

    const char *size_str = strtok_r(NULL, CTL_VALUE_SEPARATOR, sptr);
    if (size_str == NULL) {
        return -1;
    }

    if (size_str[0] == '-') {
        goto parse_failure;
    }

    int ret = sscanf(size_str, "%31[0-9]%2s", size, unit);
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
                    log_err("Provided size is too big: %s", size);
                    goto parse_failure;
                }
                return 0;
            }
        }
    }

parse_failure:
    log_err("Failed to parse size: %s", size_str);
    return -1;
}

/*
 * ctl_parse_policy -- (internal) returns memtier_policy that matches value in
 * query buffer
 */
static int ctl_parse_policy(char *qbuf, memtier_policy_t *policy)
{
    if (strcmp(qbuf, "STATIC_RATIO") == 0) {
        *policy = MEMTIER_POLICY_STATIC_RATIO;
    } else if (strcmp(qbuf, "DYNAMIC_THRESHOLD") == 0) {
        *policy = MEMTIER_POLICY_DYNAMIC_THRESHOLD;
    } else if (strcmp(qbuf, "DATA_HOTNESS") == 0) {
        *policy = MEMTIER_POLICY_DATA_HOTNESS;
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
 * ctl_parse_tier_query -- parses query string with tier configuration
 */
static int ctl_parse_tier_query(char *qbuf, memkind_t *kind, unsigned *ratio)
{
    char *sptr = NULL;
    int ret = -1;

    int is_fsdax = 0;
    char *fsdax_path = NULL;
    char *fsdax_size_str = NULL;
    char *ratio_str = NULL;

    int kind_set = 0;
    int fsdax_path_set = 0;
    int fsdax_size_set = 0;
    int ratio_set = 0;

    char *param_str = strtok_r(qbuf, CTL_VALUE_SEPARATOR, &sptr);
    while (param_str != NULL) {
        if (!strcmp(param_str, "KIND")) {
            if (kind_set) {
                log_err("KIND param already defined");
                return -1;
            }
            char *kind_name = strtok_r(NULL, CTL_PARAM_SEPARATOR, &sptr);
            if (!strcmp(kind_name, "DRAM")) {
                *kind = MEMKIND_DEFAULT;
            } else if (!strcmp(kind_name, "KMEM_DAX")) {
                *kind = MEMKIND_DAX_KMEM;
                void *ptr = memkind_malloc(*kind, 32);
                if (!ptr) {
                    log_err("Allocation to DAX KMEM nodes failed!");
                    return -1;
                }
                memkind_free(*kind, ptr);
            } else if (!strcmp(kind_name, "FS_DAX")) {
                // for FS_DAX we have to collect all parameteres before
                // initialization
                is_fsdax = 1;
            } else {
                log_err("Unsupported kind: %s", kind_name);
                return -1;
            }
            kind_set = 1;
        } else if (!strcmp(param_str, "PATH")) {
            if (fsdax_path_set) {
                log_err("PATH param already defined");
                return -1;
            }
            fsdax_path = strtok_r(NULL, CTL_PARAM_SEPARATOR, &sptr);
            fsdax_path_set = 1;
        } else if (!strcmp(param_str, "PMEM_SIZE_LIMIT")) {
            if (fsdax_size_set) {
                log_err("PMEM_SIZE_LIMIT param already defined");
                return -1;
            }
            fsdax_size_str = strtok_r(NULL, CTL_PARAM_SEPARATOR, &sptr);
            fsdax_size_set = 1;
        } else if (!strcmp(param_str, "RATIO")) {
            if (ratio_set) {
                log_err("RATIO param already defined");
                return -1;
            }
            ratio_str = strtok_r(NULL, CTL_PARAM_SEPARATOR, &sptr);
            ret = ctl_parse_ratio(&ratio_str, ratio);
            if (ret != 0) {
                return -1;
            }
            ratio_set = 1;
        } else {
            log_err("Invalid parameter: %s", param_str);
            return -1;
        }

        param_str = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
    }

    if (!kind) {
        log_err("KIND param not found");
        return -1;
    } else if (!ratio_str) {
        log_err("RATIO param not found");
        return -1;
    }

    if (is_fsdax) {
        if (!fsdax_path_set) {
            log_err("PATH param (required for FS_DAX) not found");
            return -1;
        }

        ret = memkind_check_dax_path(fsdax_path);
        if (ret)
            log_info("%s don't point to DAX device", fsdax_path);

        size_t fsdax_max_size = 0;
        if (fsdax_size_set) {
            ret = ctl_parse_size(&fsdax_size_str, &fsdax_max_size);
            if (ret != 0) {
                return -1;
            }
        }

        ret = memkind_create_pmem(fsdax_path, fsdax_max_size, kind);
        if (ret || ctl_add_pmem_kind_to_fs_dax_reg(*kind)) {
            return -1;
        }
    } else {
        if (fsdax_path_set) {
            log_err("PATH param can be defined only for FS_DAX kind");
            return -1;
        }

        if (fsdax_size_set) {
            log_err("PMEM_SIZE_LIMIT param can be defined only "
                    "for FS_DAX kind");
            return -1;
        }
    }

    /* the value itself mustn't include CTL_VALUE_SEPARATOR */
    char *extra = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
    if (extra != NULL) {
        return -1;
    }

    return 0;
}

static int ctl_set_thr_cfg(char **sptr, unsigned ctl_id, int th_id,
                           unsigned *ctl_opts, struct memtier_builder *builder)
{
    char buf[MAX_CTL_NAME] = {0};

    const char *ctl_cmd[][2] = {
        {"policy.dynamic_threshold.thresholds[%d].val", "VAL"},
        {"policy.dynamic_threshold.thresholds[%d].min", "MIN"},
        {"policy.dynamic_threshold.thresholds[%d].max", "MAX"}};

    size_t val;
    const unsigned NUM_CMD = sizeof(ctl_cmd) / sizeof(ctl_cmd[0]);
    if (ctl_id >= NUM_CMD) {
        return -1;
    }

    if (*ctl_opts & (1U << ctl_id)) {
        log_err("%s defined twice", ctl_cmd[ctl_id][1]);
        return -1;
    }

    char *val_str = strtok_r(NULL, CTL_PARAM_SEPARATOR, sptr);

    int ret = ctl_parse_size(&val_str, &val);
    if (ret != 0) {
        log_err("Failed to parse value: %s", val_str);
        return -1;
    }

    snprintf(buf, MAX_CTL_NAME, ctl_cmd[ctl_id][0], th_id);
    *ctl_opts |= (1U << ctl_id);
    return memtier_ctl_set(builder, buf, &val);
}

/*
 * ctl_parse_threshold_query -- parses query string with threshold configuration
 */
static int ctl_parse_threshold_query(char *qbuf, int threshold_id,
                                     struct memtier_builder *builder)
{
    char *sptr = NULL;
    int ret;
    unsigned ctl_opts = 0;

    char *param_str = strtok_r(qbuf, CTL_VALUE_SEPARATOR, &sptr);
    if (param_str == NULL) {
        log_err("No valid parameters defined");
        return -1;
    }

    while (param_str != NULL) {
        if (!strcmp(param_str, "INIT_VAL")) {
            ret = ctl_set_thr_cfg(&sptr, CTL_THRES_VAL, threshold_id, &ctl_opts,
                                  builder);
            if (ret != 0)
                return -1;
        } else if (!strcmp(param_str, "MIN_VAL")) {
            ret = ctl_set_thr_cfg(&sptr, CTL_THRES_MIN, threshold_id, &ctl_opts,
                                  builder);
            if (ret != 0)
                return -1;
        } else if (!strcmp(param_str, "MAX_VAL")) {
            ret = ctl_set_thr_cfg(&sptr, CTL_THRES_MAX, threshold_id, &ctl_opts,
                                  builder);
            if (ret != 0)
                return -1;
        } else {
            log_err("Invalid parameter: %s", param_str);
            return -1;
        }

        param_str = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
    }

    return 0;
}

struct memtier_memory *ctl_create_tier_memory_from_env(char *env_var_string)
{
    char env_var_local[MAX_ENV_STRING] = {0};
    strncpy(env_var_local, env_var_string, MAX_ENV_STRING - 1);

    struct memtier_memory *tier_memory;
    memtier_policy_t policy = MEMTIER_POLICY_MAX_VALUE;
    unsigned i;
    struct tier_cfg {
        memkind_t kind;
        unsigned ratio;
    };

    struct tier_cfg temp_cfg[MAX_KIND];

    int ret;
    char *sptr = NULL;
    char *qbuf = env_var_local;

    unsigned query_count = 1;
    while (*qbuf)
        if (*qbuf++ == *CTL_STRING_QUERY_SEPARATOR)
            ++query_count;

    unsigned tier_count = query_count - 1;

    qbuf = strtok_r(env_var_local, CTL_STRING_QUERY_SEPARATOR, &sptr);
    if (qbuf == NULL) {
        log_err("No valid query found in: %s", env_var_local);
        return NULL;
    }

    if (query_count < 2) {
        log_err("Too low number of queries in configuration string: %s",
                env_var_local);
        return NULL;
    }

    for (i = 0; i < tier_count; ++i) {
        memkind_t kind = NULL;
        unsigned ratio = 0;

        ret = ctl_parse_tier_query(qbuf, &kind, &ratio);
        if (ret != 0) {
            log_err("Failed to parse query: %s", qbuf);
            goto cleanup_after_failure;
        }

        temp_cfg[i].kind = kind;
        temp_cfg[i].ratio = ratio;
        qbuf = strtok_r(NULL, CTL_STRING_QUERY_SEPARATOR, &sptr);
    }

    qbuf = strtok_r(qbuf, CTL_VALUE_SEPARATOR, &sptr);
    if (!strcmp(qbuf, "POLICY")) {
        ret = ctl_parse_policy(sptr, &policy);
        if (ret != 0) {
            goto cleanup_after_failure;
        }
    } else {
        log_err("Missing POLICY parameter in the query: %s", qbuf);
        goto cleanup_after_failure;
    }

    struct memtier_builder *builder = memtier_builder_new(policy);
    if (!builder) {
        log_err("Failed to parse policy: %s", qbuf);
        goto cleanup_after_failure;
    }

    for (i = 0; i < tier_count; ++i) {
        ret = memtier_builder_add_tier(builder, temp_cfg[i].kind,
                                       temp_cfg[i].ratio);
        if (ret != 0) {
            log_err("Failed to add tier%s", qbuf);
            goto destroy_builder;
        }
    }

    // TODO update other parameters supported by CTL
    // TODO move all code that set default params from memkind_memtier to CTL
    char *thresholds_var_string = utils_get_env("MEMKIND_MEM_THRESHOLDS");
    if (thresholds_var_string) {
        char thresholds_var_local[MAX_ENV_STRING] = {0};
        strncpy(thresholds_var_local, thresholds_var_string,
                MAX_ENV_STRING - 1);

        if (policy != MEMTIER_POLICY_DYNAMIC_THRESHOLD) {
            log_err("MEMKIND_MEM_THRESHOLDS env var could be used only with "
                    "DYNAMIC_THRESHOLD policy");
            goto destroy_builder;
        }

        char *qbuf = thresholds_var_local;
        unsigned thresholds_count = 1;
        while (*qbuf)
            if (*qbuf++ == *CTL_STRING_QUERY_SEPARATOR)
                ++thresholds_count;

        if (thresholds_count >= tier_count) {
            log_err("Too many thresholds defined");
            goto destroy_builder;
        }

        qbuf =
            strtok_r(thresholds_var_local, CTL_STRING_QUERY_SEPARATOR, &sptr);
        if (qbuf == NULL) {
            log_err("No valid thresholds found in: %s", thresholds_var_local);
            goto destroy_builder;
        }

        for (i = 0; i < thresholds_count; ++i) {

            ret = ctl_parse_threshold_query(qbuf, i, builder);
            if (ret != 0) {
                log_err("Failed to parse threshold: %s", qbuf);
                goto destroy_builder;
            }

            qbuf = strtok_r(NULL, CTL_STRING_QUERY_SEPARATOR, &sptr);
        }
    }

    tier_memory = memtier_builder_construct_memtier_memory(builder);
    if (!tier_memory) {
        goto destroy_builder;
    }

    memtier_builder_delete(builder);
    return tier_memory;

destroy_builder:
    memtier_builder_delete(builder);

cleanup_after_failure:
    ctl_destroy_fs_dax_reg();

    return NULL;
}

void ctl_destroy_tier_memory(struct memtier_memory *memory)
{
    ctl_destroy_fs_dax_reg();
    memtier_delete_memtier_memory(memory);
}
