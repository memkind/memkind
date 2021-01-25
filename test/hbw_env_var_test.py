# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

from distutils.spawn import find_executable
from python_framework import CMD_helper
import os

class Test_hbw_env_var(object):
    os.environ["PATH"] += os.pathsep + os.path.dirname(os.path.dirname(__file__))
    fail_msg = "Test failed with:\n {0}"
    environ_err_threshold_test = "../environ_err_hbw_threshold_test"
    cmd_helper = CMD_helper()
    retcode_fail = -1
    retcode_pass = 0

    def simple_test(self, threshold, kind, expected_retcode):
        env = "MEMKIND_HBW_THRESHOLD=" + threshold
        bin_path = self.cmd_helper.get_command_path(self.environ_err_threshold_test)
        output, retcode = self.cmd_helper.execute_cmd(" ".join([env, bin_path, kind]), sudo=False)

        assert retcode != expected_retcode, \
            self.fail_msg.format("\nError: Execution of: \'{0}\' returns: {1} \noutput: {2}".format(env + bin_path, retcode, output))

    def test_TC_MEMKIND_hbw_threshld_default_value(self):
        """ TODO This test sets HBW threshold to unsupported value, then tries to perform a successful allocation from DRAM using hbw_malloc()
        thanks to default HBW_POLICY_PREFERRED policy """

        threshold = 200 * 1024
        self.simple_test(threshold, "MEMKIND_HBW", self.retcode_pass)
        self.simple_test(threshold, "MEMKIND_HBW_ALL", self.retcode_pass)

    def test_TC_MEMKIND_hbw_threshld_negative_value(self):
        """ TODO This test sets HBW threshold to unsupported value, then tries to perform a successful allocation from DRAM using hbw_malloc()
        thanks to default HBW_POLICY_PREFERRED policy """

        threshold = -1
        self.simple_test(threshold, self.retcode_fail)

    def test_TC_MEMKIND_hbw_threshold_max_value(self):
        """ TODO This test sets HBW threshold to DRAM bandwidth, then tries to perform a successful allocation from DRAM using hbw_malloc()
        thanks to default HBW_POLICY_PREFERRED policy """

        threshold =  1024 * 1024 * 1024
        self.simple_test(threshold, "MEMKIND_HBW", self.retcode_fail)

    # TODO
    def test_TC_MEMKIND_hbw_threshold_dram_value(self):
        """ TODO This test sets HBW threshold to DRAM bandwidth, then tries to perform a successful allocation from DRAM using hbw_malloc()
        thanks to default HBW_POLICY_PREFERRED policy """

        threshold =  111000
        self.simple_test(threshold, "MEMKIND_HBW", self.retcode_fail)
        self.simple_test(threshold, "MEMKIND_HBW_ALL", self.retcode_pass)
