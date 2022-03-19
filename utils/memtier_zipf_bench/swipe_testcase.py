#!/usr/bin/python3

# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2022 Intel Corporation.

import typing
import subprocess
import re
import time
import sys
import enum

# GENERIC CODE - to be moved to a separate file

KB = 1024
MB = 1024 * KB
GB = 1024 * MB

LIMITS_ALIGNMENT = 4 * 1024 * 1024


class AllocatorType(enum.Enum):
    static = "-s"
    glibc = "-g"
    movement = "-m"
    pool = "-p"
    fast_pool = "-f"


class OperationType(enum.Enum):
    random_incrementation = "-A"
    sequential_incrementation = "-B"
    random_copy = "-C"
    sequential_copy = "-D"


class Config(typing.NamedTuple):
    allocator_types: list[AllocatorType]
    memory_used: list[int]
    ratios: list[str]
    element_sizes: list[int]
    operation_types: list[OperationType]
    nof_types: list[int]
    nof_accesses: list[int]
    output_csv: str
    log_file: str
    result_file: str


class RunResults(typing.NamedTuple):
    avg_dram_use: float
    avg_pmem_use: float
    input_command: str
    output: str
    alloc_time: float
    accesss_time: float


ITERATIONS = "3"

MEM_MEASUREMENT_INTERVAL_S = 2


def parse_execution_times(output: str) -> (float, float):
    malloc_time, access_time = re.findall(
        "Measured execution time \\[malloc \\| access\\]: "
        "\\[([0-9]*)\\|([0-9]*)\\]",
        output,
    )[0]
    return (float(malloc_time), float(access_time))


def check_mem_usage_by_node(
    pid: int, dram_nodes: list[int], pmem_nodes: list[int]
) -> (int, int):
    # TODO this approach (numastat) takes into account cached/buffered memory,
    #  it probably shouldn't
    result = subprocess.run(
        ["numastat", "-m", f"{pid}"], stdout=subprocess.PIPE
    )
    out = result.stdout.decode()
    usage_data = re.findall("MemUsed(.*?)\n", out)[0]
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


def check_mem_usage(pid: int) -> (int, int):
    # TODO read from system - before productization
    DRAM_NODES = [0, 1]
    PMEM_NODES = [2, 3]
    return check_mem_usage_by_node(pid, DRAM_NODES, PMEM_NODES)


def calculate_dram_limits(
    proportions: (int, int), total_memory: int
) -> (int, int):
    if len(proportions) != 2:
        raise ValueError(
            'Invalid ratio format, expected: "[0-9]*:[0-9]*", e.g. "1:8"'
        )
    dram_soft_limit = int(
        (proportions[0] / (proportions[0] + proportions[1])) * total_memory
    )
    # align limit
    dram_soft_limit = (
        int(dram_soft_limit / LIMITS_ALIGNMENT) * LIMITS_ALIGNMENT
    )
    if dram_soft_limit < LIMITS_ALIGNMENT:
        dram_soft_limit = LIMITS_ALIGNMENT
    DRAM_DEFAULT_LOW = LIMITS_ALIGNMENT
    dram_low_limit = (
        DRAM_DEFAULT_LOW
        if DRAM_DEFAULT_LOW < dram_soft_limit
        else dram_soft_limit
    )
    return (dram_low_limit, dram_soft_limit)


def run(
    allocator_type: AllocatorType,
    memory_used: int,
    element_size: int,
    ratio: str,
    operation_type: OperationType,
    nof_types: int,
    nof_accesses: int,
) -> RunResults:
    proportions = [int(val) for val in ratio.split(":")]
    dram_low_limit, dram_soft_limit = calculate_dram_limits(
        proportions, memory_used * nof_types
    )
    nof_allocs = str(int(memory_used / element_size))
    command = [
        "numactl",
        "-N",
        "0",
        "-m",
        "0",
        "utils/memtier_zipf_bench/memtier_zipf_bench",
        f"{allocator_type.value}",
        "--hard-limit",
        "1000000000000",
        "--soft-limit",
        f"{dram_soft_limit}",
        "--low-limit",
        f"{dram_low_limit}",
        f"{operation_type.value}",
        "--access",
        f"{nof_accesses}",
        "--iterations",
        f"{ITERATIONS}",
        "--ratio",
        ratio,
        "--threads",
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
        print(errs, file=sys.stderr)
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
def run_test(config: Config) -> list[RunResults]:
    # TODO test if single element has size bigger than cache size!
    # TODO other correctness checks???
    results = []
    with open(config.log_file, "w") as log_file, open(
        config.result_file, "w"
    ) as result_file:
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
                                    result_summary = (
                                        f"{result.input_command}\n"
                                        f"alloc time: {result.alloc_time}\n"
                                        f"access time: {result.accesss_time}\n"
                                        f"dram usage: {result.avg_dram_use}\n"
                                        f"pmem usage: {result.avg_pmem_use}\n"
                                    )
                                    result_file.write(result_summary)
                                    print(result_summary)
    print(f"Results saved to {config.log_file}")
    return results


# ACTUAL CONFIG

# TODO increase size for the server with much bigger L3 cache size
config = Config(
    allocator_types=[
        AllocatorType.glibc,
        AllocatorType.static,
        AllocatorType.movement,
    ],
    memory_used=[256 * MB],
    ratios=["1:0", "1:4", "1:8"],
    element_sizes=[128, 256],
    operation_types=[OperationType.sequential_incrementation],
    nof_types=[8],
    nof_accesses=[40],
    output_csv="swipe_testcase.csv",
    log_file="zipf_swipe_output.log",
    result_file="results.txt",
)

run_test(config)
