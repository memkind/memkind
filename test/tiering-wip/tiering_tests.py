# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import os
import pytest
from python_framework import CMD_helper


class Test_tiering(object):
    # NOTE: this script should be called from the root of memkind repository
    utils_lib_path = "tiering/.libs/libmemtier.so"
    ld_preload_env = "LD_PRELOAD=" + utils_lib_path
    bin_path = "ls"
    cmd_helper = CMD_helper()

    def test_utils_init(self):
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=1"

        # run command without LD_PRELOAD
        default_output, ls_retcode = self.cmd_helper.execute_cmd(self.bin_path)

        # run command with LD_PRELOAD
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output, retcode = self.cmd_helper.execute_cmd(command)

        assert retcode == 0, f"Test failed with error: \nExecution of: \'{command}\' returns: {retcode} \noutput: {output}"

        assert output.split("\n")[0] == \
            "MEMKIND_MEM_TIERING_LOG_INFO: Memkind mem tiering utils lib loaded!", "Bad init message"

        # check if rest of output from command is unchanged
        rest_output = "\n".join(output.split("\n")[1:])
        assert rest_output == default_output, "Bad command output"

    def test_utils_log_level_default(self):
        # run command without LD_PRELOAD
        default_output = self.cmd_helper.execute_cmd(self.bin_path)

        # run command with LD_PRELOAD
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=0"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output_level_0 = self.cmd_helper.execute_cmd(command)

        assert output_level_0 == default_output, "Bad command output"

    def test_utils_log_level_not_set(self):
        # run command without LD_PRELOAD
        default_output = self.cmd_helper.execute_cmd(self.bin_path)

        # run command with LD_PRELOAD
        command = " ".join([self.ld_preload_env, self.bin_path])
        output = self.cmd_helper.execute_cmd(command)

        assert output == default_output, "Bad command output"

    def test_utils_log_level_debug(self):
        # run command without LD_PRELOAD
        default_output, retcode = self.cmd_helper.execute_cmd(self.bin_path)

        # run command with LD_PRELOAD
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=2"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output, retcode = self.cmd_helper.execute_cmd(command)

        assert output.split("\n")[0] == \
            "MEMKIND_MEM_TIERING_LOG_DEBUG: Setting log level to: 2", "Bad init message"

        assert output.split("\n")[1] == \
            "MEMKIND_MEM_TIERING_LOG_INFO: Memkind mem tiering utils lib loaded!", "Bad init message"

        # check if rest of output from LS is unchanged
        rest_output = "\n".join(output.split("\n")[2:])
        assert rest_output == default_output, "Bad ls output"

    def test_utils_log_level_negative(self):
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=-1"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output_level_neg, retcode = self.cmd_helper.execute_cmd(command)

        assert output_level_neg.split("\n")[0] == \
            "MEMKIND_MEM_TIERING_LOG_ERROR: Wrong value of MEMKIND_MEM_TIERING_LOG_LEVEL=-1", "Bad init message"

    def test_utils_log_level_too_high(self):
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=4"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output_level_neg, retcode = self.cmd_helper.execute_cmd(command)

        assert output_level_neg.split("\n")[0] == \
            "MEMKIND_MEM_TIERING_LOG_ERROR: Wrong value of MEMKIND_MEM_TIERING_LOG_LEVEL=4", "Bad init message"

    # TODO: extend tests for log level using output from malloc wrapper
