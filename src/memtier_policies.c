// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#include <memkind/internal/memtier_private.h>

struct static_threshold_tier_data {
    float normalized_ratio;
    float actual_ratio;
};

struct memtier_policy_static_threshold_priv_t {
    struct static_threshold_tier_data *tiers;
} memtier_policy_static_threshold_priv;

int memtier_policy_static_threshold_create(struct memtier_builder *builder)
{
    unsigned int i;

    struct memtier_policy_static_threshold_priv_t *priv =
        &memtier_policy_static_threshold_priv;
    priv->tiers =
        calloc(builder->size, sizeof(struct static_threshold_tier_data));
    if (priv->tiers == NULL) {
        return -1;
    }

    // calculate and store normalized (vs tier[0]) tier ratios
    for (i = 0; i < builder->size; ++i) {
        priv->tiers[i].normalized_ratio =
            builder->cfg[i].tier_ratio / builder->cfg[0].tier_ratio;
    }
    priv->tiers[0].normalized_ratio = 1;
    priv->tiers[0].actual_ratio = 1;

    return 0;
}

struct memtier_tier *
memtier_policy_static_threshold_get_tier(struct memtier_kind *tier_kind)
{
    struct memtier_policy_static_threshold_priv_t *priv =
        &memtier_policy_static_threshold_priv;
    struct memtier_tier_cfg *cfg = tier_kind->builder->cfg;

    // algorithm:
    // * get allocation sizes from all tiers
    // * calculate actual ratio between tiers
    // * choose tier with largest difference between actual and desired ratio

    int i;
    int worse_tier = -1;
    double worse_ratio;
    for (i = 0; i < tier_kind->builder->size; ++i) {
        priv->tiers[i].actual_ratio =
            (float)cfg[i].tier->alloc_size / cfg[0].tier->alloc_size;
        double ratio_distance =
            (float)priv->tiers[i].actual_ratio / cfg[i].tier_ratio;
        if ((worse_tier == -1) || (cfg[i].tier->alloc_size == 0) ||
            (ratio_distance < worse_ratio)) {
            worse_tier = i;
            worse_ratio = ratio_distance;
        }
    }

    return cfg[worse_tier].tier;
}

struct memtier_policy MEMTIER_POLICY_STATIC_THRESHOLD_OBJ = {
    .create = memtier_policy_static_threshold_create,
    .get_tier = memtier_policy_static_threshold_get_tier,
    .name = "POLICY_STATIC_THRESHOLD",
    .priv = (void *)&memtier_policy_static_threshold_get_tier,
};
