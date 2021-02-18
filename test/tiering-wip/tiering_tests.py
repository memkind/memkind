# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2021 Intel Corporation.

import sys
sys.path.append("..")
from python_framework import CMD_helper

import os
import pytest

class Test_tiering(object):
    utils_lib_path = "../../tiering/.libs/libutils.so"
    cmd_helper = CMD_helper()

    def test_utils_init(self):
        env = "LD_PRELOAD=" + self.utils_lib_path
        bin_path = "ls"
        command = " ".join([env, bin_path])
        output, retcode = self.cmd_helper.execute_cmd(command)

        fail_msg = f"Test failed with error: \nExecution of: \'{command}\' returns: {retcode} \noutput: {output}"

        assert retcode == 0, fail_msg
        assert output.split("\n")[0] == "tiering utils: hello world", "bad init message"
