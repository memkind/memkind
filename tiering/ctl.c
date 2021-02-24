// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include "memkind/internal/memkind_log.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CTL_VALUE_SEPARATOR ":"
#define CTL_STRING_QUERY_SEPARATOR ","

#ifdef __cplusplus
extern "C" {
#endif

static int ctl_validate_kind_name(const char *kind_name)
{
    if (kind_name == NULL) {
        log_err("Kind name not provided");
		return -1;
    }
    if (strcmp(kind_name, "DRAM")) {
        log_err("Unsupported kind: %s", kind_name);
        return -1;
    }
    
    return 0;
}

static int ctl_validate_ratio(const char *ratio_str)
{
    if (ratio_str == NULL) {
        log_err("Ratio not provided");
		return -1;
    }
    int ratio = (int)strtol(ratio_str, (char **)NULL, 10);
    if (ratio <= 0 || errno == ERANGE) {
        log_err("Unsupported ratio: %s", ratio_str);
        return -1;
    }

    return 0;
}

/*
 * ctl_parse_query -- (internal) splits an entire query string
 *	into kind_name and ratio_value
 */
static int ctl_parse_query(char *qbuf, char **kind_name, char **ratio_value)
{
    // TODO: Handle fsdax case: MEMKIND_TIERING_CONFIG=FS_DAX:/mnt/pmem1/:10G:1
	char *sptr = NULL;
	*kind_name = strtok_r(qbuf, CTL_VALUE_SEPARATOR, &sptr);
    int ret = ctl_validate_kind_name(*kind_name);
    if (ret != 0) {
        return -1;
    }

	*ratio_value = strtok_r(NULL, CTL_VALUE_SEPARATOR, &sptr);
    ret = ctl_validate_ratio(*ratio_value);
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

int ctl_load_config(char *buf, char **kind_name, char **ratio_value)
{
	int r = 0;
	char *sptr = NULL;

    //TODO: Allow multiple kinds to be created
	char *qbuf = strtok_r(buf, CTL_STRING_QUERY_SEPARATOR, &sptr);
	while (qbuf != NULL) {
		r = ctl_parse_query(qbuf, kind_name, ratio_value);
		if (r != 0) {
			log_err("failed to parse query %s", qbuf);
			return -1;
		}

		qbuf = strtok_r(NULL, CTL_STRING_QUERY_SEPARATOR, &sptr);
	}

	return 0;
}

#ifdef __cplusplus
}
#endif
