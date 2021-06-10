# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2016 - 2021 Intel Corporation.

from python_framework.cmd_helper import CMD_helper


class Test_trace_mechanism(object):
    binary = "../trace_mechanism_test_helper"
    fail_msg = "Test failed with:\n {0}"
    debug_env = "MEMKIND_DEBUG=1 "
    cmd_helper = CMD_helper()

    def test_TC_MEMKIND_logging_MEMKIND_HBW(self):
        # This test executes trace_mechanism_test_helper and
        # test if MEMKIND_INFO message occurs while calling MEMKIND_HBW
        command = self.debug_env \
            + self.cmd_helper.get_command_path(self.binary) + " MEMKIND_HBW"
        print("Executing command: {0}".format(command))
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format(
            "\nError: trace_mechanism_test_helper",
            " returned {0} \noutput: {1}".format(retcode, output))
        assert "MEMKIND_INFO: Initializing kind memkind_hbw." \
            in output, self.fail_msg.format(
                "\nError: trace mechanism in memkind doesn't show",
                " MEMKIND_INFO message \noutput: {0}").format(output)

    def test_TC_MEMKIND_2MBPages_logging_MEMKIND_HUGETLB(self):
        # This test executes trace_mechanism_test_helper and test
        # if MEMKIND_INFO message occurs while calling MEMKIND_HUGETLB
        command = self.debug_env \
            + self.cmd_helper.get_command_path(self.binary) \
            + " MEMKIND_HUGETLB"
        print("Executing command: {0}".format(command))
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format(
            "\nError: trace_mechanism_test_helper",
            " returned {0} \noutput: {1}".format(retcode, output))
        assert "MEMKIND_INFO: Number of" in output, self.fail_msg.format(
            "\nError: trace mechanism in memkind doesn't show",
            " MEMKIND_INFO message \noutput: {0}").format(output)
        assert "MEMKIND_INFO: Overcommit limit for" \
            in output, self.fail_msg.format(
                "\nError: trace mechanism in memkind doesn't show",
                " MEMKIND_INFO message \n output: {0}").format(output)

    def test_TC_MEMKIND_logging_negative_MEMKIND_DEBUG_env(self):
        # This test executes trace_mechanism_test_helper and test
        # if setting MEMKIND_DEBUG to wrong value
        # causes MEMKIND_WARNING message
        command = "MEMKIND_DEBUG=-1 " \
            + self.cmd_helper.get_command_path(self.binary) + " MEMKIND_HBW"
        print("Executing command: {0}".format(command))
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format(
            "\nError: trace_mechanism_test_helper",
            " returned {0} \noutput: {1}".format(retcode, output))
        assert "MEMKIND_WARNING: debug option" in output, self.fail_msg.format(
            "\nError: setting wrong MEMKIND_DEBUG environment variable",
            " doesn't show MEMKIND_WARNING \noutput: {0})").format(output)
