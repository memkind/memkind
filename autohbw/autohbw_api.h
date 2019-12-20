// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2016 Intel Corporation. */

///////////////////////////////////////////////////////////////////////////
// File   : autohbw_api.h
// Purpose: Header file to include, to use API calls to AutoHBW
// Author : Ruchira Sasanka (ruchira.sasanka AT intel.com)
// Date   : Jan 30, 2015
///////////////////////////////////////////////////////////////////////////

#ifndef AUTOHBW_API_H
#define AUTOHBW_API_H

#ifdef __cplusplus
extern "C" {
#endif

// Temporarily enable HBM allocations
void enableAutoHBW();

// Temporarily disable HBM allocations
void disableAutoHBW();

#ifdef __cplusplus
}
#endif

#endif
