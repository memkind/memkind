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
struct memtier_tier;
struct memtier_builder;
struct memtier_kind;

typedef enum memtier_policy_t
{
    /**
     * TODO FIX ME
     */
    MEMTIER_DUMMY_VALUE = 0,

} memtier_policy_t;

///
/// \brief Create a memtier tier
/// \note STANDARD API
/// \return memtier tier, NULL on failure
///
struct memtier_tier *memtier_tier_new(void);

///
/// \brief Delete memtier tier
/// \note STANDARD API
/// \param tier memtier tier
///
void memtier_tier_delete(struct memtier_tier *tier);

///
/// \brief Set memtier ratio of memtier tier
/// \note STANDARD API
/// \param tier memtier tier
/// \param ratio expected ratio of tier
/// \return Operation status, 0 on success, other values on
/// failure
///
int memtier_tier_set_ratio(struct memtier_tier *tier, size_t ratio);

///
/// \brief Set memory kind of memtier tier
/// \note STANDARD API
/// \param tier memtier tier
/// \param kind memkind kind
/// \return Operation status, 0 on success, other values on
/// failure
///
int memtier_tier_set_memory_kind(struct memtier_tier *tier, memkind_t kind);

///
/// \brief Create a memtier builder
/// \note STANDARD API
/// \return memtier builder, NULL on failure
///
struct memtier_builder *memtier_builder(void);

///
/// \brief Add memtier tier to memtier builder
/// \note STANDARD API
/// \param builder memtier builder
/// \param tier memtier tier
/// \return Operation status, 0 on success, other values on
/// failure
///
int memtier_builder_add_tier(struct memtier_builder *builder,
                             struct memtier_tier *tier);

///
/// \brief Set memtier policy to memtier builder
/// \note STANDARD API
/// \param builder memtier builder
/// \param policy memtier policy
/// \return Operation status, 0 on success, other values on
/// failure
///
int memtier_builder_set_policy(struct memtier_builder *builder,
                               memtier_policy_t policy);

///
/// \brief Construct a memtier kind
/// \note STANDARD API
/// \param builder memtier builder
/// \param kind pointer to memtier kind which will be created
/// \return Operation status, 0 on success, other values on
/// failure
///
int memtier_builder_construct_kind(struct memtier_builder *builder,
                                   struct memtier_kind **kind);

///
/// \brief Delete memtier kind
/// \note STANDARD API
/// \param kind memtier kind
///
void memtier_delete_kind(struct memtier_kind *kind);

///
/// \brief Allocates size bytes of uninitialized storage of the specified
///        tiering kind
/// \note STANDARD API
/// \param kind memtier kind
/// \param size number of bytes to allocate
/// \return Pointer to the allocated memory
///
void *memtier_kind_malloc(struct memtier_kind *kind, size_t size);

///
/// \brief Allocates size bytes of uninitialized storage of the specified
///        memtier tier
/// \note STANDARD API
/// \param tier specified memtier tier
/// \param size number of bytes to allocate
/// \return Pointer to the allocated memory
void *memtier_tier_malloc(struct memtier_tier *tier, size_t size);

///
/// \brief Allocates memory of the specified memtier kind for an array of num
///        elements of size bytes each and initializes all bytes in the
///        allocated storage to zero
/// \note STANDARD API
/// \param kind memtier kind
/// \param num number of objects
/// \param size specified size of each element
/// \return Pointer to the allocated memory
///
void *memtier_kind_calloc(struct memtier_kind *kind, size_t num, size_t size);

///
/// \brief Allocates memory of the specified memtier tier for an array of num
///        elements of size bytes each and initializes all bytes in the
///        allocated storage to zero
/// \note STANDARD API
/// \param tier specified memtier tier
/// \param num number of objects
/// \param size specified size of each element
/// \return Pointer to the allocated memory
///
void *memtier_tier_calloc(struct memtier_tier *tier, size_t num, size_t size);

///
/// \brief Reallocates memory of the specified memtier kind
/// \note STANDARD API
/// \param kind specified memtier kind
/// \param ptr pointer to the memory block to be reallocated
/// \param size new size for the memory block in bytes
/// \return Pointer to the allocated memory
///
void *memtier_kind_realloc(struct memtier_kind *kind, void *ptr, size_t size);

///
/// \brief Reallocates memory of the specified memtier tier
/// \note STANDARD API
/// \param tier specified memtier tier
/// \param ptr pointer to the memory block to be reallocated
/// \param size new size for the memory block in bytes
/// \return Pointer to the allocated memory
///
void *memtier_tier_realloc(struct memtier_tier *tier, void *ptr, size_t size);

///
/// \brief Allocates size bytes of the specified memtier kind and places the
///        address of the allocated memory in *memptr. The address of the
//         allocated memory will be a multiple of alignment, which must be a
///        power of two and a multiple of sizeof(void*)
/// \note STANDARD API
/// \param kind specified memtier kind
/// \param memptr address of the allocated memory
/// \param alignment specified alignment of bytes
/// \param size specified size of bytes
/// \return operation status, 0 on success, EINVAL or
///         ENOMEM on failure
///
int memtier_kind_posix_memalign(struct memtier_kind *kind, void **memptr,
                                size_t alignment, size_t size);

///
/// \brief Allocates size bytes of the specified memtier tier and places the
///        address of the allocated memory in *memptr. The address of the
///        allocated memory will be a multiple of alignment, which must be a
///        power of two and a multiple of sizeof(void*)
/// \note STANDARD API
/// \param tier specified memtier tier
/// \param memptr address of the allocated memory
/// \param alignment specified alignment of bytes
/// \param size specified size of bytes
/// \return operation status, 0 on success, EINVAL or
///         ENOMEM on failure
///
int memtier_tier_posix_memalign(struct memtier_tier *tier, void **memptr,
                                size_t alignment, size_t size);

///
/// \brief Obtain size of block of memory allocated with the memtier API
/// \note STANDARD API
/// \param ptr pointer to the allocated memory
/// \return Number of usable bytes
///
size_t memtier_usable_size(void *ptr);

///
/// \brief Free the memory space allocated with the memtier API
/// \note STANDARD API
/// \param ptr pointer to the allocated memory
///
void memtier_free(void *ptr);

#ifdef __cplusplus
}
#endif
