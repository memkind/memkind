// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2019 - 2020 Intel Corporation. */

#include "memkind.h"

#include "allocator_perf_tool/TimerSysTime.hpp"
#include "common.h"

class MemkindNullKindTests: public ::testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_DefaultKindFreeNullPtr)
{
    const double test_time = 5;

    TimerSysTime timer;
    timer.start();
    do {
        memkind_free(MEMKIND_DEFAULT, nullptr);
    } while (timer.getElapsedTime() < test_time);
}

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_NullKindFreeNullPtr)
{
    const double test_time = 5;

    TimerSysTime timer;
    timer.start();
    do {
        memkind_free(nullptr, nullptr);
    } while (timer.getElapsedTime() < test_time);
}

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_NullKindUsableSizeNullPtr)
{
    const double test_time = 5;

    TimerSysTime timer;
    timer.start();
    do {
        memkind_malloc_usable_size(nullptr, nullptr);
    } while (timer.getElapsedTime() < test_time);
}

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_DefaultRegularKindFreeNullPtr)
{
    const size_t size_1 = 1 * KB;
    const size_t size_2 = 1 * MB;
    void *ptr_default = nullptr;
    void *ptr_regular = nullptr;
    for (unsigned int i = 0; i < MEMKIND_MAX_KIND; ++i) {
        ptr_default = memkind_malloc(MEMKIND_DEFAULT, size_2);
        ASSERT_NE(nullptr, ptr_default);
        memkind_free(nullptr, ptr_default);
        ptr_default = memkind_malloc(MEMKIND_DEFAULT, size_1);
        ASSERT_NE(nullptr, ptr_default);
        memkind_free(nullptr, ptr_default);
        ptr_regular = memkind_malloc(MEMKIND_REGULAR, size_2);
        ASSERT_NE(nullptr, ptr_regular);
        memkind_free(nullptr, ptr_regular);
        ptr_regular = memkind_malloc(MEMKIND_REGULAR, size_1);
        ASSERT_NE(nullptr, ptr_regular);
        memkind_free(nullptr, ptr_regular);
    }
}

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_DefaultReallocNullptrSizeZero)
{
    const double test_time = 5;
    void *test_nullptr = nullptr;
    void *test_ptr_malloc = nullptr;
    void *test_ptr_realloc = nullptr;
    TimerSysTime timer;
    timer.start();
    do {
        test_ptr_malloc = memkind_malloc(MEMKIND_DEFAULT, 5 * MB);
        errno = 0;
        test_ptr_realloc = memkind_realloc(MEMKIND_DEFAULT, test_ptr_malloc, 0);
        ASSERT_EQ(test_ptr_realloc, nullptr);
        ASSERT_EQ(errno, 0);

        errno = 0;
        // equivalent to memkind_malloc(MEMKIND_DEFAULT,0)
        test_nullptr = memkind_realloc(MEMKIND_DEFAULT, nullptr, 0);
        ASSERT_EQ(test_nullptr, nullptr);
        ASSERT_EQ(errno, 0);
    } while (timer.getElapsedTime() < test_time);
}

TEST_F(MemkindNullKindTests,
       test_TC_MEMKIND_DefaultKindReallocNullKindSizeZeroImplicitFree)
{
    const double test_time = 5;
    size_t size = 1 * KB;
    void *test_ptr_malloc = nullptr;
    void *test_ptr_realloc = nullptr;
    TimerSysTime timer;
    timer.start();
    do {
        errno = 0;
        test_ptr_malloc = memkind_malloc(MEMKIND_DEFAULT, size);
        ASSERT_NE(test_ptr_malloc, nullptr);
        ASSERT_EQ(errno, 0);

        errno = 0;
        test_ptr_realloc = memkind_realloc(nullptr, test_ptr_malloc, 0);
        ASSERT_EQ(test_ptr_realloc, nullptr);
        ASSERT_EQ(errno, 0);
    } while (timer.getElapsedTime() < test_time);
}

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_DefaultKindReallocNullKindSizeMax)
{
    size_t size = 1 * KB;
    void *test_ptr_malloc = nullptr;
    void *test_ptr_realloc = nullptr;

    test_ptr_malloc = memkind_malloc(MEMKIND_DEFAULT, size);
    ASSERT_NE(test_ptr_malloc, nullptr);

    errno = 0;
    test_ptr_realloc = memkind_realloc(nullptr, test_ptr_malloc, SIZE_MAX);
    ASSERT_EQ(test_ptr_realloc, nullptr);
    ASSERT_EQ(errno, ENOMEM);

    memkind_free(nullptr, test_ptr_malloc);
}

TEST_F(MemkindNullKindTests,
       test_TC_MEMKIND_DefaultReallocIncreaseSizeNullKindVariant)
{
    size_t size = 1 * KB;
    char *test1 = nullptr;
    char *test2 = nullptr;
    const char val[] =
        "test_TC_MEMKIND_DefaultReallocIncreaseSizeNullKindVariant";
    int status;

    test1 = (char *)memkind_malloc(MEMKIND_DEFAULT, size);
    ASSERT_NE(test1, nullptr);

    sprintf(test1, "%s", val);

    size *= 2;
    test2 = (char *)memkind_realloc(nullptr, test1, size);
    ASSERT_NE(test2, nullptr);
    status = memcmp(val, test2, sizeof(val));
    ASSERT_EQ(status, 0);

    memkind_free(MEMKIND_DEFAULT, test2);
}

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_ReallocNullptrNullKind)
{
    size_t size = 1 * KB;

    errno = 0;
    void *test = memkind_realloc(nullptr, nullptr, size);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, EINVAL);
}

TEST_F(MemkindNullKindTests,
       test_TC_MEMKIND_DefaultReallocDecreaseSizeNullKindVariant)
{
    size_t size = 1 * KB;
    char *test1 = nullptr;
    char *test2 = nullptr;
    const char val[] =
        "test_TC_MEMKIND_DefaultReallocDecreaseSizeNullKindVariant";
    int status;

    test1 = (char *)memkind_malloc(MEMKIND_DEFAULT, size);
    ASSERT_NE(test1, nullptr);

    sprintf(test1, "%s", val);

    size = 4;
    test2 = (char *)memkind_realloc(nullptr, test1, size);
    ASSERT_NE(test2, nullptr);
    status = memcmp(val, test2, size);
    ASSERT_EQ(status, 0);

    memkind_free(MEMKIND_DEFAULT, test2);
}

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_ReallocNullptrNullKindSizeMax)
{
    errno = 0;
    void *test = memkind_realloc(nullptr, nullptr, SIZE_MAX);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, EINVAL);
}

TEST_F(MemkindNullKindTests, test_TC_MEMKIND_ReallocNullptrNullKindSizeZero)
{
    errno = 0;
    void *test = memkind_realloc(nullptr, nullptr, 0);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, EINVAL);
}
