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

class Test_hbw_detection(object):

    fail_msg = "Test failed with:\n {0}"

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

    def get_nodemask_default(self):
        """ This function executes memkind function 'get_mbind_nodemask' and returns its output """
        hbw_nodemask_default = None
        command = self.get_command_path("hbw_nodemask")
        output, retcode = self.execute_cmd(command, sudo=False)
        if retcode == 0:
            hbw_nodemask_default = output
            print "Nodemask detected in test_hbw_detection_default: {0}".format(hbw_nodemask_default)
        assert retcode == 0, self.fail_msg.format("Error: hbw_nodemask returned {0}".format(retcode))
        return hbw_nodemask_default

    def get_nodemask_env_variable(self):
        """ This function overrides environment variable MEMKIND_HBW_NODES with values returned from 'memkind-hbw-nodes',
        executes memkind function 'get_mbind_nodemask' and returns its output """
        hbw_nodemask_env_variable = None
        command = "MEMKIND_HBW_NODES=`memkind-hbw-nodes` " + self.get_command_path("hbw_nodemask")
        output, retcode = self.execute_cmd(command, sudo=False)
        if retcode == 0:
            hbw_nodemask_env_variable = output
            print "Nodemask detected in test_hbw_detection_env_variable: {0}".format(hbw_nodemask_env_variable)
        assert retcode == 0, self.fail_msg.format("Error: hbw_nodemask returned {0}".format(retcode))
        return hbw_nodemask_env_variable

    def get_nodemask_heuristic(self):
        """ This function renames 'node-bandwidth' file, executes memkind function 'get_mbind_nodemask'
        and returns its output """
        hbw_nodemask_heuristic = None
        command = "mv /var/run/memkind/node-bandwidth /var/run/memkind/node-bandwidth_disabled"
        self.execute_cmd(command, sudo=True)
        command = self.get_command_path("hbw_nodemask")
        output, retcode = self.execute_cmd(command, sudo=False)
        """ Before processing output and retcode node-bandwidth should be recovered """
        command = "mv /var/run/memkind/node-bandwidth_disabled /var/run/memkind/node-bandwidth"
        self.execute_cmd(command, sudo=True)
        if retcode == 0:
            hbw_nodemask_heuristic = output
            print "Nodemask detected in test_hbw_detection_heuristic: {0}".format(hbw_nodemask_heuristic)
        assert retcode == 0, self.fail_msg.format("Error: hbw_nodemask returned {0}".format(retcode))
        return hbw_nodemask_heuristic

    def test_hbw_detection_default(self):
        """ This test checks whether hbw_nodemask_default is not None """
        assert self.get_nodemask_default() is not None, self.fail_msg.format("Error: hbw_nodemask_default is None")

    def test_hbw_detection_env_variable(self):
        """ This test checks whether hbw_nodemask_env_variable is not None """
        assert self.get_nodemask_env_variable() is not None, self.fail_msg.format("Error: hbw_nodemask_env_variable is None")

    def test_hbw_detection_heuristic(self):
        """ This test checks whether hbw_nodemask_heuristic is not None """
        assert self.get_nodemask_heuristic() is not None, self.fail_msg.format("Error: hbw_nodemask_heuristic is None")

    def test_hbw_detection_compare_nodemask_default_and_env_variable(self):
        """ This test checks whether hbw_nodemask_default and hbw_nodemask_env_variable has the same value """
        hbw_nodemask_default = self.get_nodemask_default()
        hbw_nodemask_env_variable = self.get_nodemask_env_variable()
        assert hbw_nodemask_default == hbw_nodemask_env_variable, self.fail_msg.format("Error: Nodemask hbw_nodemask_default ({0}) is not the same as nodemask hbw_nodemask_env_variable ({1})".format(hbw_nodemask_default, hbw_nodemask_env_variable))

    def test_hbw_detection_compare_nodemask_default_and_heuristic(self):
        """ This test checks whether hbw_nodemask_default and hbw_nodemask_heuristic has the same value """
        hbw_nodemask_default = self.get_nodemask_default()
        hbw_nodemask_heuristic = self.get_nodemask_heuristic()
        assert hbw_nodemask_default == hbw_nodemask_heuristic, self.fail_msg.format("Error: Nodemask hbw_nodemask_default ({0}) is not the same as nodemask hbw_nodemask_heuristic ({1})".format(hbw_nodemask_default, hbw_nodemask_heuristic))

    def test_hbw_detection_compare_nodemask_env_variable_and_heuristic(self):
        """ This test checks whether hbw_nodemask_env_variable and hbw_nodemask_heuristic has the same value """
        hbw_nodemask_env_variable = self.get_nodemask_env_variable()
        hbw_nodemask_heuristic =self.get_nodemask_heuristic()
        assert hbw_nodemask_env_variable == hbw_nodemask_heuristic, self.fail_msg.format("Error: Nodemask hbw_nodemask_env_variable ({0}) is not the same as nodemask hbw_nodemask_heuristic ({1})".format(hbw_nodemask_env_variable, hbw_nodemask_heuristic))