# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2016 - 2021 Intel Corporation.

import os
from python_framework.cmd_helper import CMD_helper


def _get_libautohbw_path():
    for p in ["/usr/lib64/libautohbw.so.0", "/usr/lib/libautohbw.so.0"]:
        if os.path.isfile(p):
            return p
    raise Exception("Cannot find library path in OS")


class Test_autohbw(object):
    binary = "../autohbw_test_helper"
    fail_msg = "Test failed with:\n {0}"
    test_prefix = "AUTO_HBW_LOG=2 LD_PRELOAD=%s " % _get_libautohbw_path()
    memkind_malloc_log = "In my memkind malloc"
    memkind_calloc_log = "In my memkind calloc"
    memkind_realloc_log = "In my memkind realloc"
    memkind_posix_memalign_log = "In my memkind align"
    memkind_free_log = "In my memkind free"
    cmd_helper = CMD_helper()

    def test_TC_MEMKIND_autohbw_malloc_and_free(self):
        """ This test executes ./autohbw_test_helper with LD_PRELOAD that is
            overriding malloc() and free() to equivalent autohbw functions"""
        command = (self.test_prefix
                   + self.cmd_helper.get_command_path(self.binary) + " malloc")
        print("Executing command: {0}".format(command))
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode == (0,
                           self.fail_msg.format("\nError: autohbw_test_helper",
                                                " returned {0} \noutput: {1}"
                                                .format(retcode, output)))
        assert self.memkind_malloc_log in output, self.fail_msg.format(
            "\nError: malloc was not overridden",
            " by autohbw equivalent \noutput: {0}").format(output)
        assert self.memkind_free_log in output, self.fail_msg.format(
            "\nError: free was not overridden",
            " by autohbw equivalent \noutput: {0}").format(output)

    def test_TC_MEMKIND_autohbw_calloc_and_free(self):
        """ This test executes ./autohbw_test_helper with
            LD_PRELOAD that is overriding calloc() and free()
            to equivalent autohbw functions"""
        command = (self.test_prefix
                   + self.cmd_helper.get_command_path(self.binary) + " calloc")
        print("Executing command: {0}".format(command))
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode == (0,
                           self.fail_msg.format("\nError: autohbw_test_helper",
                                                " returned {0} \noutput: {1}"
                                                .format(retcode, output)))
        assert self.memkind_calloc_log in output, self.fail_msg.format(
            "\nError: calloc was not overridden by",
            " autohbw equivalent \noutput: {0}").format(output)
        assert self.memkind_free_log in output, self.fail_msg.format(
            "Error: free was not overridden by",
            " autohbw equivalent \noutput: {0}").format(output)

    def test_TC_MEMKIND_autohbw_realloc_and_free(self):
        """ This test executes ./autohbw_test_helper with LD_PRELOAD that is
            overriding realloc() and free() to equivalent autohbw functions"""
        command = (self.test_prefix
                   + self.cmd_helper.get_command_path(self.binary)
                   + " realloc")
        print("Executing command: {0}".format(command))
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format(
            "\nError: autohbw_test_helper returned {0} \noutput: {1}"
            .format(retcode, output))
        assert self.memkind_realloc_log in output, self.fail_msg.format(
            "\nError: realloc was not overridden by",
            " autohbw equivalent \noutput: {0}").format(output)
        assert self.memkind_free_log in output, self.fail_msg.format(
            "\nError: free was not overridden by",
            " autohbw equivalent \noutput: {0}").format(output)

    def test_TC_MEMKIND_autohbw_posix_memalign_and_free(self):
        """ This test executes ./autohbw_test_helper with LD_PRELOAD
            that is overriding posix_memalign() and free()
            to equivalent autohbw functions"""
        command = (self.test_prefix
                   + self.cmd_helper.get_command_path(self.binary)
                   + " posix_memalign")
        print("Executing command: {0}".format(command))
        output, retcode = self.cmd_helper.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format(
            "\nError: autohbw_test_helper returned {0} \noutput: {1}"
            .format(retcode, output))
        assert self.memkind_posix_memalign_log in output, self.fail_msg.format(
            "\nError: posix_memalign was not overridden by",
            " autohbw equivalent \noutput: {0}").format(output)
        assert self.memkind_free_log in output, self.fail_msg.format(
            "\nError: free was not overridden by",
            " autohbw equivalent \noutput: {0}").format(output)
