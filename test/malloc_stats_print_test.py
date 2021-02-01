# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

from python_framework import CMD_helper

class Test_malloc_stats_print(object):

    cmd_helper = CMD_helper()

    def test_TC_MEMKIND_malloc_stats_print_check_output(self):
        """ This test checks if there is output from malloc_stats_print() memkind API function """
        bin_path = "../malloc_stats_print_test_helper"
        bin_param = "default"
        command = " ".join([bin_path, bin_param])
        output, retcode = self.cmd_helper.execute_cmd(self.cmd_helper.get_command_path(command))
        fail_msg = f"Test failed with:\nError: Execution of \'{command}\' returns {retcode}, \noutput: {output}"
        assert retcode == 0, fail_msg
        assert output.endswith("--- End jemalloc statistics ---\n")
