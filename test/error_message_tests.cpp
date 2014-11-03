#include <memkind.h>
#include <gtest.h>


class ErrorMessage: public :: testing::Test {
protected:
    int all_error_code[] = {
        MEMKIND_ERROR_UNAVAILABLE,
        MEMKIND_ERROR_MBIND,
        MEMKIND_ERROR_MMAP,
        MEMKIND_ERROR_MEMALIGN,
        MEMKIND_ERROR_MALLCTL,
        MEMKIND_ERROR_MALLOC,
        MEMKIND_ERROR_GETCPU,
        MEMKIND_ERROR_PMTT,
        MEMKIND_ERROR_TIEDISTANCE,
        MEMKIND_ERROR_ALIGNMENT,
        MEMKIND_ERROR_MALLOCX,
        MEMKIND_ERROR_ENVIRON,
        MEMKIND_ERROR_INVALID,
        MEMKIND_ERROR_REPNAME,
        MEMKIND_ERROR_TOOMANY,
        MEMKIND_ERROR_PTHREAD,
        MEMKIND_ERROR_BADOPS,
        MEMKIND_ERROR_RUNTIME
    };
    int num_error_code = sizeof(all_error_code);
 };

TEST_F(ErrorMessage, message_length)
{
    int i;
    int max_len = MEMKIND_ERROR_MESSAGE_SIZE - 1;
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    for (i = 0; i < num_error_code; i++) {
        memkind_error_message(all_error_code[i], error_message, MEMKIND_ERROR_MESSAGE_SIZE);
        EXPECT_TRUE(strlen(error_message) < MEMKIND_ERROR_MESSAGE_SIZE - 1);
    }
}

TEST_F(ErrorMessage, message_format)
{
    int i;
    int max_len = MEMKIND_ERROR_MESSAGE_SIZE - 1;
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    for (i = 0; i < num_error_code; i++) {
        memkind_error_message(all_error_code[i], error_message, MEMKIND_ERROR_MESSAGE_SIZE);
        EXPECT_TRUE(strncmp(error_message, "<memkind>", strlen("<memkind>")) == 0);
    }
}

TEST_F(ErrorMEssage, undefined_message)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(-0xdeadbeef, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    EXPECT_TRUE(strncmp(error_message, "<memkind> Undefined error number:", strlen("<memkind> Undefined error number:")) == 0);
}

