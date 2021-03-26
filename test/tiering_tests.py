# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import os
import pytest
import re

from python_framework import CMD_helper

MEMKIND_PMEM_MIN_SIZE = 1024 * 1024 * 16


class Helper(object):
    # NOTE: this script should be called from the root of memkind repository
    ld_preload_env = "LD_PRELOAD=tiering/.libs/libmemtier.so"
    # TODO create separate, parametrized binary that could be used for testing
    # instead of using ls here
    bin_path = "ls"
    cmd = CMD_helper()

    log_prefix = "MEMKIND_MEM_TIERING_"
    log_debug_prefix = log_prefix + "LOG_DEBUG: "
    log_error_prefix = log_prefix + "LOG_ERROR: "
    log_info_prefix = log_prefix + "LOG_INFO: "

    kind_name_dict = {
        'DRAM': 'memkind_default',
        'FS_DAX': 'FS-DAX'}

    # POLICY_STATIC_THRESHOLD is a policy used in tests that have to set a
    # valid policy but don't test anything related to allocation policies
    default_policy = "POLICY_STATIC_THRESHOLD"

    def check_fs_dax_support(self):
        fs_dax_path = os.environ.get('PMEM_PATH', '/tmp').rstrip("/")
        with open('/proc/mounts', 'r') as f:
            for line in f.readlines():
                if 'dax' in line and fs_dax_path in line:
                    return True
        return False

    def get_ld_preload_cmd_output(self, config_env, log_level=None,
                                  validate_retcode=True):
        log_level_env = self.log_prefix + "LOG_LEVEL=" + log_level \
            if log_level else ""
        command = " ".join([self.ld_preload_env, log_level_env,
                            config_env, self.bin_path])
        output, retcode = self.cmd.execute_cmd(command)

        if validate_retcode:
            assert retcode == 0, \
                "Test failed with error: \nExecution of: '" + command + \
                "' returns: " + str(retcode) + "\noutput: " + output
        return output.splitlines()

    def get_default_cmd_output(self):
        output, retcode = self.cmd.execute_cmd(self.bin_path)

        assert retcode == 0, \
            "Test failed with error: \nExecution of: '" + self.bin_path + \
            "' returns: " + str(retcode) + "\noutput: " + output
        return output.splitlines()


class Test_tiering_log(Helper):

    def test_utils_init(self):
        default_output = self.get_default_cmd_output()

        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1," + self.default_policy,
            log_level="1")

        assert output[0] == self.log_info_prefix + \
            "Memkind memtier lib loaded!", "Bad init message"

        # check if rest of output from command is unchanged
        rest_output = output[1:]
        assert rest_output == default_output, "Bad command output"

    def test_utils_log_level_default(self):
        default_output = self.get_default_cmd_output()

        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1," + self.default_policy,
            log_level="0")

        assert output == default_output, "Bad command output"

    def test_utils_log_level_not_set(self):
        default_output = self.get_default_cmd_output()

        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1," + self.default_policy)

        assert output == default_output, "Bad command output"

    def test_utils_log_level_debug(self):
        re_hex_or_nil = r"((0[xX][a-fA-F0-9]+)|\(nil\))"
        re_log_debug_valid_messages = [
            self.log_debug_prefix +
            r"malloc\(\d+\) = " + re_hex_or_nil + r"$",
            self.log_debug_prefix + r"realloc\(" + re_hex_or_nil +
            r", \d+\) = " + re_hex_or_nil + r"$",
            self.log_debug_prefix + r"calloc\(\d+, \d+\) = " +
            re_hex_or_nil + r"$",
            self.log_debug_prefix + r"memalign\(\d+, \d +\) = " +
            re_hex_or_nil + r"$",
            self.log_debug_prefix + r"free\(" + re_hex_or_nil + r"\)$",
            self.log_debug_prefix + r"kind_name: \w+$",
            self.log_debug_prefix + r"pmem_path: .*$",  # TODO add path re
            self.log_debug_prefix + r"pmem_size: \d+$",
            self.log_debug_prefix + r"ratio_value: \d+$",
            self.log_debug_prefix + r"policy: \w+$",
        ]

        default_output = self.get_default_cmd_output()

        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1," + self.default_policy,
            log_level="2")

        assert output[0] == self.log_debug_prefix + \
            "Setting log level to: 2", "Bad init message"

        assert output[1] == self.log_prefix + "LOG_INFO: " + \
            "Memkind memtier lib loaded!", "Bad init message"

        # extract from the output all lines starting with
        # "MEMKIND_MEM_TIERING_LOG" prefix and check if they are correct
        log_output = [line for line in output[2:]
                      if line.startswith(self.log_prefix)]
        for log_line in log_output:
            log_line_valid = any(re.match(pattern, log_line) for pattern in
                                 re_log_debug_valid_messages)
            assert log_line_valid, "Bad log message format: " + log_line

        # finally check if rest of output from command is unchanged
        output = [line for line in output if line not in log_output][2:]
        assert output == default_output, "Bad ls output"

    def test_utils_log_level_negative(self):
        log_level_env = self.log_prefix + "LOG_LEVEL=-1"
        command = " ".join([self.ld_preload_env, log_level_env,
                            self.bin_path])
        output_level_neg, _ = self.cmd.execute_cmd(command)

        assert output_level_neg.splitlines()[0] == \
            self.log_error_prefix + "Wrong value of " + \
            self.log_prefix + "LOG_LEVEL=-1", "Bad init message"

    def test_utils_log_level_too_high(self):
        log_level_env = self.log_prefix + "LOG_LEVEL=4"
        command = " ".join([self.ld_preload_env, log_level_env,
                            self.bin_path])
        output_level_neg, _ = self.cmd.execute_cmd(command)

        assert output_level_neg.splitlines()[0] == \
            self.log_error_prefix + "Wrong value of " + \
            self.log_prefix + "LOG_LEVEL=4", "Bad init message"


class Test_memkind_log(Helper):

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
        tiering_cfg_env = "MEMKIND_MEM_TIERING_CONFIG=DRAM:1," + \
            self.default_policy

        command = " ".join([self.ld_preload_env, tiering_cfg_env,
                            memkind_debug_env, self.bin_path])
        self.cmd.execute_cmd(command)

        # TODO enable this check after implementation of decorators in
        # memkind_memtier.c
        """
        kind_name = "memkind_default$"
        output = [l for l in output if l.startswith(self.log_info_prefix)]

        for log_line in output:
            log_line_valid = any(re.match(pattern + kind_name, log_line)
                                 for pattern in self.re_log_valid_messages)
            assert log_line_valid, "Bad log message format: " + log_line
        """


class Test_tiering_config_env(Helper):

    def test_no_config(self):
        output = self.get_ld_preload_cmd_output('', validate_retcode=False)

        assert self.log_error_prefix + \
            "Missing MEMKIND_MEM_TIERING_CONFIG env var" in output, \
            "Wrong message"

    def test_empty_config(self):
        output = self.get_ld_preload_cmd_output(
            'MEMKIND_MEM_TIERING_CONFIG=""', validate_retcode=False)

        assert self.log_error_prefix + \
            "Error with parsing MEMKIND_MEM_TIERING_CONFIG" in output, \
            "Wrong message"

    @pytest.mark.parametrize("ratio", ["1", "1000", "4294967295"])
    def test_DRAM_ratio(self, ratio):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:" + ratio + "," +
            self.default_policy, log_level="2")

        assert self.log_debug_prefix + "kind_name: " + \
            self.kind_name_dict.get('DRAM') in output, "Wrong message"
        assert self.log_debug_prefix + "ratio_value: " + ratio in output, \
            "Wrong message"
        assert self.log_debug_prefix + "policy: " + self.default_policy \
            in output, "Wrong message"

    @pytest.mark.parametrize("policy", ["POLICY_STATIC_THRESHOLD"])
    def test_DRAM_policy(self, policy):
        ratio = "1"
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:" + ratio + "," + policy,
            log_level="2")

        assert self.log_debug_prefix + "kind_name: " + \
            self.kind_name_dict.get('DRAM') in output, "Wrong message"
        assert self.log_debug_prefix + "ratio_value: " + ratio in output, \
            "Wrong message"
        assert self.log_debug_prefix + "policy: " + policy in output, \
            "Wrong message"

    @pytest.mark.parametrize("pmem_size", ["0", str(MEMKIND_PMEM_MIN_SIZE),
                                           "18446744073709551615"])
    def test_FSDAX(self, pmem_size):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:" +
            pmem_size + ":1," + self.default_policy,
            log_level="2")

        assert self.log_debug_prefix + "kind_name: " + \
            self.kind_name_dict.get('FS_DAX') in output, "Wrong message"
        assert self.log_debug_prefix + "pmem_path: /tmp/" in output, \
            "Wrong message"
        assert self.log_debug_prefix + "pmem_size: " + pmem_size in output, \
            "Wrong message"
        assert self.log_debug_prefix + "ratio_value: 1" in output, \
            "Wrong message"
        # TODO uncomment after full implementation of FS_DAX
        # assert self.log_debug_prefix + "policy: " + self.default_policy \
        #    in output, "Wrong message"

    @pytest.mark.parametrize("pmem_size",
                             ["1073741824", "1048576K", "1024M", "1G"])
    def test_FSDAX_pmem_size_with_suffix(self, pmem_size):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:" +
            pmem_size + ":1," + self.default_policy,
            log_level="2")

        assert self.log_debug_prefix + "kind_name: " + \
            self.kind_name_dict.get('FS_DAX') in output, "Wrong message"
        assert self.log_debug_prefix + "pmem_path: /tmp/" in output, \
            "Wrong message"
        assert self.log_debug_prefix + "pmem_size: 1073741824" in output, \
            "Wrong message"
        assert self.log_debug_prefix + "ratio_value: 1" in output, \
            "Wrong message"
        # TODO uncomment after full implementation of FS_DAX
        # assert self.log_debug_prefix + "policy: " + self.default_policy \
        #    in output, "Wrong message"

    @pytest.mark.parametrize("pmem_size",
                             ["-1", "-4294967295", "-18446744073709551615",
                              "18446744073709551616"])
    def test_FSDAX_pmem_size_outside_limits(self, pmem_size):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:" + pmem_size + ":1," +
            self.default_policy,
            validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Failed to parse pmem size: " + pmem_size, "Wrong message"

    @pytest.mark.parametrize("pmem_size",
                             ["18446744073709551615K", "18446744073709551615M",
                              "18446744073709551615G"])
    def test_FSDAX_pmem_size_with_suffix_too_big(self, pmem_size):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:" + pmem_size + ":1," +
            self.default_policy,
            validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Provided pmem size is too big: 18446744073709551615", \
            "Wrong message"

    def test_FSDAX_no_size(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX", validate_retcode=False)

        assert self.log_error_prefix + \
            "Error with parsing MEMKIND_MEM_TIERING_CONFIG" in output, \
            "Wrong message"

    @pytest.mark.parametrize("pmem_size", ["as", "10K1", "M", "M2", "10KM"])
    def test_FSDAX_wrong_size(self, pmem_size):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:" + pmem_size + ":1," +
            self.default_policy,
            validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Failed to parse pmem size: " + pmem_size, "Wrong message"

    def test_FSDAX_check_only_fs_dax(self):
        pmem_path = os.environ.get('PMEM_PATH')
        if not self.check_fs_dax_support():
            pytest.skip("Missing FS DAX mounted on" + pmem_path)
        self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:" +
            pmem_path + ":1G:1," + self.default_policy, log_level="2")

    def test_FSDAX_negative_ratio(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:10G:-1," +
            self.default_policy,
            validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Unsupported ratio: -1", "Wrong message"

    def test_FSDAX_wrong_ratio(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=FS_DAX:/tmp/:10G:a," +
            self.default_policy,
            validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Unsupported ratio: a", "Wrong message"

    def test_class_not_defined(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=2", validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Too low number of queries in configuration string: 2", \
            "Wrong message"

    def test_bad_ratio(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:A," +
            self.default_policy, validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Unsupported ratio: A", "Wrong message"

    def test_no_ratio(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM," +
            self.default_policy, validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Ratio not provided", "Wrong message"

    def test_bad_class(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=a1b2:10," +
            self.default_policy, validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Unsupported kind: a1b2", "Wrong message"

    def test_negative_ratio(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:-1," +
            self.default_policy, validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Unsupported ratio: -1", "Wrong message"

    @pytest.mark.parametrize("policy_str",
                             ["ABC", "5252", "1AB2C3", "#@%srfs"])
    def test_negative_unsupported_policy(self, policy_str):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1," + policy_str,
            validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Unknown policy: " + policy_str, "Wrong message"

    def test_negative_multiple_policies(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=DRAM:1," + self.default_policy +
            "," + self.default_policy, validate_retcode=False)

        # NOTE: only last query is treated as a policy
        assert output[0] == self.log_error_prefix + \
            "Unsupported kind: " + self.default_policy, "Wrong message"

    def test_negative_policy_not_last(self):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=" + self.default_policy + ",DRAM:1",
            validate_retcode=False)

        # NOTE: only last query is treated as a policy
        assert output[0] == self.log_error_prefix + \
            "Unsupported kind: " + self.default_policy, "Wrong message"

    @pytest.mark.parametrize("config_str", ["", ",", ",,,"])
    def test_negative_config_str_invalid(self, config_str):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=" + config_str, validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "No valid query found in: " + config_str, "Wrong message"

    @pytest.mark.parametrize("query_str", [":", ":::"])
    def test_negative_query_str_invalid(self, query_str):
        output = self.get_ld_preload_cmd_output(
            "MEMKIND_MEM_TIERING_CONFIG=" + query_str + "," +
            self.default_policy, validate_retcode=False)

        assert output[0] == self.log_error_prefix + \
            "Kind name string not found in: " + query_str, "Wrong message"
