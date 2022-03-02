// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include "stddef.h"
#include "stdint.h"

/// @brief Convert @p size in bytes size rank
///
/// @return size_rank corresponding to @p size
///
/// @note this function is heavily optimised and might lack readability
///
/// @warning exact inverse calculation is not possible
/// @see size_rank_to_size for lossy inverse conversion
extern size_t size_to_rank_size(size_t size);

/// @brief Convert @p size_rank to absolute size in bytes
///
/// @return size, in bytes, corresponding to @p size_rank
///
/// @note this function is heavily optimised and might lack readability
/// @see size_to_size_rank
extern size_t rank_size_to_size(size_t size_rank);
