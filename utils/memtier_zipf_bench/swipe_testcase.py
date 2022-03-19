#!/usr/bin/python3

# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2022 Intel Corporation.

from collections import namedtuple
import subprocess
import re
import time

# GENERIC CODE - to be moved to a separate file

KB = 1024
MB = 1024 * KB
GB = 1024 * MB

Config = namedtuple(
    "Config",
    [
        "allocator_types",
        "memory_used",
        "ratios",
        "element_sizes",
        "operation_types",
        "nof_types",
        "nof_accesses",
        "output_csv",
        "log_file",
    ],
)
RunResults = namedtuple(
    "RunResults",
    [
        "avg_dram_use",
        "avg_pmem_use",
        "input_command",
        "output",
        "alloc_time",
        "accesss_time",
    ],
)

ITERATIONS = "3"

MEM_MEASUREMENT_INTERVAL_S = 2


def parse_execution_times(output):
    values = re.findall(
        "Measured execution time \\[malloc \\| access\\]:"
        "\\[([0-9]*)\\|([0-9]*)\\]",
        output,
    )[0]
    values


def check_mem_usage_by_node(pid, dram_nodes, pmem_nodes):
    result = subprocess.run(
        ["numastat", "-m", f"{pid}"], stdout=subprocess.PIPE
    )
    out = result.stdout.decode()
    usage_data = re.findall("MemUsed(.*?)\n", out)[
        0
    ]  # TODO this takes into account cached/buffered memory as well ...
    usage_data = [float(data) for data in usage_data.split(" ") if data != ""][
        :-1
    ]
    dram = 0.0
    pmem = 0.0
    used_nodes = set()
    for dram_node in dram_nodes:
        if dram_node in used_nodes:
            raise ValueError("incorrect arguments, nodes used multiple times!")
        dram += float(usage_data[dram_node])
        used_nodes.add(dram_node)
    for pmem_node in pmem_nodes:
        if pmem_node in used_nodes:
            raise ValueError("incorrect arguments, nodes used multiple times!")
        pmem += float(usage_data[pmem_node])
        used_nodes.add(pmem_node)
    return (dram, pmem)


def check_mem_usage(pid):
    DRAM_NODES = [0, 1]
    PMEM_NODES = [2, 3]
    return check_mem_usage_by_node(pid, DRAM_NODES, PMEM_NODES)


def run(
    allocator_type,
    memory_used,
    element_size,
    ratio,
    operation_type,
    nof_types,
    nof_accesses,
):
    proportions = [int(val) for val in ratio.split(":")]
    if len(proportions) != 2:
        raise ValueError("invalid ratio format")
    dram_soft_limit = str(
        int(proportions[0] / (proportions[0] + proportions[1]))
    )
    nof_allocs = str(int(memory_used / element_size))
    command = [
        "utils/memtier_zipf_bench/memtier_zipf_bench",
        allocator_type,
        "--hard-limit",
        "1000000000000",
        "--soft-limit",
        f"{dram_soft_limit}",
        "--low-limit",
        "512000",
        operation_type,
        "-a",
        f"{nof_accesses}",
        "-i",
        f"{ITERATIONS}",
        "-r",
        ratio,
        "-t",
        f"{nof_types}",
        "--nof_allocs",
        f"{nof_allocs}",
        "--alloc_size",
        f"{element_size}",
    ]
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    mem_usages = []
    while process.poll() is None:
        mem_usages.append(check_mem_usage(process.pid))
        time.sleep(MEM_MEASUREMENT_INTERVAL_S)
    output, errs = process.communicate()
    output = output.decode()
    if errs is not None:
        raise ValueError("Incorrect process execution!")
    avg_dram_use = 0
    avg_pmem_use = 0
    nof_measurements = len(mem_usages)
    for dram_usage, pmem_usage in mem_usages:
        avg_dram_use += dram_usage
        avg_pmem_use += pmem_usage
    avg_dram_use /= nof_measurements
    avg_pmem_use /= nof_measurements
    alloc_time, accesss_time = parse_execution_times(output)
    ret = RunResults(
        avg_dram_use=avg_dram_use,
        avg_pmem_use=avg_pmem_use,
        input_command=" ".join(command),
        output=output,
        alloc_time=alloc_time,
        accesss_time=accesss_time,
    )
    return ret


# how it works:
# for each possible combination of parameters, run test and output results
def run_test(config: Config):
    # TODO test if single element has size bigger than cache size!
    # TODO other correctness checks???
    results = []
    with open(config.log_file, "w") as log_file:
        for nof_accesses in config.nof_accesses:
            for allocator_type in config.allocator_types:
                for nof_types in config.nof_types:
                    for operation_type in config.operation_types:
                        for memory_used in config.memory_used:
                            for element_size in config.element_sizes:
                                for ratio in config.ratios:
                                    result = run(
                                        allocator_type,
                                        memory_used,
                                        element_size,
                                        ratio,
                                        operation_type,
                                        nof_types,
                                        nof_accesses,
                                    )
                                    results.append(result)
                                    log_file.write(result.input_command)
                                    log_file.write("\n")
                                    log_file.write(result.output)
                                    log_file.write("\n\n")
    print(f"Results saved to {config.log_file}")


# ACTUAL CONFIG

# TODO increase size for the server with much bigger L3 cache size
config = Config(
    allocator_types=["-g", "-s", "-m"],
    memory_used=[8 * GB],
    ratios=["1:0", "1:4", "1:8"],
    element_sizes=[128],
    operation_types=["-D"],
    nof_types=[8],
    nof_accesses=[100],
    output_csv="swipe_testcase.csv",
    log_file="zipf_swipe_output.log",
)

run_test(config)
