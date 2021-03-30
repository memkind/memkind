# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2017 - 2021 Intel Corporation.

from distutils.spawn import find_executable
from python_framework import CMD_helper
import os


class Test_hbw_detection(object):
    os.environ["PATH"] += os.pathsep + os.path.dirname(
        os.path.dirname(__file__))
    binary_path = find_executable("memkind-hbw-nodes")
    environ_err_test = "../environ_err_hbw_malloc_test"
    expected_libnuma_warning = ("libnuma: Warning: node",
                                " argument -1 is out of range\n\n")
    fail_msg = "Test failed with:\n {0}"
    cmd_helper = CMD_helper()

    def get_hbw_nodes(self, nodemask=None):
        """ This function executes memkind function 'get_mbind_nodemask'
            and returns its output - comma-separated HBW nodes """
        command = self.binary_path
        if (nodemask):
            command = "MEMKIND_HBW_NODES={}".format(nodemask) + command
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format(
            "\nError: Execution of \'{0}\' returns {1},",
            " \noutput: {2}".format(command, retcode, output))
        print("\nExecution of {} returns output {}".format(command, output))
        return output

    def test_TC_MEMKIND_hbw_detection_compare_nodemask_default_and_env_variable(self):
        """ This test checks whether hbw_nodemask_default
            and hbw_nodemask_env_variable has the same value """
        hbw_nodemask_default = self.get_hbw_nodes()
        hbw_nodemask_env_variable = self.get_hbw_nodes(hbw_nodemask_default)
        assert hbw_nodemask_default == (
            hbw_nodemask_env_variable, self.fail_msg.format(
                "Error: Nodemask hbw_nodemask_default ({0}) ",
                "is not the same as nodemask",
                " hbw_nodemask_env_variable ({1})".format(
                    hbw_nodemask_default, hbw_nodemask_env_variable)))

    def test_TC_MEMKIND_hbw_detection_negative_hbw_malloc(self):
        """ This test sets unsupported value of MEMKIND_HBW_NODES,
            then try to perform a successful allocation from DRAM using
            hbw_malloc() thanks to default HBW_POLICY_PREFERRED policy """
        command = "MEMKIND_HBW_NODES=-1 " \
            + self.cmd_helper.get_command_path(self.environ_err_test)
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode != 0, self.fail_msg.format(
            "\nError: Execution of: \'{0}\' returns: {1} \noutput: {2}".format(
                command, retcode, output))
        assert self.expected_libnuma_warning == output, self.fail_msg.format(
            "Error: expected libnuma warning ({0}) ",
            "was not found (output: {1})").format(
                self.expected_libnuma_warning, output)
