#! /usr/bin/env python
#
#  Copyright (C) 2014, 2015 Intel Corporation.
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

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#
# I M P O R T A N T  N O T E:
#
# P L E A S E  D O  N O T  P U T  A N Y  T A B S  I N  T H I S  F I L E
#
# USE EXACTLY 4 SPACES ('    ') FOR ONE COLUMN OF INDENTATION.
# IF YOU DO NOT UNDERSTAND WHY, THEN PLEASE GOOGLE TABS AND PYTHON INDENTATION.
#
# YOU HAVE BEEN WARNED.
#
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
import os
import platform
import sys
import shutil
import getopt
import subprocess
import optparse
import time
import tempfile
import platform
import logging
import re
import xml.dom.minidom

if os.name == 'posix':
    Linux       = True
    HOST        = "/usr/share/mpss/test/memkind-dt/"
    slash       = "/"
else:
    import _winreg
    Linux       = False
    mpss_home   = os.environ.get("INTEL_MPSS_HOME")
    HOST        =  mpss_home + "test\\windows-memkind-dt\\"
    slash       = "\\"

def setup_logging(name):
    fmt = '%(asctime)s %(name)12s (%(lineno)4d): %(levelname)-8s %(message)s'
    # configure logging to file
    logging.basicConfig(level=logging.DEBUG,
        format=fmt,
        filename='%s.log' % name,
        filemode='a')
    # configure logging to console
    console = logging.StreamHandler()
    console.setLevel(logging.DEBUG)
    console.setFormatter(logging.Formatter(fmt))
    logging.getLogger().addHandler(console)
    global log
    log = logging.getLogger(name)
    return log

def run(cmd, wait_time_in_seconds=None, ignore_error=True):

    # Init temp file for std out. Will be removed when closed
    outfile = tempfile.SpooledTemporaryFile()
    timed_out = False
    try:
        # Invoke process
        p = subprocess.Popen(cmd, stdout=outfile,
                             stderr=subprocess.STDOUT, shell=True)

        # Poll for completion if wait time was set
        if wait_time_in_seconds:
            while p.poll() is None and wait_time_in_seconds > 0:
                time.sleep(0.25)
                wait_time_in_seconds -= 0.25

            # Kill process if wait time exceeded
            if wait_time_in_seconds <= 0 and p.poll() is None:
                if platform.system().startswith("Linux"):
                    cmd = "kill %d" % p.pid
                else:
                    cmd = "taskkill.exe /PID %d /T /F" % p.pid
                pk = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT, shell=True)
                pk.communicate()
                timed_out = True

        # Wait for process completion
        p.communicate()
        # Read std out from file
        outfile.seek(0)
        stdout = outfile.read()
        outfile.close()

    finally:
        # Make sure the file is closed
        outfile.close()

    retcode = p.returncode

    return stdout, retcode

def rm_class_testname(output):
    output = output.splitlines(True)[1:]
    tout = ''
    for line in output:
        if not "." in line:
            tout = tout + line
    return tout

# ========= XML handling functions ==========
def get_testsuite(doc):
    return doc.getElementsByTagName("testsuite")[0]

def get_firsttestcase(doc):
    return doc.getElementsByTagName("testcase")[0]

def testcase_in(val, nodes):
    ret = False
    for i in nodes.getElementsByTagName("testcase"):
        if (i.getAttribute("name") == val.getAttribute("name")):
            ret = True
    return ret

def get_run_testcase(node):
    for case in node.getElementsByTagName("testcase"):
        if (case.getAttribute("status") != "notrun"):
            return case
    return None

def add_attr(node,name, val):
    if node.hasAttribute(name) and not name == 'time':
        new_val = int(node.getAttribute(name)) + val
        node.setAttribute(name, "%d" % new_val)
        return new_val
    elif name == 'time':
        new_val = float(node.getAttribute(name)) + val
        node.setAttribute(name, "%f" % new_val)
        return new_val
    else:
        return -1

def add_suite_to_doc(doc, node):
    suite_name = node.getAttribute("name")
    run_test = get_run_testcase(node)

    for suite in doc.getElementsByTagName("testsuite"):
        if suite.getAttribute("name") == suite_name:
            print suite.parentNode.tagName
            add_attr(suite, "time", float(node.getAttribute("time")))
            add_attr(suite.parentNode, "failures", int(node.getAttribute("failures")))
            add_attr(suite, "failures", int(node.getAttribute("failures")))
            add_attr(suite.parentNode, "disabled", int(node.getAttribute("disabled")))
            add_attr(suite, "disabled", int(node.getAttribute("disabled")))
            add_attr(suite.parentNode, "errors", int(node.getAttribute("errors")))
            add_attr(suite, "errors", int(node.getAttribute("errors")))
            new_case = True
            for case in suite.getElementsByTagName("testcase"):
                if (case.getAttribute("name") == run_test.getAttribute("name")):
                    suite.replaceChild(run_test , case)
                    new_case = False
                    break

            if new_case:
                suite.appendChild( run_test )
            return
    doc.getElementsByTagName("testsuites")[0].appendChild(node)

def main(argv):
    return 0

log = setup_logging("memkind_ft")

if __name__ == "__main__":
    log = setup_logging("memkind_ft")
    source = os.getcwd()
    laresu = False

    parser = optparse.OptionParser()
    parser.add_option('-l', '--list        ', help='--gtest_list_tests                    ',dest='list',default=False,action='store_true')
    parser.add_option('-t', '--show-timeout', help='dummy option to displat timeout       ',dest='time',default=False,action='store_true')
    parser.add_option('-r', '--results     ', help='--gtest output xml to bins            ',dest='resu',action='store',type='string')
    parser.add_option('-o', '--log         ', help='dummy option, berta wrappers handle it',dest='logn',action='store',type='string')

    (opts, args) = parser.parse_args()

    if Linux:
        cmd = './' + sys.argv[1]
    else:
        cmd = sys.argv[1]

    if Linux:
        if opts.list:
            shutil.copy(HOST+sys.argv[1], source)
            cmd = cmd + " --gtest_list_tests"
    else:
        if opts.list:
            shutil.copy(HOST+sys.argv[1]+'.exe', source)
            cmd = cmd + " --gtest_list_tests"

    if opts.time:
        sys.exit()

    if opts.resu:
        xmlf = opts.resu
        laresu = True

    # Changing to execution directory
    source = os.getcwd()
    os.chdir(source)

    first = True
    if not opts.list:
        xmlt = os.getcwd() + slash + sys.argv[1] + ".xml"
        proc = subprocess.Popen(cmd + " --gtest_list_tests" , stdout=subprocess.PIPE, shell=True)
        for test in proc.stdout:
            laresu = False
            test = test.strip()
            if "." in test:
                test_main = test
            else: #Run test command
                test_cmd = cmd + " --gtest_filter=" +test_main+test + " --gtest_output=xml:" + xmlt
                print test_cmd
                stdout, ret = run(test_cmd,900)
                print stdout
                laresu = True
            if laresu:
                try:
                    temp_out = xml.dom.minidom.parse(xmlt)
                    if first:
                        results_out = temp_out.cloneNode(True)
                        first = False
                    else:
                        test_suite = get_testsuite (temp_out)
                        add_suite_to_doc(results_out, test_suite)
                        temp_out.unlink()
                    os.remove(xmlt)
                except IOError:
                    print '%s failed' % test

        output_file = open(xmlf, 'w')

        output_file.write(results_out.toprettyxml())
        output_file.close()
    else:
        # Run command
        stdout, ret = run(cmd,900)
        # if --list remove test class name from stdout
        print rm_class_testname(stdout)
