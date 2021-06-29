# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import json
import pytest
from python_framework.cmd_helper import CMD_helper
import re


class Test_malloc_stats_print(object):

    cmd_helper = CMD_helper()
    bin_path = cmd_helper.get_command_path("../stats_print_test_helper")
    fail_msg = "Test failed with:\n {0}"
    bin_param = ""
    error_msg = "Error: {0} option not parsed correctly."

    def run_test_binary(self):
        command = " ".join([self.bin_path, self.bin_param])
        output, retcode = self.cmd_helper.execute_cmd(command)
        assert retcode == 0, \
            self.fail_msg.format(
                f"\nError: Execution of \'{command}\'",
                f" returns {retcode}. Output: {output}")
        return output, retcode

    @pytest.mark.parametrize("bin_param", ["default", "stdout", "no_write_cb"])
    def test_TC_MEMKIND_malloc_stats_print_check_output(self, bin_param):
        """ This test checks if there is output
            from malloc_stats_print() memkind API function """
        self.bin_param = bin_param
        output, _ = self.run_test_binary()
        assert output.endswith("--- End jemalloc statistics ---\n"), \
            "Error: Didn't get the expected output"\
            " from the '{self.bin_path}' binary."

    def test_TC_MEMKIND_malloc_stats_print_multi_opt_test(self):
        """ This test checks output from malloc_stats_print()
            memkind API function when options
            "MEMKIND_STAT_PRINT_JSON_FORMAT | MEMKIND_STAT_PRINT_OMIT_PER_ARENA
            | MEMKIND_STAT_PRINT_OMIT_EXTENT"
            are passed to the function """
        self.bin_param = "pass_opts"
        output, _ = self.run_test_binary()
        assert json.loads(output), \
            self.error_msg.format("MEMKIND_STAT_PRINT_JSON_FORMAT")
        assert "arenas[" not in output, \
               self.error_msg.format("MEMKIND_STAT_PRINT_OMIT_PER_ARENA")
        assert "extents:" not in output, \
               self.error_msg.format("MEMKIND_STAT_PRINT_OMIT_EXTENT")

    def test_TC_MEMKIND_malloc_stats_print_all_opts_test(self):
        """ This test checks output from malloc_stats_print()
            memkind API function when all possible options
            are passed to the function """
        self.bin_param = "all_opts"
        output, _ = self.run_test_binary()
        assert json.loads(output), \
            self.error_msg.format("MEMKIND_STAT_PRINT_JSON_FORMAT")
        assert "version" not in output, \
               self.error_msg.format("MEMKIND_STAT_PRINT_OMIT_GENERAL")
        assert "merged" not in output, \
               self.error_msg.format("MEMKIND_STAT_PRINT_OMIT_MERGED_ARENA")
        assert "destroyed" not in output, \
               self.error_msg.format(
                   "MEMKIND_STAT_PRINT_OMIT_DESTROYED_MERGED_ARENA")
        assert re.search(re.compile(r'(\d+)": {'), output) is None, \
            self.error_msg.format("MEMKIND_STAT_PRINT_OMIT_PER_ARENA")
        assert "\"bins\"" not in output, \
               self.error_msg.format(
                   "MEMKIND_STAT_PRINT_OMIT_PER_SIZE_CLASS_BINS")
        assert "lextents" not in output, \
               self.error_msg.format(
                   "MEMKIND_STAT_PRINT_OMIT_PER_SIZE_CLASS_LARGE")
        assert "mutex" not in output, \
               self.error_msg.format("MEMKIND_STAT_PRINT_OMIT_MUTEX")
        assert "\"extents\"" not in output, \
               self.error_msg.format("MEMKIND_STAT_PRINT_OMIT_EXTENT")

    def test_TC_MEMKIND_malloc_stats_print_negative_test(self):
        """ This test checks if there is no output from malloc_stats_print()
            memkind API function when wrong arguments
            are being passed to the function """
        self.bin_param = "negative_test"
        output, _ = self.run_test_binary()
        assert len(output) == 0, \
            "Error: There should be no output from"\
            " the '{self.bin_path}' binary."

    def test_TC_MEMKIND_malloc_stats_print_opts_negative_test(self):
        """ This test checks if there is a failure in parsing opts
            in malloc_stats_print() memkind API function
            when wrong options string is passed to the function """
        self.bin_param = "opts_negative_test"
        _, retcode = self.run_test_binary()
        assert retcode == 0, \
            f"Error: '{self.bin_path}' binary should return 0" \
            " indicating that parsing opts string failed."
