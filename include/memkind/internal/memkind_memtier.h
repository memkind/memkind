// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2021 Intel Corporation. */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <memkind.h>
#include <stdlib.h>

/**
 * Header file for the memkind heap manager.
 * More details in memtier(3) man page.
 *
 * API standards are described in memtier(3) man page.
 */

/// \brief Forward declaration
struct memtier_builder;
struct memtier_memory;

typedef enum memtier_policy_t
{
    /**
     * Static Ratio policy
     */
    MEMTIER_POLICY_STATIC_RATIO = 0,

    /**
     * Dynamic Threshold policy
     */
    MEMTIER_POLICY_DYNAMIC_THRESHOLD = 1,

    MEMTIER_POLICY_DATA_HOTNESS = 2,

    /**
     * Max policy value.
     */
    MEMTIER_POLICY_MAX_VALUE
} memtier_policy_t;

///
/// \brief Create a memtier builder
/// \note STANDARD API
/// \param policy memtier policy
/// \return memtier builder, NULL on failure
///
struct memtier_builder *memtier_builder_new(memtier_policy_t policy);

///
/// \brief Delete memtier builder
/// \note STANDARD API
/// \param builder memtier builder
///
void memtier_builder_delete(struct memtier_builder *builder);

///
/// \brief Add memtier tier to memtier builder
/// \note STANDARD API
/// \param builder memtier builder
/// \param kind memkind kind
/// \param kind_ratio expected memtier tier ratio
/// \return Operation status, 0 on success, other values on
/// failure
///
int memtier_builder_add_tier(struct memtier_builder *builder, memkind_t kind,
                             unsigned kind_ratio);

///
/// \brief Construct a memtier memory
/// \note STANDARD API
/// \param builder memtier builder
/// \return Pointer to constructed memtier memory object
///
struct memtier_memory *
memtier_builder_construct_memtier_memory(struct memtier_builder *builder);

///
/// \brief Delete memtier memory
/// \note STANDARD API
/// \param memory memtier memory
///
void memtier_delete_memtier_memory(struct memtier_memory *memory);

///
/// \brief Allocates size bytes of uninitialized storage of the specified
///        memtier memory
/// \note STANDARD API
/// \param memory specified memtier memory
/// \param size number of bytes to allocate
/// \return Pointer to the allocated memory
///
void *memtier_malloc(struct memtier_memory *memory, size_t size);

///
/// \brief Allocates size bytes of uninitialized storage of the specified kind
/// \note STANDARD API
/// \param kind specified memkind kind
/// \param size number of bytes to allocate
/// \return Pointer to the allocated memory
void *memtier_kind_malloc(memkind_t kind, size_t size);

///
/// \brief Allocates memory of the specified memtier memory for an array of num
///        elements of size bytes each and initializes all bytes in the
///        allocated storage to zero
/// \note STANDARD API
/// \param memory specified memtier memory
/// \param num number of objects
/// \param size specified size of each element
/// \return Pointer to the allocated memory
///
void *memtier_calloc(struct memtier_memory *memory, size_t num, size_t size);

///
/// \brief Allocates memory of the specified kind for an array of num
///        elements of size bytes each and initializes all bytes in the
///        allocated storage to zero
/// \note STANDARD API
/// \param kind specified memkind kind
/// \param num number of objects
/// \param size specified size of each element
/// \return Pointer to the allocated memory
///
void *memtier_kind_calloc(memkind_t kind, size_t num, size_t size);

///
/// \brief Reallocates memory of the specified memtier memory
/// \note STANDARD API
/// \param memory specified memtier memory
/// \param ptr pointer to the memory block to be reallocated
/// \param size new size for the memory block in bytes
/// \return Pointer to the allocated memory
///
void *memtier_realloc(struct memtier_memory *memory, void *ptr, size_t size);

///
/// \brief Reallocates memory of the specified kind
/// \note STANDARD API
/// \param kind specified memkind kind
/// \param ptr pointer to the memory block to be reallocated
/// \param size new size for the memory block in bytes
/// \return Pointer to the allocated memory
///
void *memtier_kind_realloc(memkind_t kind, void *ptr, size_t size);

///
/// \brief Allocates size bytes of the specified memtier memory and places the
///        address of the allocated memory in *memptr. The address of the
//         allocated memory will be a multiple of alignment, which must be a
///        power of two and a multiple of sizeof(void*)
/// \note STANDARD API
/// \param memory specified memtier memory
/// \param memptr address of the allocated memory
/// \param alignment specified alignment of bytes
/// \param size specified size of bytes
/// \return operation status, 0 on success, EINVAL or
///         ENOMEM on failure
///
int memtier_posix_memalign(struct memtier_memory *memory, void **memptr,
                           size_t alignment, size_t size);

///
/// \brief Allocates size bytes of the specified kind and places the
///        address of the allocated memory in *memptr. The address of the
///        allocated memory will be a multiple of alignment, which must be a
///        power of two and a multiple of sizeof(void*)
/// \note STANDARD API
/// \param kind specified memkind kind
/// \param memptr address of the allocated memory
/// \param alignment specified alignment of bytes
/// \param size specified size of bytes
/// \return operation status, 0 on success, EINVAL or
///         ENOMEM on failure
///
int memtier_kind_posix_memalign(memkind_t kind, void **memptr, size_t alignment,
                                size_t size);

///
/// \brief Obtain size of block of memory allocated with the memtier API
/// \note STANDARD API
/// \param ptr pointer to the allocated memory
/// \return Number of usable bytes
///
size_t memtier_usable_size(void *ptr);

///
/// \brief Free the memory space allocated with the memtier_kind API
/// \note STANDARD API
/// \param kind specified memkind kind
/// \param ptr pointer to the allocated memory
///
void memtier_kind_free(memkind_t kind, void *ptr);

///
/// \brief Free the memory space allocated with the memtier API
/// \note STANDARD API
/// \param ptr pointer to the allocated memory
///
static inline void memtier_free(void *ptr)
{
    memtier_kind_free(NULL, ptr);
}

///
/// \brief Obtain size of allocated memory with the memtier API inside
///        specified kind
/// \note STANDARD API
/// \param kind specified memkind kind
/// \return Number of usable bytes
///
size_t memtier_kind_allocated_size(memkind_t kind);

///
/// \brief Set memtier property
/// \note STANDARD API
/// \param builder memtier builder
/// \param name name of the property
/// \param val value to set
/// \return Operation status, 0 on success, other values on
/// failure
///
int memtier_ctl_set(struct memtier_builder *builder, const char *name,
                    const void *val);

#ifdef __cplusplus
}
#endif
