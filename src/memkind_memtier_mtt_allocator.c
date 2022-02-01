// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include <memkind/internal/memkind_memtier_mtt_allocator.h>
#include <memkind/internal/memkind_private.h>

pthread_t bg_thread;

static void *mtt_internal_create()
{
    // TODO: Add an implementation
    return NULL;
}
static void mtt_internal_destroy()
{
    // TODO: Add an implementation
}

//--------------------------------------------------------------------------
// ----------------------------- Public API --------------------------------
//--------------------------------------------------------------------------

MEMKIND_EXPORT void memkind_mtt_create()
{
    bg_thread = pthread_create(&bg_thread, NULL, &mtt_internal_create, NULL);
}

MEMKIND_EXPORT void memkind_mtt_destroy()
{
    mtt_internal_destroy();
    void *ret;
    pthread_join(bg_thread, &ret);
}
