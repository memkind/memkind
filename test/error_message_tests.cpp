#include <memkind.h>
#include <gtest/gtest.h>


class ErrorMessage: public :: testing :: Test {
protected:
    int num_error_code;
    int *all_error_code;

    void SetUp() {
        num_error_code = 18;
        all_error_code = new int[num_error_code];
        all_error_code[0] = MEMKIND_ERROR_UNAVAILABLE;
        all_error_code[1] = MEMKIND_ERROR_MBIND;
        all_error_code[2] = MEMKIND_ERROR_MMAP;
        all_error_code[3] = MEMKIND_ERROR_MEMALIGN;
        all_error_code[4] = MEMKIND_ERROR_MALLCTL;
        all_error_code[5] = MEMKIND_ERROR_MALLOC;
        all_error_code[6] = MEMKIND_ERROR_GETCPU;
        all_error_code[7] = MEMKIND_ERROR_PMTT;
        all_error_code[8] = MEMKIND_ERROR_TIEDISTANCE;
        all_error_code[9] = MEMKIND_ERROR_ALIGNMENT;
        all_error_code[10] = MEMKIND_ERROR_MALLOCX;
        all_error_code[11] = MEMKIND_ERROR_ENVIRON;
        all_error_code[12] = MEMKIND_ERROR_INVALID;
        all_error_code[13] = MEMKIND_ERROR_REPNAME;
        all_error_code[14] = MEMKIND_ERROR_TOOMANY;
        all_error_code[15] = MEMKIND_ERROR_PTHREAD;
        all_error_code[16] = MEMKIND_ERROR_BADOPS;
        all_error_code[17] = MEMKIND_ERROR_RUNTIME;
    }
    void TearDown() {
        delete all_error_code;
    }
};

TEST_F(ErrorMessage, message_length)
{
    int i;
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    for (i = 0; i < num_error_code; i++) {
        memkind_error_message(all_error_code[i], error_message, MEMKIND_ERROR_MESSAGE_SIZE);
        EXPECT_TRUE(strlen(error_message) < MEMKIND_ERROR_MESSAGE_SIZE - 1);
    }
}

TEST_F(ErrorMessage, message_format)
{
    int i;
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    for (i = 0; i < num_error_code; i++) {
        memkind_error_message(all_error_code[i], error_message, MEMKIND_ERROR_MESSAGE_SIZE);
        EXPECT_TRUE(strncmp(error_message, "<memkind>", strlen("<memkind>")) == 0);
    }
}

TEST_F(ErrorMessage, undefined_message)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(-0xdeadbeef, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    EXPECT_TRUE(strncmp(error_message, "<memkind> Undefined error number:", strlen("<memkind> Undefined error number:")) == 0);
}

