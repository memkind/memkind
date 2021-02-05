// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#include "ScenarioWorkload.h"

ScenarioWorkload::ScenarioWorkload(VectorIterator<Allocator *> *a,
                                   VectorIterator<size_t> *as,
                                   VectorIterator<int> *fc)
{
    allocators = a;
    func_calls = fc;
    alloc_sizes = as;
}

bool ScenarioWorkload::run()
{
    if (func_calls->has_next() && allocators->has_next() &&
        alloc_sizes->has_next()) {
        switch (func_calls->next()) {
            case FunctionCalls::MALLOC: {
                memory_operation data =
                    allocators->next()->wrapped_malloc(alloc_sizes->next());
                post_allocation_check(data);
                break;
            }
            case FunctionCalls::CALLOC: {
                memory_operation data =
                    allocators->next()->wrapped_calloc(1, alloc_sizes->next());
                post_allocation_check(data);
                break;
            }
            case FunctionCalls::REALLOC: {
                // Guarantee the memory for realloc.
                Allocator *allocator = allocators->next();
                memory_operation to_realloc = allocator->wrapped_malloc(512);

                memory_operation data = allocator->wrapped_realloc(
                    to_realloc.ptr, alloc_sizes->next());
                post_allocation_check(data);
                break;
            }
            case FunctionCalls::FREE: {
                memory_operation *data = get_allocated_memory();

                if (!allocations.empty() && (data != NULL)) {
                    allocator_factory.get_existing(data->allocator_type)
                        ->wrapped_free(data->ptr);
                    data->is_allocated = false;

                    memory_operation free_op = *data;
                    free_op.allocation_method = FunctionCalls::FREE;
                    allocations.push_back(free_op);
                }

                break;
            }
            default:
                assert(!"Function call identifier out of range.");
                break;
        }

        return true;
    }

    return false;
}

ScenarioWorkload::~ScenarioWorkload(void)
{
    for (int i = 0; i < allocations.size(); i++) {
        memory_operation data = allocations[i];
        if (data.is_allocated &&
            (data.allocation_method != FunctionCalls::FREE))
            allocator_factory.get_existing(data.allocator_type)
                ->wrapped_free(data.ptr);
    }
}

memory_operation *ScenarioWorkload::get_allocated_memory()
{
    for (int i = allocations.size() - 1; i >= 0; i--) {
        memory_operation *data = &allocations[i];
        if (data->is_allocated)
            return data;
    }

    return NULL;
}

void ScenarioWorkload::post_allocation_check(const memory_operation &data)
{
    allocations.push_back(data);
    if (touch_memory_on_allocation && (data.ptr != NULL) &&
        (data.error_code != ENOMEM)) {
        // Write memory to ensure physical allocation.
        memset(data.ptr, 1, data.size_of_allocation);
    }
}
