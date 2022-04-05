// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/memkind_log.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CTL_VALUE_SEPARATOR ":"

/*
 * ctl_parse_u -- parses and returns an unsigned integer
 */
int ctl_parse_u(const char *str, unsigned *dest)
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
 * ctl_parse_size_t -- parses string and returns a size_t integer
 */
int ctl_parse_size_t(const char *str, size_t *dest)
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
int ctl_parse_size(char **sptr, size_t *sizep)
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
