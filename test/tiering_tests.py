# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import re

from python_framework import CMD_helper


class Test_tiering(object):
    # NOTE: this script should be called from the root of memkind repository
    ld_preload_env = "LD_PRELOAD=tiering/.libs/libmemtier.so"
    # TODO create separate, parametrized binary that could be used for testing
    # instead of using ls here
    bin_path = "ls -l"
    cmd_helper = CMD_helper()

    re_hex_or_nil = r"((0[xX][a-fA-F0-9]+)|\(nil\))"
    re_log_debug_malloc = r"MEMKIND_MEM_TIERING_LOG_DEBUG: " + \
        r"malloc\(\d+\) = " + re_hex_or_nil
    re_log_debug_realloc = r"MEMKIND_MEM_TIERING_LOG_DEBUG: realloc\(" + \
        re_hex_or_nil + r", \d+\) = " + re_hex_or_nil
    re_log_debug_calloc = r"MEMKIND_MEM_TIERING_LOG_DEBUG: " + \
        r"calloc\(\d+, \d+\) = " + re_hex_or_nil
    re_log_debug_memalign = r"MEMKIND_MEM_TIERING_LOG_DEBUG: " + \
        r"memalign\(\d+, \d +\) = " + re_hex_or_nil
    re_log_debug_free = r"MEMKIND_MEM_TIERING_LOG_DEBUG: free\(" + \
        re_hex_or_nil + r"\)"

    def test_utils_init(self):
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=1"

        # run command without LD_PRELOAD
        default_output, default_retcode = self.cmd_helper.execute_cmd(
            self.bin_path)

        assert default_retcode == 0, "Test failed with error: \n" + \
            "Execution of: \'" + self.bin_path + "\' returns: " + \
            default_retcode + " \n" + "output: " + default_output

        # run command with LD_PRELOAD
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output, retcode = self.cmd_helper.execute_cmd(command)

        assert retcode == 0, "Test failed with error: \n" + \
            "Execution of: \'" + command + "\' returns: " + retcode + " \n" + \
            "output: " + output

        assert output.split("\n")[0] == "MEMKIND_MEM_TIERING_LOG_INFO: " + \
            "Memkind memtier lib loaded!", "Bad init message"

        # check if rest of output from command is unchanged
        rest_output = "\n".join(output.split("\n")[1:])
        assert rest_output == default_output, "Bad command output"

    def test_utils_log_level_default(self):
        # run command without LD_PRELOAD
        default_output, default_retcode = self.cmd_helper.execute_cmd(
            self.bin_path)

        assert default_retcode == 0, "Test failed with error: \n" + \
            "Execution of: \'" + self.bin_path + "\' returns: " + \
            default_retcode + " \n" + "output: " + default_output

        # run command with LD_PRELOAD
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=0"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        test_output, test_retcode = self.cmd_helper.execute_cmd(command)

        assert test_retcode == 0, "Test failed with error: \n" + \
            "Execution of: \'" + command + "\' returns: " + \
            test_retcode + " \n" + "output: " + test_output

        assert test_output == default_output, "Bad command output"

    def test_utils_log_level_not_set(self):
        # run command without LD_PRELOAD
        default_output, default_retcode = self.cmd_helper.execute_cmd(
            self.bin_path)

        assert default_retcode == 0, "Test failed with error: \n" + \
            "Execution of: \'" + self.bin_path + "\' returns: " + \
            default_retcode + " \n" + "output: " + default_output

        # run command with LD_PRELOAD
        command = " ".join([self.ld_preload_env, self.bin_path])
        test_output, test_retcode = self.cmd_helper.execute_cmd(command)

        assert test_retcode == 0, "Test failed with error: \n" + \
            "Execution of: \'" + command + "\' returns: " + \
            test_retcode + " \n" + "output: " + test_output

        assert test_output == default_output, "Bad command output"

    def test_utils_log_level_debug(self):
        # run command without LD_PRELOAD
        default_output, retcode = self.cmd_helper.execute_cmd(self.bin_path)

        # run command with LD_PRELOAD
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=2"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output, retcode = self.cmd_helper.execute_cmd(command)

        assert output.split("\n")[0] == "MEMKIND_MEM_TIERING_LOG_DEBUG: " + \
            "Setting log level to: 2", "Bad init message"

        assert output.split("\n")[1] == "MEMKIND_MEM_TIERING_LOG_INFO: " + \
            "Memkind memtier lib loaded!", "Bad init message"

        # next, extract from the output all lines starting with
        # "MEMKIND_MEM_TIERING_LOG" prefix and check if they are correct
        output = output.split("\n")[2:]
        log_output = [l for l in output
                      if l.startswith("MEMKIND_MEM_TIERING_LOG")]
        for log_line in log_output:
            is_malloc = re.match(self.re_log_debug_malloc, log_line)
            is_realloc = re.match(self.re_log_debug_realloc, log_line)
            is_calloc = re.match(self.re_log_debug_calloc, log_line)
            is_free = re.match(self.re_log_debug_free, log_line)
            assert is_malloc or is_realloc or is_calloc or is_free, \
                "Bad log message format: " + log_line

        # finally check if rest of output from command is unchanged
        output = "\n".join([l for l in output if l not in log_output])
        assert output == default_output, "Bad ls output"

    def test_utils_log_level_negative(self):
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=-1"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output_level_neg, retcode = self.cmd_helper.execute_cmd(command)

        assert output_level_neg.split("\n")[0] == \
            "MEMKIND_MEM_TIERING_LOG_ERROR: Wrong value of " + \
            "MEMKIND_MEM_TIERING_LOG_LEVEL=-1", "Bad init message"

    def test_utils_log_level_too_high(self):
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=4"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output_level_neg, retcode = self.cmd_helper.execute_cmd(command)

        assert output_level_neg.split("\n")[0] == \
            "MEMKIND_MEM_TIERING_LOG_ERROR: Wrong value of " + \
            "MEMKIND_MEM_TIERING_LOG_LEVEL=4", "Bad init message"
