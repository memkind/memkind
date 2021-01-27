# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

from python_framework import CMD_helper

class Test_max_bg_threads_env_var():

    cmd_helper = CMD_helper()
    fail_msg = "Test failed with:\n {0}"
    base_threads_count = 1

    def test_TC_MEMKIND_max_bg_threads_env_var_min(self):
        """This test checks whether MEMKIND_BACKGROUND_THREAD_LIMIT environment variable limits the number of
           background threads spawned correctly."""
        threads_count_test_path = "../environ_max_bg_threads_test"
        threads_limit = 1
        command = f"MEMKIND_BACKGROUND_THREAD_LIMIT={threads_limit} " \
                  f"{self.cmd_helper.get_command_path(threads_count_test_path)}"
        _, retcode = self.cmd_helper.execute_cmd(command)
        assert retcode != -1, self.fail_msg.format(f"\nError: Execution of \'{command}\' returns {retcode}")
        min_threads = self.base_threads_count + threads_limit
        assert retcode == min_threads, self.fail_msg.format(f"Error: There should be {min_threads} threads running. " \
                                                            f"Counted threads: {retcode}")

        threads_limit = 10
        command = f"MEMKIND_BACKGROUND_THREAD_LIMIT={threads_limit} " \
                  f"{self.cmd_helper.get_command_path(threads_count_test_path)}"
        _, retcode = self.cmd_helper.execute_cmd(command)
        assert retcode != -1, self.fail_msg.format(f"\nError: Execution of \'{command}\' returns {retcode}")
        assert retcode > min_threads, self.fail_msg.format(f"Error: There should be more than {min_threads} threads " \
                                                           f"running. Counted threads: {retcode}")
