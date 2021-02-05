// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once
#include <assert.h>

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

class Numastat
{
public:
    // Returns numastat memory usage per node.
    static float get_total_memory(unsigned node)
    {
        static pid_t pid = getpid();

        std::stringstream cmd;
        cmd << "numastat " << pid;

        FILE *file;
        char buff[256];
        float result = -1.0;

        if ((file = popen(cmd.str().c_str(), "r"))) {
            while (fgets(buff, 256, file))
                ;
            pclose(file);

            // We got: "Total                     1181.90            2.00
            // 1183.90". 2.00 is our memory usage from Node 1.
            std::string last_line(buff);

            size_t dot_pos = 0;
            // Now we search in: "           2.00         1183.90".
            for (unsigned i = 0; i <= node; i++) {
                dot_pos = last_line.find(".", dot_pos + 1);
                assert(dot_pos != std::string::npos);
            }

            // We are at: " 2.00         1183.90".
            size_t number_begin = last_line.rfind(" ", dot_pos);
            assert(number_begin != std::string::npos);

            number_begin += 1;
            buff[dot_pos + 3] = '\0';

#if __cplusplus > 201100L
            result = strtod(&buff[number_begin], NULL);
#else
            result = atof(&buff[number_begin]);
#endif
        }

        assert(result > -0.01);

        return result;
    }
};
