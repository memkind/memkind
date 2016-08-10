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
import os
import tempfile
import subprocess

class Test_autohbw(object):
    binary = "autohbw_test_helper"
    fail_msg = "Test failed with:\n {0}"
    test_prefix = "AUTO_HBW_LOG=2 LD_PRELOAD=/usr/lib64/libautohbw.so.0 "
    memkind_malloc_log = "In my memkind malloc"
    memkind_calloc_log = "In my memkind calloc"
    memkind_realloc_log = "In my memkind realloc"
    memkind_posix_memalign_log = "In my memkind align"
    memkind_free_log = "In my memkind free"

    def execute_cmd(self, command, sudo=False):
        if sudo:
            command = "sudo {0}".format(command)
        """ Initialize temp file for stdout. Will be removed when closed. """
        outfile = tempfile.SpooledTemporaryFile()
        try:
            """ Invoke process """
            p = subprocess.Popen(command, stdout=outfile, stderr=subprocess.STDOUT, shell=True)
            p.communicate()
            """ Read stdout from file """
            outfile.seek(0)
            stdout = outfile.read()
            outfile.close()
        except:
            raise
        finally:
            """ Make sure the file is closed """
            outfile.close()
        retcode = p.returncode
        return stdout, retcode

    def get_command_path(self, binary):
        """ Get the path to the binary. """
        path = os.path.dirname(os.path.abspath(__file__))
        return os.path.join(path, binary)

    def test_TC_MEMKIND_autohbw_malloc_and_free(self):
        """ This test executes ./autohbw_test_helper with LD_PRELOAD that is overriding malloc() and free() to equivalent autohbw functions"""
        command = self.test_prefix + self.get_command_path(self.binary) + " malloc"
        print "Executing command: {0}".format(command)
        output, retcode = self.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format("Error: autohbw_test_helper returned {0} with output: {1}".format(retcode,output))
        assert self.memkind_malloc_log in output, self.fail_msg.format("Error: malloc was not overrided by autohbw equivalent (output: {0})").format(output)
        assert self.memkind_free_log in output, self.fail_msg.format("Error: free was not overrided by autohbw equivalent (output: {0})").format(output)

    def test_TC_MEMKIND_autohbw_calloc_and_free(self):
        """ This test executes ./autohbw_test_helper with LD_PRELOAD that is overriding calloc() and free() to equivalent autohbw functions"""
        command = self.test_prefix + self.get_command_path(self.binary) + " calloc"
        print "Executing command: {0}".format(command)
        output, retcode = self.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format("Error: autohbw_test_helper returned {0} with output: {1}".format(retcode,output))
        assert self.memkind_calloc_log in output, self.fail_msg.format("Error: calloc was not overrided by autohbw equivalent (output: {0})").format(output)
        assert self.memkind_free_log in output, self.fail_msg.format("Error: free was not overrided by autohbw equivalent (output: {0})").format(output)

    def test_TC_MEMKIND_autohbw_realloc_and_free(self):
        """ This test executes ./autohbw_test_helper with LD_PRELOAD that is overriding realloc() and free() to equivalent autohbw functions"""
        command = self.test_prefix + self.get_command_path(self.binary) + " realloc"
        print "Executing command: {0}".format(command)
        output, retcode = self.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format("Error: autohbw_test_helper returned {0} with output: {1}".format(retcode,output))
        assert self.memkind_realloc_log in output, self.fail_msg.format("Error: realloc was not overrided by autohbw equivalent (output: {0})").format(output)
        assert self.memkind_free_log in output, self.fail_msg.format("Error: free was not overrided by autohbw equivalent (output: {0})").format(output)

    def test_TC_MEMKIND_autohbw_posix_memalign_and_free(self):
        """ This test executes ./autohbw_test_helper with LD_PRELOAD that is overriding posix_memalign() and free() to equivalent autohbw functions"""
        command = self.test_prefix + self.get_command_path(self.binary) + " posix_memalign"
        print "Executing command: {0}".format(command)
        output, retcode = self.execute_cmd(command, sudo=False)
        assert retcode == 0, self.fail_msg.format("Error: autohbw_test_helper returned {0} with output: {1}".format(retcode,output))
        assert self.memkind_posix_memalign_log in output, self.fail_msg.format("Error: posix_memalign was not overrided by autohbw equivalent (output: {0})").format(output)
        assert self.memkind_free_log in output, self.fail_msg.format("Error: free was not overrided by autohbw equivalent (output: {0})").format(output)
