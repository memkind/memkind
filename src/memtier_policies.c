// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind/internal/memtier_private.h>

struct memtier_policy_circular_priv_t {
    int tier_id;
} memtier_policy_circular_priv;

int memtier_policy_circular_create(struct memtier_builder *builder)
{
    return 0;
}

struct memtier_tier *
memtier_policy_circular_get_tier(struct memtier_kind *tier_kind)
{
    unsigned temp_id =
        (memtier_policy_circular_priv.tier_id++) % tier_kind->builder->size;
    return tier_kind->builder->cfg[temp_id].tier;
}

struct memtier_policy MEMTIER_POLICY_CIRCULAR_OBJ = {
    .create = memtier_policy_circular_create,
    .get_tier = memtier_policy_circular_get_tier,
    .name = "POLICY_CIRCULAR",
    .priv = (void *)&memtier_policy_circular_priv,
};
