// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include "memkind.h"

#include "common.h"
#include "config.h"
#include "decorator_test.h"

size_t size = 16;
memkind_t kind = MEMKIND_DEFAULT;

class DecoratorTest: public ::testing::Test
{

protected:
    void SetUp()
    {
        decorators_state = new decorators_flags();
    }

    void TearDown()
    {
        free(decorators_state);
    }
};

TEST_F(DecoratorTest, test_TC_MEMKIND_DT_malloc)
{
#ifdef MEMKIND_DECORATION_ENABLED
    void *buffer = memkind_malloc(kind, size);

    ASSERT_TRUE(buffer != NULL);
    EXPECT_EQ(1, decorators_state->malloc_pre);
    EXPECT_EQ(1, decorators_state->malloc_post);

    memkind_free(0, buffer);
#endif
}

TEST_F(DecoratorTest, test_TC_MEMKIND_DT_calloc)
{
#ifdef MEMKIND_DECORATION_ENABLED
    void *buffer = memkind_calloc(kind, 1, size);

    ASSERT_TRUE(buffer != NULL);
    EXPECT_EQ(1, decorators_state->calloc_pre);
    EXPECT_EQ(1, decorators_state->calloc_post);

    memkind_free(0, buffer);
#endif
}

TEST_F(DecoratorTest, test_TC_MEMKIND_DT_posix_memalign)
{
#ifdef MEMKIND_DECORATION_ENABLED
    void *buffer;

    int res = memkind_posix_memalign(kind, &buffer, 8, size);

    ASSERT_TRUE(buffer != NULL);
    ASSERT_EQ(0, res);
    EXPECT_EQ(1, decorators_state->posix_memalign_pre);
    EXPECT_EQ(1, decorators_state->posix_memalign_post);

    memkind_free(0, buffer);
#endif
}

TEST_F(DecoratorTest, test_TC_MEMKIND_DT_realloc)
{
#ifdef MEMKIND_DECORATION_ENABLED
    void *buffer = memkind_realloc(kind, NULL, size);

    ASSERT_TRUE(buffer != NULL);
    EXPECT_EQ(1, decorators_state->realloc_pre);
    EXPECT_EQ(1, decorators_state->realloc_post);

    memkind_free(0, buffer);
#endif
}

TEST_F(DecoratorTest, test_TC_MEMKIND_DT_free)
{
#ifdef MEMKIND_DECORATION_ENABLED
    void *buffer = memkind_malloc(kind, size);

    ASSERT_TRUE(buffer != NULL);

    memkind_free(0, buffer);

    EXPECT_EQ(1, decorators_state->free_pre);
    EXPECT_EQ(1, decorators_state->free_post);
#endif
}
