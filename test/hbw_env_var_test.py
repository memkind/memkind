# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

from python_framework import CMD_helper
import os
import pytest
#from fstring import fstring

class Test_hbw_env_var(object):
    environ_err_threshold_test = "../environ_err_hbw_threshold_test"
    cmd_helper = CMD_helper()

    def run_test(self, threshold, kind):
        env = "MEMKIND_HBW_THRESHOLD=" + str(threshold)
        bin_path = self.cmd_helper.get_command_path(self.environ_err_threshold_test)
        command = " ".join([env, bin_path, kind])
        output, retcode = self.cmd_helper.execute_cmd(command)

        fail_msg = f"Test failed with error: \nExecution of: \'{command}\' returns: {retcode} \noutput: {output}"
        assert retcode == 0, fail_msg

    @pytest.mark.parametrize("kind", ["MEMKIND_HBW", "MEMKIND_HBW_ALL"])
    def test_TC_MEMKIND_hbw_threshld_default_value(self, kind):
        threshold = 200 * 1024
        self.run_test(threshold, kind)

    @pytest.mark.parametrize("kind", ["MEMKIND_HBW", "MEMKIND_HBW_ALL"])
    def test_TC_MEMKIND_hbw_threshld_negative_value(self, kind):
        threshold = -1
        self.run_test(threshold, kind)

    @pytest.mark.parametrize("kind", ["MEMKIND_HBW", "MEMKIND_HBW_ALL"])
    def test_TC_MEMKIND_hbw_threshold_max_value(self, kind):
        threshold = 1024 * 1024 * 1024
        self.run_test(threshold, kind)

    @pytest.mark.parametrize("kind", ["MEMKIND_HBW", "MEMKIND_HBW_ALL"])
    def test_TC_MEMKIND_hbw_threshold_low_value(self, kind):
        threshold = 20
        self.run_test(threshold, kind)
