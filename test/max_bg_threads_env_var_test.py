# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import pytest
from python_framework import CMD_helper

class Test_max_bg_threads_env_var():

    cmd_helper = CMD_helper()
    fail_msg = "Test failed with:\n {0}"
    MAIN_THREAD = 1
    threads_limit = 0

    @property
    def min_threads(self):
        return self.MAIN_THREAD + self.threads_limit

    def run_test_binary(self):
        command = f"MEMKIND_DEBUG=1 MEMKIND_BACKGROUND_THREAD_LIMIT={self.threads_limit} {self.cmd_helper.get_command_path('../environ_max_bg_threads_test')}"
        output, retcode = self.cmd_helper.execute_cmd(command)
        assert retcode != -1, self.fail_msg.format(f"\nError: Execution of \'{command}\' returns {retcode}")
        return output, retcode

    def test_TC_MEMKIND_max_bg_threads_env_var_min(self):
        """This test checks if MEMKIND_BACKGROUND_THREAD_LIMIT environment variable limits the number of
           background threads spawned correctly."""
        self.threads_limit = 1
        output, threads_count = self.run_test_binary()
        assert threads_count == self.min_threads, \
               self.fail_msg.format(f"Error: There should be {self.min_threads} threads running. " \
                                    f"Counted threads: {threads_count}. Output: {output}")

    @pytest.mark.parametrize("threads_limit", [0, 10])
    def test_TC_MEMKIND_max_bg_threads_env_var_multiple_threads(self, threads_limit):
        """This test checks if MEMKIND_BACKGROUND_THREAD_LIMIT environment variable
           allows spawning of multiple threads."""
        output, threads_count = self.run_test_binary()
        assert threads_count > self.min_threads, \
               self.fail_msg.format(f"Error: There should be more than {self.min_threads} threads running. " \
                                    f"Counted threads: {threads_count}. Output: {output}")

    def test_TC_MEMKIND_max_bg_threads_env_var_err(self):
        """This test checks if a negative value MEMKIND_BACKGROUND_THREAD_LIMIT environment variable is handled
           correctly."""
        self.threads_limit = -1
        output, threads_count = self.run_test_binary()
        assert threads_count == 134, \
               self.fail_msg.format(f"Error: Negative value of MEMKIND_BACKGROUND_THREAD_LIMIT should not be handled. " \
                                    f"Counted threads: {threads_count}. Output: {output}")
