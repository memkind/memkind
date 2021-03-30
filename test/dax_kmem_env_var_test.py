# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2019 - 2021 Intel Corporation.

from distutils.spawn import find_executable
from python_framework import CMD_helper
import os


class Test_dax_kmem_env_var(object):
    os.environ["PATH"] += (os.pathsep
                           + os.path.dirname(os.path.dirname(__file__)))
    binary_path = find_executable("memkind-auto-dax-kmem-nodes")
    environ_err_test = "../environ_err_dax_kmem_malloc_test"
    environ_err_positive_test = "../environ_err_dax_kmem_malloc_positive_test"
    expected_libnuma_warning = (
        "libnuma: Warning: node argument -1 is out of range\n\n")
    fail_msg = "Test failed with:\n {0}"
    cmd_helper = CMD_helper()

    def get_dax_kmem_nodes(self, nodemask=None):
        """ This function executes memkind function 'get_mbind_nodemask'
            and returns its output - comma-separated DAX_KMEM nodes """
        command = self.binary_path
        if (nodemask):
            command = "MEMKIND_DAX_KMEM_NODES={}".format(nodemask) + command
        output, retcode = self.cmd_helper.execute_cmd(command)
        assert retcode == 0, self.fail_msg.format(
            "\nError: Execution of \'{0}\' returns {1}, \noutput: {2}"
            .format(command, retcode, output))
        print("\nExecution of {} returns output {}".format(command, output))
        return output

    def test_TC_MEMKIND_dax_kmem_env_var_compare_nodemask_default_and_env_variable(self):
        """ This test checks whether dax_kmem_nodemask_default and
            dax_kmem_nodemask_env_variable have the same value """
        dax_kmem_nodemask_default = self.get_dax_kmem_nodes()
        dax_kmem_nodemask_env_variable = self.get_dax_kmem_nodes(
            dax_kmem_nodemask_default)
        assert dax_kmem_nodemask_default == (
            dax_kmem_nodemask_env_variable,
            self.fail_msg.format(
                "Error: Nodemask dax_kmem_nodemask_default ({0}) ",
                "is not the same as nodemask",
                " dax_kmem_nodemask_env_variable ({1})".format(
                    dax_kmem_nodemask_default,
                    dax_kmem_nodemask_env_variable)))

    def test_TC_MEMKIND_dax_kmem_env_var_negative_memkind_malloc(self):
        """ This test sets unsupported value of MEMKIND_DAX_KMEM_NODES,
            then tries to perform a successful allocation
            from DRAM using memkind_malloc() """
        command = ("MEMKIND_DAX_KMEM_NODES=-1 "
                   + self.cmd_helper.get_command_path(self.environ_err_test))
        output, retcode = self.cmd_helper.execute_cmd(command)
        assert retcode != 0, self.fail_msg.format(
            "\nError: Execution of: \'{0}\' returns: {1} \noutput: {2}"
            .format(command, retcode, output))
        assert self.expected_libnuma_warning == output, self.fail_msg.format(
            "Error: expected libnuma warning ({0}) ",
            "was not found (output: {1})").format(
                self.expected_libnuma_warning, output)

    def test_TC_MEMKIND_dax_kmem_env_var_proper_memkind_malloc(self):
        """ This test checks if allocation is performed
            on persistent memory correctly """
        dax_kmem_nodemask_default = self.get_dax_kmem_nodes().strip()
        command = "MEMKIND_DAX_KMEM_NODES={} ".format(
            dax_kmem_nodemask_default) + self.cmd_helper.get_command_path(
                self.environ_err_positive_test)
        output, retcode = self.cmd_helper.execute_cmd(command)
        assert retcode == 0, self.fail_msg.format(
            "\nError: Execution of: \'{0}\' returns: {1} \noutput: {2}"
            .format(command, retcode, output))
