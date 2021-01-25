# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

from distutils.spawn import find_executable
from python_framework import CMD_helper
import os
import pytest

class Test_hbw_env_var(object):
    os.environ["PATH"] += os.pathsep + os.path.dirname(os.path.dirname(__file__))
    fail_msg = "Test failed with:\n Error: Execution of: \'{0}\' returns: {1} \noutput: {2}"
    environ_err_threshold_test = "../environ_err_hbw_threshold_test"
    cmd_helper = CMD_helper()
    retcode_success = 0
    retcode_not_available = 1
    retcode_error = 2

    def run_test(self, threshold, kind, expected_retcode):
        env = "MEMKIND_HBW_THRESHOLD=" + str(threshold)
        bin_path = self.cmd_helper.get_command_path(self.environ_err_threshold_test)
        command = " ".join([env, bin_path, kind])
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)

        assert retcode == expected_retcode, \
            self.fail_msg.format(command, retcode, output)

    @pytest.mark.parametrize("kind", ["MEMKIND_HBW", "MEMKIND_HBW_ALL"])
    def test_TC_MEMKIND_hbw_threshld_default_value(self, kind):
        threshold = 200 * 1024
        self.run_test(threshold, kind, self.retcode_success)

    @pytest.mark.parametrize("kind", ["MEMKIND_HBW", "MEMKIND_HBW_ALL"])
    def test_TC_MEMKIND_hbw_threshld_negative_value(self, kind):
        threshold = -1
        self.run_test(threshold, kind, self.retcode_not_available)

    @pytest.mark.parametrize("kind", ["MEMKIND_HBW", "MEMKIND_HBW_ALL"])
    def test_TC_MEMKIND_hbw_threshold_max_value(self, kind):
        threshold =  1024 * 1024 * 1024
        self.run_test(threshold, kind, self.retcode_not_available)

    @pytest.mark.parametrize("kind", ["MEMKIND_HBW", "MEMKIND_HBW_ALL"])
    def test_TC_MEMKIND_hbw_threshold_dram_value(self, kind):
        threshold =  111000
        self.run_test(threshold, kind, self.retcode_success)
