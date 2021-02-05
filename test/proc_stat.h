// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2017 - 2021 Intel Corporation. */

#pragma once
#include <cstring>
#include <dirent.h>
#include <fstream>

class ProcStat
{
public:
    size_t get_virtual_memory_size_bytes()
    {
        get_stat("VmSize", str_value);
        return strtol(str_value, NULL, 10) * 1024;
    }

    size_t get_physical_memory_size_bytes()
    {
        get_stat("VmRSS", str_value);
        return strtol(str_value, NULL, 10) * 1024;
    }

    size_t get_used_swap_space_size_bytes(void)
    {
        get_stat("VmSwap", str_value);
        return strtol(str_value, NULL, 10) * 1024;
    }

    unsigned get_threads_count()
    {
        DIR *proc_dir;
        char dirname[64];
        snprintf(dirname, sizeof dirname, "/proc/self/task");
        proc_dir = opendir(dirname);

        unsigned threads_count = 0;
        struct dirent *direntry;
        while ((direntry = readdir(proc_dir)) != NULL) {
            if (direntry->d_name[0] != '.')
                threads_count++;
        }
        closedir(proc_dir);
        return threads_count;
    }

private:
    /* We are avoiding to allocate local buffers,
     * since it can produce noise in memory footprint tests.
     */
    char line[1024];
    char current_entry_name[1024];
    char str_value[1024];

    // Note: this function is not thread-safe.
    void get_stat(const char *field_name, char *value)
    {
        char *pos = nullptr;
        std::ifstream file("/proc/self/status", std::ifstream::in);
        if (file.is_open()) {
            while (file.getline(line, sizeof(line))) {
                pos = strstr(line, field_name);
                if (pos) {
                    sscanf(pos, "%64[a-zA-Z_0-9()]: %s", current_entry_name,
                           value);
                    break;
                }
            }
            file.close();
        }
    }
};
