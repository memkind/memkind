# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import os
import pytest
import re
import subprocess

from python_framework import cmd_helper

MEMKIND_PMEM_MIN_SIZE = 1024 * 1024 * 16


class Helper(object):
    # NOTE: this script should be called from the root of memkind repository
    ld_preload_env = "LD_PRELOAD=tiering/.libs/libmemtier.so"
    # TODO create separate, parametrized binary that could be used for testing
    # instead of using ls here
    bin_path = "ls"
    cmd = cmd_helper.CMD_helper()

    log_prefix = "MEMKIND_MEM_TIERING_"
    log_debug_prefix = log_prefix + "LOG_DEBUG: "
    log_error_prefix = log_prefix + "LOG_ERROR: "
    log_info_prefix = log_prefix + "LOG_INFO: "

    kind_name_dict = {
        'DRAM': 'memkind_default',
        'FS_DAX': 'FS-DAX',
        'DAX_KMEM': 'memkind_dax_kmem'}

    mem_tiers_env_var = "MEMKIND_MEM_TIERS"
    mem_thresholds_env_var = "MEMKIND_MEM_THRESHOLDS"

    # POLICY_STATIC_THRESHOLD is a policy used in tests that have to set a
    # valid policy but don't test anything related to allocation policies
    default_policy = "POLICY:STATIC_THRESHOLD"

    @staticmethod
    def bytes_from_str(bytes_str):
        multiplier = 1
        if bytes_str[-1] == 'K':
            multiplier = 1 << 10
        elif bytes_str[-1] == 'M':
            multiplier = 1 << 20
        elif bytes_str[-1] == 'G':
            multiplier = 1 << 30

        return str(multiplier * int(bytes_str[:-1]))

    def check_fs_dax_support(self):
        fs_dax_path = os.environ.get('PMEM_PATH', '/tmp').rstrip("/")
        with open('/proc/mounts', 'r') as f:
            for line in f.readlines():
                if 'dax' in line and fs_dax_path in line:
                    return True
        return False

    def check_kmem_dax_support(self):
        try:
            subprocess.check_call("./memkind-auto-dax-kmem-nodes")
        except subprocess.CalledProcessError:
            return False
        else:
            return True

    def get_ld_preload_cmd_output(self, tiers_config, thresholds_config=None,
                                  log_level=None, negative_test=False):
        log_level_env = self.log_prefix + "LOG_LEVEL=" + log_level \
            if log_level else ""
        tiers_env = self.mem_tiers_env_var + '="' + tiers_config + '"'
        thresholds_env = self.mem_thresholds_env_var + '="' + \
            thresholds_config + '"' if thresholds_config else ""
        command = " ".join([self.ld_preload_env, log_level_env,
                            tiers_env, thresholds_env, self.bin_path])
        output, retcode = self.cmd.execute_cmd(command)
        fail_msg = "Execution of: '" + command + \
            "' returns: " + str(retcode) + "\noutput: " + output

        if negative_test:
            assert retcode != 0, fail_msg
        else:
            assert retcode == 0, fail_msg
        return output.splitlines()

    def get_default_cmd_output(self):
        output, retcode = self.cmd.execute_cmd(self.bin_path)

        assert retcode == 0, \
            "Execution of: '" + self.bin_path + \
            "' returns: " + str(retcode) + "\noutput: " + output
        return output.splitlines()


class Test_tiering_log(Helper):

    def test_utils_init(self):
        default_output = self.get_default_cmd_output()

        output = self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;" + self.default_policy,
            log_level="1")

        assert output[0] == self.log_info_prefix + \
            "Memkind memtier lib loaded!", "Bad init message"

        # check if rest of output from command is unchanged
        rest_output = output[1:]
        assert rest_output == default_output, "Bad command output"

    def test_utils_log_level_default(self):
        default_output = self.get_default_cmd_output()

        output = self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;" + self.default_policy, log_level="0")

        assert output == default_output, "Bad command output"

    def test_utils_log_level_not_set(self):
        default_output = self.get_default_cmd_output()

        output = self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;" + self.default_policy)

        assert output == default_output, "Bad command output"

    def test_utils_log_level_debug(self):
        re_hex_or_nil = r"((0[xX][a-fA-F0-9]+)|\(nil\))"
        re_log_debug_valid_messages = [
            self.log_debug_prefix
            + r"malloc\(\d+\) = " + re_hex_or_nil + r"$",
            self.log_debug_prefix
            + r"realloc\(" + re_hex_or_nil + r", \d+\) = "
            + re_hex_or_nil + r"$",
            self.log_debug_prefix
            + r"calloc\(\d+, \d+\) = " + re_hex_or_nil + r"$",
            self.log_debug_prefix
            + r"memalign\(\d+, \d +\) = " + re_hex_or_nil + r"$",
            self.log_debug_prefix + r"free\(" + re_hex_or_nil + r"\)$",
            self.log_debug_prefix + r"kind_name: \w+$",
            self.log_debug_prefix + r"pmem_path: .*$",  # TODO add path re
            self.log_debug_prefix + r"pmem_size: \d+$",
            self.log_debug_prefix + r"ratio_value: \d+$",
            self.log_debug_prefix + r"policy: \w+$",
        ]

        default_output = self.get_default_cmd_output()

        output = self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;" + self.default_policy,
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
        log_info_prefix
        + r"malloc\(\d+\) = " + re_hex_or_nil + r" from kind: ",
        log_info_prefix
        + r"realloc\(" + re_hex_or_nil + r", \d+\) = "
        + re_hex_or_nil + r" from kind: ",
        log_info_prefix
        + r"calloc\(\d+, \d+\) = " + re_hex_or_nil + r" from kind: ",
        log_info_prefix
        + r"memalign\(\d+, \d +\) = " + re_hex_or_nil + r" from kind: ",
        log_info_prefix + r"free\(" + re_hex_or_nil + r"\) from kind: ",
    ]

    def test_DRAM_only(self):
        memkind_debug_env = "MEMKIND_DEBUG=1"
        tiering_cfg_env = "KIND:DRAM,RATIO:1;" + \
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

    def test_DRAM_ratio_first(self):
        self.get_ld_preload_cmd_output(
            "RATIO:1,KIND:DRAM;" + self.default_policy,
            log_level="2")

    @pytest.mark.parametrize("param_name",
                             ["fafes", "fdas3rfs43", "%#3^"])
    def test_bad_param_name(self, param_name):
        self.get_ld_preload_cmd_output(
            "RATIO:1," + param_name + ":DRAM;" + self.default_policy,
            negative_test=True)

    @pytest.mark.parametrize("param_name",
                             ["PMEM_SIZE_LIMIT:1G", "PATH:/tmp/"])
    def test_unsupported_param(self, param_name):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM," + param_name + ",RATIO:1;" + self.default_policy,
            negative_test=True)

    @pytest.mark.parametrize("fsdax_cfg",
                             ["KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:10G,"
                              "RATIO:1",
                              "PATH:/tmp/,KIND:FS_DAX,PMEM_SIZE_LIMIT:10G,"
                              "RATIO:1",
                              "KIND:FS_DAX,PMEM_SIZE_LIMIT:10G,PATH:/tmp/,"
                              "RATIO:1",
                              "RATIO:1,PMEM_SIZE_LIMIT:10G,KIND:FS_DAX,"
                              "PATH:/tmp/"])
    def test_FS_DAX_various_param_order(self, fsdax_cfg):
        self.get_ld_preload_cmd_output(
            fsdax_cfg + ";" + self.default_policy,
            log_level="2")

    def test_no_config(self):
        self.get_ld_preload_cmd_output('', negative_test=True)

    @pytest.mark.parametrize("order", [0, 1])
    def test_config_string_order(self, order):
        tiers_str = "KIND:DRAM,RATIO:1;" + \
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,RATIO:4;" + \
            "POLICY:DYNAMIC_THRESHOLD"
        threshold_str = "VAL:1K,MIN:64,MAX:2K"

        log_level_env = self.log_prefix + "LOG_LEVEL=2"
        tiers_env = self.mem_tiers_env_var + '="' + tiers_str + '"'
        thresholds_env = self.mem_thresholds_env_var + '="' + \
            threshold_str + '"'

        first_env = tiers_env if order else thresholds_env
        second_env = thresholds_env if order else tiers_env

        command = " ".join([self.ld_preload_env, log_level_env, first_env,
                            second_env, self.bin_path])

        output, retcode = self.cmd.execute_cmd(command)
        fail_msg = "Execution of: '" + command + \
            "' returns: " + str(retcode) + "\noutput: " + output
        assert retcode == 0, fail_msg

    @pytest.mark.parametrize("ratio", ["1", "1000", "4294967295"])
    def test_DRAM_ratio(self, ratio):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:" + ratio + ";" + self.default_policy,
            log_level="2")

    @pytest.mark.parametrize("policy", ["POLICY:STATIC_THRESHOLD"])
    def test_DRAM_policy(self, policy):
        ratio = "1"
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:" + ratio + ";" + policy,
            log_level="2")

    def test_KMEM_DAX(self):
        if not self.check_kmem_dax_support():
            pytest.skip("KMEM DAX is not configured")
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;KIND:KMEM_DAX,RATIO:100;" + self.default_policy)

    @pytest.mark.parametrize("policy", ["POLICY:STATIC_THRESHOLD",
                                        "POLICY:DYNAMIC_THRESHOLD"])
    def test_KMEM_DAX_multiple(self, policy):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;KIND:KMEM_DAX,RATIO:10;KIND:KMEM_DAX,RATIO:8;"
            + policy, negative_test=True)

    @pytest.mark.parametrize("pmem_size", ["0", str(MEMKIND_PMEM_MIN_SIZE),
                                           "18446744073709551615"])
    def test_FSDAX(self, pmem_size):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:" + pmem_size + ",RATIO:1;"
            + self.default_policy, log_level="2")

    @pytest.mark.parametrize("pmem_size",
                             ["1073741824", "1048576K", "1024M", "1G"])
    def test_FSDAX_pmem_size_with_suffix(self, pmem_size):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:" + pmem_size + ",RATIO:1;"
            + self.default_policy, log_level="2")

    @pytest.mark.parametrize("pmem_size",
                             ["-1", "-4294967295", "-18446744073709551615",
                              "18446744073709551616"])
    def test_FSDAX_pmem_size_outside_limits(self, pmem_size):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:" + pmem_size + ",RATIO:1;"
            + self.default_policy, negative_test=True)

    @pytest.mark.parametrize("pmem_size",
                             ["1", str(MEMKIND_PMEM_MIN_SIZE - 1)])
    def test_FSDAX_pmem_size_too_small(self, pmem_size):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:" + pmem_size + ",RATIO:1;"
            + self.default_policy, negative_test=True)

    @pytest.mark.parametrize("pmem_size",
                             ["18446744073709551615K", "18446744073709551615M",
                              "18446744073709551615G"])
    def test_FSDAX_pmem_size_with_suffix_too_big(self, pmem_size):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT" + pmem_size + ",RATIO:1;"
            + self.default_policy, negative_test=True)

    def test_FSDAX_no_size(self):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX", negative_test=True)

    @pytest.mark.parametrize("pmem_size", ["as", "10K1", "M", "M2", "10KM"])
    def test_FSDAX_wrong_size(self, pmem_size):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:" + pmem_size + ",RATIO:1;"
            + self.default_policy, negative_test=True)

    def test_FSDAX_check_only_fs_dax(self):
        pmem_path = os.environ.get('PMEM_PATH')
        if not self.check_fs_dax_support():
            pytest.skip("Missing FS DAX mounted on" + pmem_path)
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:" + pmem_path + ",PMEM_SIZE_LIMIT:1G,RATIO:1;"
            + self.default_policy, log_level="2")

    def test_FSDAX_negative_ratio(self):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:10G,RATIO:-1;"
            + self.default_policy, negative_test=True)

    def test_FSDAX_wrong_ratio(self):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:10G,RATIO:a;"
            + self.default_policy, negative_test=True)

    def test_FSDAX_no_path(self):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PMEM_SIZE_LIMIT:10G,RATIO:1;"
            + self.default_policy, negative_test=True)

    def test_FSDAX_no_max_size(self):
        self.get_ld_preload_cmd_output(
            "KIND:FS_DAX,PATH:/tmp/,RATIO:1;"
            + self.default_policy, log_level="2")

    def test_class_not_defined(self):
        self.get_ld_preload_cmd_output("2", negative_test=True)

    def test_bad_ratio(self):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:A;" + self.default_policy, negative_test=True)

    def test_no_ratio(self):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM;" + self.default_policy, negative_test=True)

    def test_bad_class(self):
        self.get_ld_preload_cmd_output(
            "KIND:a1b2,RATIO:10;" + self.default_policy, negative_test=True)

    def test_negative_ratio(self):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:-1;" + self.default_policy, negative_test=True)

    def test_double_ratio(self):
        self.get_ld_preload_cmd_output(
            "RATIO:2,KIND:DRAM,RATIO:1;" + self.default_policy,
            negative_test=True)

    def test_multiple_kind(self):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1,KIND:FS_DAX;" + self.default_policy,
            negative_test=True)

    @pytest.mark.parametrize("policy_str",
                             ["ABC", "5252", "1AB2C3", "#@%srfs"])
    def test_negative_unsupported_policy(self, policy_str):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;POLICY:" + policy_str,
            negative_test=True)

    def test_negative_multiple_policies(self):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;" + self.default_policy + ";"
            + self.default_policy, negative_test=True)

    def test_negative_policy_not_last(self):
        self.get_ld_preload_cmd_output(
            self.default_policy + ";KIND:DRAM,RATIO:1",
            negative_test=True)

    @pytest.mark.parametrize("config_str", ["", ",", ",,,", ":", "::"])
    def test_negative_config_str_invalid(self, config_str):
        self.get_ld_preload_cmd_output(
            config_str, negative_test=True)

    @pytest.mark.parametrize("query_str", [":", ":::"])
    def test_negative_query_str_invalid(self, query_str):
        self.get_ld_preload_cmd_output(
            "KIND" + query_str + "DRAM;" + self.default_policy,
            negative_test=True)

    @pytest.mark.parametrize("tier_str",
                             ["KIND:DRAM,RATIO:1",
                              "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,"
                              "RATIO:1",
                              "KIND:DRAM,RATIO:1;KIND:FS_DAX,PATH:/tmp/,"
                              "PMEM_SIZE_LIMIT:100M,RATIO:1"])
    def test_negative_no_policy(self, tier_str):
        self.get_ld_preload_cmd_output(
            tier_str, negative_test=True)

    @pytest.mark.parametrize("policy",
                             ["POLICY:STATIC_THRESHOLD",
                              "POLICY:DYNAMIC_THRESHOLD"])
    def test_multiple_tiers(self, policy):
        assert os.access("/mnt", os.W_OK | os.X_OK), \
            "Write and execute permissions to the /mnt directory "
        "are required for this test"
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,"
            "RATIO:4;KIND:FS_DAX,PATH:/mnt,PMEM_SIZE_LIMIT:150M,RATIO:8;"
            + policy, log_level="2")

    @pytest.mark.parametrize("thresholds",
                             ["VAL:1K,MIN:64,MAX:2K",
                              "VAL:100,MIN:1,MAX:1000",
                              "MIN:1,MAX:1000,VAL:100",
                              "MIN:1,VAL:100,MAX:1000",
                              "MAX:1000,MIN:1,VAL:100"])
    def test_two_tiers_thresholds(self, thresholds):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;"
            + "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,RATIO:4;"
            + "POLICY:DYNAMIC_THRESHOLD", thresholds_config=thresholds,
            log_level="2")

    @pytest.mark.parametrize("thresholds",
                             ["::", "AAA", "A1B2C3", "AAA:BB", "AA:42",
                              "VAL:AAA", "VAL:1A2B", "VAL::", "VAL:14,4.4",
                              "VAL:111,VAL:222"])
    def test_negative_two_tiers_bad_thresholds(self, thresholds):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;"
            + "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,RATIO:4;"
            + "POLICY:DYNAMIC_THRESHOLD", thresholds_config=thresholds,
            negative_test=True)

    @pytest.mark.parametrize("thresholds",
                             ["VAL:101,MIN:1,MAX:100",
                              "VAL:50,MIN:100,MAX:10",
                              "VAL:1,MIN:10,MAX:100"])
    def test_negative_two_tiers_bad_threshold_values(self, thresholds):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;"
            + "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,RATIO:4;"
            + "POLICY:DYNAMIC_THRESHOLD", thresholds_config=thresholds,
            negative_test=True)

    def test_negative_thresholds_bad_policy(self):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;"
            + "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,RATIO:4;"
            + "POLICY:STATIC_THRESHOLD",
            thresholds_config="VAL:1K,MIN:64,MAX:2K",
            negative_test=True)

    @pytest.mark.parametrize("params",
                             ["VAL:1K,VAL:2K,MIN:64,MAX:3K",
                              "VAL:1K,MIN:64,MAX:2K,MAX:10K",
                              "VAL:1K,MIN:3K,MIN:64,MAX:2K",
                              "VAL:1K,MIN:64,MAX:2K,VAL:10K",
                              "VAL:1K,MIN:64,MAX:2K;VAL:10K,MIN:3K,MAX:20K"])
    def test_negative_too_many_threshold_params(self, params):
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;"
            + "KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,RATIO:4;"
            + "POLICY:STATIC_THRESHOLD",
            thresholds_config=params,
            negative_test=True)

    @pytest.mark.parametrize("wrong_tier",
                             ["KIND:non_existent,PATH:/mnt,"
                              "PMEM_SIZE_LIMIT:150M,RATIO:8",
                              "KIND:FS_DAX,PATH:/non_existent,"
                              "PMEM_SIZE_LIMIT:150M,RATIO:8",
                              "KIND:FS_DAX,PATH:/mnt,PMEM_SIZE_LIMIT:1M,"
                              "RATIO:8",
                              "KIND:FS_DAX,PATH:/mnt,PMEM_SIZE_LIMIT:150M,"
                              "RATIO:0",
                              "KIND:DRAM,RATIO:1"])
    def test_multiple_tiers_wrong_tier(self, wrong_tier):
        assert os.access("/mnt", os.W_OK | os.X_OK), \
            "Write and execute permissions to the /mnt directory "
        "are required for this test"
        self.get_ld_preload_cmd_output(
            "KIND:DRAM,RATIO:1;KIND:FS_DAX,PATH:/tmp/,PMEM_SIZE_LIMIT:100M,"
            + "RATIO:4;" + wrong_tier + ";" + self.default_policy,
            log_level="2",
            negative_test=True)
