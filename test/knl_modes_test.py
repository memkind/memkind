#
#  Copyright (C) 2016 Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice(s),
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice(s),
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import pytest

from distutils.spawn import find_executable
from python_framework import CMD_helper

class Test_KNL_modes(object):
    binary_path = "../get_knl_modes"
    cmd_helper = CMD_helper()

    def test_TC_MEMKIND_get_knl_modes(self):
        command = self.cmd_helper.get_command_path(self.binary_path)
        try:
                output = self.cmd_helper.execute_cmd(command, sudo=False)
        except:
                print "get_knl_modes failed"
                raise
        else:
                print "get_knl_modes returned = {0}".format(output)

