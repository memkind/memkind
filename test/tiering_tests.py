# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import pytest
import re

from python_framework import CMD_helper


class Test_tiering_log(object):
    # NOTE: this script should be called from the root of memkind repository
    ld_preload_env = "LD_PRELOAD=tiering/.libs/libmemtier.so"
    # TODO create separate, parametrized binary that could be used for testing
    # instead of using ls here
    bin_path = "ls -l"
    cmd_helper = CMD_helper()

    log_prefix = "MEMKIND_MEM_TIERING_"
    log_debug_prefix = log_prefix + "LOG_DEBUG: "
    log_error_prefix = log_prefix + "LOG_ERROR: "
    log_info_prefix = log_prefix + "LOG_INFO: "

    re_hex_or_nil = r"((0[xX][a-fA-F0-9]+)|\(nil\))"
    re_log_debug_valid_messages = [
        log_debug_prefix + r"malloc\(\d+\) = " + re_hex_or_nil + r"$",
        log_debug_prefix +
        r"realloc\(" + re_hex_or_nil + r", \d+\) = " + re_hex_or_nil + r"$",
        log_debug_prefix + r"calloc\(\d+, \d+\) = " + re_hex_or_nil + r"$",
        log_debug_prefix + r"memalign\(\d+, \d +\) = " + re_hex_or_nil + r"$",
        log_debug_prefix + r"free\(" + re_hex_or_nil + r"\)$",
        log_debug_prefix + r"kind_name: \w+$",
        log_debug_prefix + r"pmem_path: .*$",  # TODO add path re
        log_debug_prefix + r"pmem_size: (\(null\))|\d+$",
        log_debug_prefix + r"ratio_value: \d+$",
    ]

    def get_ld_preload_cmd_output(self, config_env, log_level=None):
        log_level_env = self.log_prefix + "LOG_LEVEL=" + log_level \
            if log_level else ""
        command = " ".join([self.ld_preload_env, log_level_env,
                            config_env, self.bin_path])
        output, retcode = self.cmd_helper.execute_cmd(command)

        assert retcode == 0, \
            "Test failed with error: \nExecution of: '" + command + \
            "' returns: " + str(retcode) + "\noutput: " + output
        return output

    def get_default_cmd_output(self):
        output, retcode = self.cmd_helper.execute_cmd(self.bin_path)

        assert retcode == 0, \
            "Test failed with error: \nExecution of: '" + self.bin_path + \
            "' returns: " + str(retcode) + "\noutput: " + output
        return output

    def test_utils_init(self):
        default_output = self.get_default_cmd_output()
        default_output = default_output.split("\n")

        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1", log_level="1")
        output = output.split("\n")

        assert output[0] == self.log_info_prefix + \
            "Memkind memtier lib loaded!", "Bad init message"

        # check if rest of output from command is unchanged
        rest_output = output[1:]
        assert rest_output == default_output, "Bad command output"

    def test_utils_log_level_default(self):
        default_output = self.get_default_cmd_output()
        default_output = default_output.split("\n")

        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1", log_level="0")
        output = output.split("\n")

        assert output == default_output, "Bad command output"

    def test_utils_log_level_not_set(self):
        default_output = self.get_default_cmd_output()
        default_output = default_output.split("\n")

        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1", log_level=None)
        output = output.split("\n")

        assert output == default_output, "Bad command output"

    def test_utils_log_level_debug(self):
        default_output = self.get_default_cmd_output()
        default_output = default_output.split("\n")

        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1", log_level="2")
        output = output.split("\n")

        assert output[0] == self.log_debug_prefix + \
            "Setting log level to: 2", "Bad init message"

        assert output[1] == self.log_prefix + "LOG_INFO: " + \
            "Memkind memtier lib loaded!", "Bad init message"

        # extract from the output all lines starting with
        # "MEMKIND_MEM_TIERING_LOG" prefix and check if they are correct
        log_output = [l for l in output[2:] if l.startswith(self.log_prefix)]
        for log_line in log_output:
            log_line_valid = any(re.match(pattern, log_line) for pattern in
                                 self.re_log_debug_valid_messages)
            assert log_line_valid, "Bad log message format: " + log_line

        # finally check if rest of output from command is unchanged
        output = [l for l in output if l not in log_output][2:]
        assert output == default_output, "Bad ls output"

    def test_utils_log_level_negative(self):
        log_level_env = self.log_prefix + "LOG_LEVEL=-1"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output_level_neg, _ = self.cmd_helper.execute_cmd(command)

        assert output_level_neg.split("\n")[0] == \
            self.log_error_prefix + "Wrong value of " + \
            self.log_prefix + "LOG_LEVEL=-1", "Bad init message"

    def test_utils_log_level_too_high(self):
        log_level_env = self.log_prefix + "LOG_LEVEL=4"
        command = " ".join([self.ld_preload_env, log_level_env, self.bin_path])
        output_level_neg, _ = self.cmd_helper.execute_cmd(command)

        assert output_level_neg.split("\n")[0] == \
            self.log_error_prefix + "Wrong value of " + \
            self.log_prefix + "LOG_LEVEL=4", "Bad init message"


class Test_memkind_log(object):
    # NOTE: this script should be called from the root of memkind repository
    ld_preload_env = "LD_PRELOAD=tiering/.libs/libmemtier.so"
    # TODO create separate, parametrized binary that could be used for testing
    # instead of using ls here
    bin_path = "ls -l"
    cmd_helper = CMD_helper()

    log_info_prefix = "MEMKIND_INFO: "

    re_hex_or_nil = r"((0[xX][a-fA-F0-9]+)|\(nil\))"
    re_log_valid_messages = [
        log_info_prefix + r"malloc\(\d+\) = " + re_hex_or_nil +
        r" from kind: ",
        log_info_prefix + r"realloc\(" + re_hex_or_nil + r", \d+\) = " +
        re_hex_or_nil + r" from kind: ",
        log_info_prefix + r"calloc\(\d+, \d+\) = " + re_hex_or_nil +
        r" from kind: ",
        log_info_prefix + r"memalign\(\d+, \d +\) = " + re_hex_or_nil +
        r" from kind: ",
        log_info_prefix + r"free\(" + re_hex_or_nil + r"\) from kind: ",
    ]

    def test_DRAM_only(self):
        memkind_debug_env = "MEMKIND_DEBUG=1"
        tiering_cfg_env = "MEMKIND_MEM_TIERING_CONFIG=DRAM:1"
        kind_name = "memkind_default$"

        command = " ".join([self.ld_preload_env, tiering_cfg_env,
                            memkind_debug_env, self.bin_path])
        output, _ = self.cmd_helper.execute_cmd(command)
        output = output.splitlines()
        output = [l for l in output if l.startswith(self.log_info_prefix)]

        for log_line in output:
            log_line_valid = any(re.match(pattern + kind_name, log_line)
                                 for pattern in self.re_log_valid_messages)
            assert log_line_valid, "Bad log message format: " + log_line


class Test_tiering_config_env(object):
    ld_preload_env = "LD_PRELOAD=tiering/.libs/libmemtier.so"
    cmd_helper = CMD_helper()

    def get_cmd_output(self, config_env, log_level="0"):
        log_level_env = "MEMKIND_MEM_TIERING_LOG_LEVEL=" + log_level
        command = " ".join([self.ld_preload_env, log_level_env, config_env,
                            "ls"])
        output, retcode = self.cmd_helper.execute_cmd(command)

        assert retcode == 0, \
            "Test failed with error: \nExecution of: '" + command + \
            "' returns: " + str(retcode) + "\noutput: " + output
        return output

    def test_DRAM_only(self):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1", log_level="2")

        assert "MEMKIND_MEM_TIERING_LOG_DEBUG: kind_name: DRAM" in \
            output.splitlines(), "Wrong message"
        assert "MEMKIND_MEM_TIERING_LOG_DEBUG: ratio_value: 1" in \
            output.splitlines(), "Wrong message"

    def test_FSDAX_only(self):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:10G:1", log_level="2")

        assert "MEMKIND_MEM_TIERING_LOG_DEBUG: kind_name: FS_DAX" in \
            output.splitlines(), "Wrong message"
        assert "MEMKIND_MEM_TIERING_LOG_DEBUG: pmem_path: /tmp/" in \
            output.splitlines(), "Wrong message"
        assert "MEMKIND_MEM_TIERING_LOG_DEBUG: pmem_size: 10G" in \
            output.splitlines(), "Wrong message"
        assert "MEMKIND_MEM_TIERING_LOG_DEBUG: ratio_value: 1" in \
            output.splitlines(), "Wrong message"

    def test_FSDAX_negative_size(self):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:-1:1")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported pmem_size format: -1", "Wrong message"

    def test_FSDAX_negative_size_min(self):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:-9223372036854775808:1")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported pmem_size format: -9223372036854775808", \
            "Wrong message"

    def test_FSDAX_wrong_size(self):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:as:1")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported pmem_size format: as", "Wrong message"

    def test_FSDAX_negative_ratio(self):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:10G:-1")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported ratio: -1", "Wrong message"

    def test_FSDAX_wrong_ratio(self):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:10G:a")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported ratio: a", "Wrong message"

    def test_class_not_defined(self):
        output = self.get_cmd_output("MEMKIND_MEM_TIERING_CONFIG=2")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported kind: 2", "Wrong message"

    def test_bad_ratio(self):
        output = self.get_cmd_output("MEMKIND_MEM_TIERING_CONFIG=DRAM:A")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported ratio: A", "Wrong message"

    def test_no_ratio(self):
        output = self.get_cmd_output("MEMKIND_MEM_TIERING_CONFIG=DRAM")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Ratio not provided", "Wrong message"

    def test_bad_class(self):
        output = self.get_cmd_output("MEMKIND_MEM_TIERING_CONFIG=a1b2:10")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported kind: a1b2", "Wrong message"

    def test_negative_ratio(self):
        output = self.get_cmd_output("MEMKIND_MEM_TIERING_CONFIG=DRAM:-1")

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Unsupported ratio: -1", "Wrong message"

    @pytest.mark.parametrize("config_str", ["", ",", ",,,"])
    def test_negative_config_str_invalid(self, config_str):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=" + config_str)

        assert output.splitlines()[0] == \
            "MEMKIND_MEM_TIERING_LOG_ERROR: No valid query found in: " + \
            config_str, "Wrong message"

    @pytest.mark.parametrize("query_str", [":", ":::"])
    def test_negative_query_str_invalid(self, query_str):
        output = self.get_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=" + query_str)

        assert output.splitlines()[0] == "MEMKIND_MEM_TIERING_LOG_ERROR: " + \
            "Kind name string not found in: " + query_str, "Wrong message"
