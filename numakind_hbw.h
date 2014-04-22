/*
 * Copyright (2014) Intel Corporation All Rights Reserved.
 *
 * This software is supplied under the terms of a license
 * agreement or nondisclosure agreement with Intel Corp.
 * and may not be copied or disclosed except in accordance
 * with the terms of that agreement.
 *
 */

#ifndef numakind_hbw_include_h
#define numakind_hbw_include_h
#ifdef __cplusplus
extern "C" {
#endif

static const char *NUMAKIND_BANDWIDTH_PATH = "/etc/numakind/node-bandwidth";

int numakind_hbw_is_available(void);
int numakind_hbw_get_nodemask(unsigned long *nodemask, unsigned long maxnode);

#ifdef __cplusplus
}
#endif
#endif
