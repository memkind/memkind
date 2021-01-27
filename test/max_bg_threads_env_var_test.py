# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import pytest
from python_framework import CMD_helper

class Test_max_bg_threads_env_var():

    cmd_helper = CMD_helper()
    threads_count_test_path = "../environ_max_bg_threads_test"
    fail_msg = "Test failed with:\n {0}"
    base_threads_count = 1
    threads_limit = 0

    @property
    def min_threads(self):
        return self.base_threads_count + self.threads_limit

    @property
    def command(self):
        return f"MEMKIND_BACKGROUND_THREAD_LIMIT={self.threads_limit} " \
               f"{self.cmd_helper.get_command_path(self.threads_count_test_path)}"

    def get_threads_count(self):
        output, retcode = self.cmd_helper.execute_cmd(self.command)
        assert retcode != -1, self.fail_msg.format(f"\nError: Execution of \'{command}\' returns {retcode}")
        return output, retcode

    def test_TC_MEMKIND_max_bg_threads_env_var_min(self):
        """This test checks if MEMKIND_BACKGROUND_THREAD_LIMIT environment variable limits the number of
           background threads spawned correctly."""
        self.threads_limit = 1
        output, threads_count = self.get_threads_count()
        assert threads_count == self.min_threads, \
               self.fail_msg.format(f"Error: There should be {self.min_threads} threads running. " \
                                    f"Counted threads: {threads_count}. Output: {output}")

    @pytest.mark.parametrize("threads_limit", [-1, 0, 10])
    def test_TC_MEMKIND_max_bg_threads_env_var_multiple_threads(self, threads_limit):
        """This test checks if MEMKIND_BACKGROUND_THREAD_LIMIT environment variable
           allows spawning of multiple threads."""
        output, threads_count = self.get_threads_count()
        assert threads_count > self.min_threads, \
               self.fail_msg.format(f"Error: There should be more than {self.min_threads} threads running. " \
                                    f"Counted threads: {threads_count}. Output: {output}")
