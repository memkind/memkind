// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <tiering/memtier_log.h>

#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CTL_VALUE_SEPARATOR        ":"
#define CTL_STRING_QUERY_SEPARATOR ","

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

static int ctl_validate_kind_name(const char *kind_name)
{
    if (strcmp(kind_name, "DRAM") && strcmp(kind_name, "FS_DAX")) {
        log_err("Unsupported kind: %s", kind_name);
        return -1;
    }

    return 0;
}

// TODO: Parse pmem_size string to unsigned long long supporting kMGT suffixes
static int ctl_validate_pmem_size(const char *pmem_size)
{
    if (pmem_size == NULL) {
        log_err("PMEM size not provided");
        return -1;
    }

    regex_t regex;
    int ret = regcomp(&regex, "[[:digit:]+]G", 0);
    if (ret) {
        log_err(
            "Unsuccessful compilation of regex occurred during validation of PMEM size");
        return -1;
    }
    int regex_ret = regexec(&regex, pmem_size, 0, NULL, 0);
    if (regex_ret == REG_NOMATCH) {
        log_err("Unsupported pmem_size format: %s", pmem_size);
        return -1;
    }

    return 0;
}

static int ctl_parse_ratio(const char *ratio_str, unsigned *dest)
{
    if (ratio_str == NULL) {
        log_err("Ratio not provided");
        return -1;
    }

    int ret = ctl_parse_u(ratio_str, dest);
    if (ret != 0) {
        log_err("Unsupported ratio: %s", ratio_str);
        dest = NULL;
        return -1;
    }

    return 0;
}

/*
 * ctl_parse_query -- (internal) splits an entire query string
 * into single queries
 */
static int ctl_parse_query(char *qbuf, char **kind_name, char **pmem_path,
                           char **pmem_size, unsigned *ratio_value)
{
    char *sptr = NULL;
    *kind_name = strtok_r(qbuf, CTL_VALUE_SEPARATOR, &sptr);
    if (*kind_name == NULL) {
        log_err("Kind name string not found in: %s", qbuf);
        return -1;
    }
    int ret = ctl_validate_kind_name(*kind_name);
    if (ret != 0) {
        return -1;
    }

    if (!strcmp(*kind_name, "FS_DAX")) {
        *pmem_path = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
        *pmem_size = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
        ret = ctl_validate_pmem_size(*pmem_size);
        if (ret != 0) {
            return -1;
        }
    } else {
        pmem_path = NULL;
        pmem_size = NULL;
    }

    char *ratio_value_str = NULL;
    ratio_value_str = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
    ret = ctl_parse_ratio(ratio_value_str, ratio_value);
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

/*
 * ctl_load_config -- splits an entire config into query strings
 */
int ctl_load_config(char *buf, char **kind_name, char **pmem_path,
                    char **pmem_size, unsigned *ratio_value)
{
    int ret = 0;
    char *sptr = NULL;

    // TODO: Allow multiple kinds to be created
    char *qbuf = strtok_r(buf, CTL_STRING_QUERY_SEPARATOR, &sptr);
    if (qbuf == NULL) {
        log_err("No valid query found in: %s", buf);
        return -1;
    }
    while (qbuf != NULL) {
        ret =
            ctl_parse_query(qbuf, kind_name, pmem_path, pmem_size, ratio_value);
        if (ret != 0) {
            log_err("Failed to parse query: %s", qbuf);
            return -1;
        }

        qbuf = strtok_r(NULL, CTL_STRING_QUERY_SEPARATOR, &sptr);
    }

    return 0;
}
