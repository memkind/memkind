# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2016 - 2021 Intel Corporation.

import os
import tempfile
import subprocess


class CMD_helper(object):

    def execute_cmd(self, command, sudo=False):
        if sudo:
            command = "sudo {0}".format(command)
        # Initialize temp file for stdout. Will be removed when closed.
        outfile = tempfile.SpooledTemporaryFile()
        try:
            # Invoke process
            p = subprocess.Popen(
                command, stdout=outfile, stderr=subprocess.STDOUT, shell=True)
            p.communicate()
            # Read stdout from file
            outfile.seek(0)
            stdout = outfile.read()
        except subprocess.CalledProcessError:
            raise
        finally:
            # Make sure the file is closed
            outfile.close()
        retcode = p.returncode
        return stdout.decode("utf-8"), retcode

    def get_command_path(self, binary):
        """Get the path to the binary."""
        path = os.path.dirname(os.path.abspath(__file__))
        return os.path.join(path, binary)
