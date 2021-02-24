// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "tiering/memtier_log.h"

#include <errno.h>
#include <regex.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CTL_VALUE_SEPARATOR        ":"
#define CTL_STRING_QUERY_SEPARATOR ","

/*
 * ctl_parse_ll -- (internal) parses and returns a long long signed integer
 */
static int ctl_parse_ll(const char *str, unsigned long long *dest)
{
    char *endptr;
    int olderrno = errno;
    errno = 0;
    long long val;
    if (str[0] != '-') {
        val = strtoll(str, &endptr, 0);
    } else {
        return -1;
    }
    if (endptr == str || errno != 0) {
        return -1;
    }
    errno = olderrno;
    *dest = val;

    return 0;
}

static int ctl_validate_kind_name(const char *kind_name)
{
    if (kind_name == NULL) {
        log_err("Kind name not provided");
        return -1;
    }
    if (strcmp(kind_name, "DRAM") && strcmp(kind_name, "FS_DAX")) {
        log_err("Unsupported kind: %s", kind_name);
        return -1;
    }

    return 0;
}

// TODO: Parse pmem_size string to long long supporting kMGT suffixes
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

static int ctl_parse_ratio(const char *ratio_str, unsigned long long *dest)
{
    if (ratio_str == NULL) {
        log_err("Ratio not provided");
        return -1;
    }

    int olderrno = errno;
    errno = 0;
    int ret = ctl_parse_ll(ratio_str, dest);
    if (ret != 0 || errno != 0) {
        log_err("Unsupported ratio: %s", ratio_str);
        dest = NULL;
        return -1;
    }
    errno = olderrno;

    return 0;
}

/*
 * ctl_parse_query -- (internal) splits an entire query string
 *	into kind_name and ratio_value
 */
static int ctl_parse_query(char *qbuf, char **kind_name, char **pmem_path,
                           char **pmem_size, unsigned long long *ratio_value)
{
    char *sptr = NULL;
    *kind_name = strtok_r(qbuf, CTL_VALUE_SEPARATOR, &sptr);
    int ret = ctl_validate_kind_name(*kind_name);
    if (ret != 0) {
        return -1;
    }

    if (strcmp(*kind_name, "FS_DAX")) {
        *pmem_path = "N/A";
        *pmem_size = "N/A";
    } else {
        *pmem_path = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
        *pmem_size = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
        ret = ctl_validate_pmem_size(*pmem_size);
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

int ctl_load_config(char *buf, char **kind_name, char **pmem_path,
                    char **pmem_size, unsigned long long *ratio_value)
{
    int r = 0;
    char *sptr = NULL;

    // TODO: Allow multiple kinds to be created
    char *qbuf = strtok_r(buf, CTL_STRING_QUERY_SEPARATOR, &sptr);
    while (qbuf != NULL) {
        r = ctl_parse_query(qbuf, kind_name, pmem_path, pmem_size, ratio_value);
        if (r != 0) {
            log_err("failed to parse query %s", qbuf);
            return -1;
        }

        qbuf = strtok_r(NULL, CTL_STRING_QUERY_SEPARATOR, &sptr);
    }

    return 0;
}
