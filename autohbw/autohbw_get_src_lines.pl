#!/usr/bin/perl
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2015 - 2019 Intel Corporation.

use strict;

my $usage = "Usage: get_autohbw_srclines.pl output_log_of_AutoHBW executable";

# Check for 2 arguments
#
if (@ARGV != 2) {
    print $usage, "\n";
    exit;
}

# Read the command line arguments
#
my $LogF = shift @ARGV;
my $BinaryF = shift @ARGV;

&main();

sub main {


    print("Info: Reading AutoHBW log from: $LogF\n");
    print("Info: Binary file: $BinaryF\n");

    # open the log file produced by AutoHBM and look at lines starting
    # with Log
    open LOGF, "grep Log $LogF |" or die "Can't open log file $LogF";

    my $line;

    # Read each log line
    #
    while ($line = <LOGF>) {

        # if the line contain 3 backtrace addresses, try to find the source
        # lines for them
        #
        if ($line =~ /^(Log:.*)Backtrace:.*0x([0-9a-f]+).*0x([0-9a-f]+).*0x([0-9a-f]+)/ ) {

            #  Read the pointers
            #
            my @ptrs;

            my $pre = $1;

            $ptrs[0] = $2;
            $ptrs[1] = $3;
            $ptrs[2] = $4;

            # prints the first portion of the line
            #
            print $pre, "\n";

            # for each of the pointers, lookup its source line using
            # addr2line and print the src line(s) if found
            #
            my $i=0;
            for ($i=0; $i < @ptrs; $i++) {

                my $addr = $ptrs[$i];

                open SRCL, "addr2line -e $BinaryF 0x$addr |"
                    or die "addr2line fail";


                my $srcl = <SRCL>;

                if ($srcl =~ /^\?/) {

                } else {

                    print "\t- Src: $srcl";
                }

            }

        }

    }

}
