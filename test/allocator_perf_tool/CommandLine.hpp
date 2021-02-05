// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once

#include <map>
#include <string>
#include <vector>

class CommandLine
{
public:
    // Parse and write to val when option exist and strtol(...) > 0, otherwise
    // val is not changed. T should be an integer type.
    template <class T>
    void parse_with_strtol(const std::string &option, T &val)
    {
        if (args.count(option)) {
            T tmp = static_cast<T>(strtol(args[option].c_str(), NULL, 10));
            if (tmp > 0)
                val = tmp;
            else
                printf("Warning! Option '%s' may not be set.\n",
                       option.c_str());
        } else {
            // Do not modify val.
            printf("Warning! Option '%s' is not present.\n", option.c_str());
        }
    }

    bool is_option_present(const std::string &option) const
    {
        return args.count(option);
    }

    bool is_option_set(const std::string option, std::string val)
    {
        if (is_option_present(option))
            return (args[option] == val);
        return false;
    }

    const std::string &get_option_value(const std::string &option)
    {
        return args[option];
    }

    CommandLine(int argc, char *argv[])
    {
        for (int i = 0; i < argc; i++) {
            std::string arg(argv[i]);
            size_t found = arg.find("=");

            if (found != std::string::npos) {
                std::string option = arg.substr(0, found);
                std::string val = arg.substr(found + 1, arg.length());

                args[option] = val;
            }
        }
    }

private:
    std::map<std::string, std::string> args;
};
