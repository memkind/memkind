// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2014 - 2020 Intel Corporation. */

#include <memkind.h>

#include <errno.h>
#include <gtest/gtest.h>

/* Tests which calls APIS in wrong ways to generate Error Messages thrown by the
 * the memkind library
 */

const int all_error_code[] = {MEMKIND_ERROR_UNAVAILABLE,
                              MEMKIND_ERROR_MBIND,
                              MEMKIND_ERROR_MMAP,
                              MEMKIND_ERROR_MALLOC,
                              MEMKIND_ERROR_RUNTIME,
                              MEMKIND_ERROR_ENVIRON,
                              MEMKIND_ERROR_INVALID,
                              MEMKIND_ERROR_TOOMANY,
                              MEMKIND_ERROR_BADOPS,
                              MEMKIND_ERROR_HUGETLB,
                              MEMKIND_ERROR_ARENAS_CREATE,
                              MEMKIND_ERROR_OPERATION_FAILED,
                              MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE,
                              EINVAL,
                              ENOMEM};

class ErrorMessage: public ::testing ::Test
{
protected:
    void SetUp()
    {}
    void TearDown()
    {}
};

TEST_F(ErrorMessage, test_TC_MEMKIND_ErrorMsgLength)
{
    size_t i;
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    for (i = 0; i < sizeof(all_error_code) / sizeof(all_error_code[0]); ++i) {
        memkind_error_message(all_error_code[i], error_message,
                              MEMKIND_ERROR_MESSAGE_SIZE);
        EXPECT_TRUE(strlen(error_message) < MEMKIND_ERROR_MESSAGE_SIZE - 1);
    }
    memkind_error_message(MEMKIND_ERROR_UNAVAILABLE, NULL, 0);
}

TEST_F(ErrorMessage, test_TC_MEMKIND_ErrorMsgFormat)
{
    size_t i;
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    for (i = 0; i < sizeof(all_error_code) / sizeof(all_error_code[0]); ++i) {
        memkind_error_message(all_error_code[i], error_message,
                              MEMKIND_ERROR_MESSAGE_SIZE);
        EXPECT_TRUE(strncmp(error_message, "<memkind>", strlen("<memkind>")) ==
                    0);
    }
}

TEST_F(ErrorMessage, test_TC_MEMKIND_ErrorMsgUndefMesg)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(-0xdeadbeef, error_message,
                          MEMKIND_ERROR_MESSAGE_SIZE);
    EXPECT_TRUE(strncmp(error_message, "<memkind> Undefined error number:",
                        strlen("<memkind> Undefined error number:")) == 0);
}
