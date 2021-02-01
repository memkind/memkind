# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import json
import pytest
from python_framework import CMD_helper

class Test_malloc_stats_print(object):

    cmd_helper = CMD_helper()
    bin_path = cmd_helper.get_command_path("../malloc_stats_print_test_helper")
    fail_msg = "Test failed with:\n {0}"
    bin_param = ""

    def run_test_binary(self):
        command = " ".join([self.bin_path, self.bin_param])
        output, retcode = self.cmd_helper.execute_cmd(command)
        assert retcode == 0, \
               self.fail_msg.format(f"\nError: Execution of \'{command}\' returns {retcode}. Output: {output}")
        return output

    @pytest.mark.parametrize("bin_param", ["default", "stdout", "no_write_cb"])
    def test_TC_MEMKIND_malloc_stats_print_check_output(self, bin_param):
        """ This test checks if there is output from malloc_stats_print() memkind API function """
        self.bin_param = bin_param
        output = self.run_test_binary()
        assert output.endswith("--- End jemalloc statistics ---\n"), \
               f"Error: Didn't get the expected output from the '{self.bin_path}' binary."

    def test_TC_MEMKIND_malloc_stats_print_multi_opt_test(self):
        """ This test checks output from malloc_stats_print() memkind API function when options
        "OMIT_GENERAL | OMIT_PER_SIZE_CLASS_LARGE" are being passed to the function """
        self.bin_param = "pass_opts"
        output = self.run_test_binary()
        assert "Build-time option settings" not in output
        assert r"large:\s+size ind" not in output
        assert json.loads(output), \
               f"Error: There should be no output from the '{self.bin_path}' binary."

    def test_TC_MEMKIND_malloc_stats_print_negative_test(self):
        """ This test checks if there is no output from malloc_stats_print() memkind API function when wrong arguments
            are being passed to the function """
        self.bin_param = "negative_test"
        output = self.run_test_binary()
        assert len(output) == 0, \
               f"Error: There should be no output from the '{self.bin_path}' binary."
