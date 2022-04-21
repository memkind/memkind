#!/usr/bin/python3

# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2022 Intel Corporation.

import sys
import argparse
import subprocess
import os
import re
import time
from matplotlib import pyplot as plt

LOW_LIMIT = "512M"
SOFT_LIMIT = "1G"
HARD_LIMIT = "1G"
ITERATION_TEST_TIME_S = 10
MIN_KEY = 1
MAX_KEY = 5_000_000
ITERATIONS = 30
USED_RATIO = 1/8
SUBAREAS = 20


class RedisServer:
    def __init__(self, redis_path: str):
        menv = os.environ.copy()
        menv["LD_PRELOAD"] = "tiering/.libs/libmemtier.so"
        menv[
            "MEMKIND_MEM_TIERS"
        ] = "KIND:DRAM,RATIO:1;KIND:KMEM_DAX,RATIO:8;POLICY:DATA_MOVEMENT"
        menv[
            "MEMKIND_MTT_LIMITS"
        ] = f"LOW:{LOW_LIMIT},SOFT:{SOFT_LIMIT},HARD:{HARD_LIMIT}"
        command = [
            os.path.join(redis_path, "src/redis-server"),
            os.path.join(redis_path, "redis.conf"),
            "--port",
            "7000",
            "--bind",
            "127.0.0.1",
            "--save",
            "",
        ]
        self.handle = subprocess.Popen(
            command, env=menv, stdout=subprocess.PIPE
        )
        time.sleep(10)  # temporary solution...

    def shutdown(self):
        if self.handle:
            self.handle.kill()
            _output, _errs = self.handle.communicate()
            self.handle = None
            # TODO cleanup !!! remove all dumped databases, etc

    def __del__(self):
        self.shutdown()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.shutdown()
        if exc_type or exc_value or traceback:
            print(
                "Errors:\n\t"
                f"exc_type: {exc_type}\n\t"
                f"exc_value: {exc_value}\n\t"
                f"traceback: {traceback}"
            )


class MemtierBenchRunner:
    def __split_keys(
        min_key: int, max_key: int, subareas: int, used_ratio: float
    ):
        distance = max_key - min_key
        keys_used = used_ratio * distance
        single_key_span = int(keys_used / subareas)
        single_distance = int(distance / subareas)
        return [
            (begin, begin + single_key_span)
            for begin in range(
                min_key, max_key - single_key_span, single_distance
            )
        ]

    def create_command_fill(
        memtier_bench_path: str, min_key: int, max_key: int
    ) -> list[str]:
        return [
            os.path.join(memtier_bench_path, "memtier_benchmark"),
            "--ratio=1:0",
            "--data-size=1024",
            "--requests=allkeys",
            "--key-pattern=P:P",
            f"--key-minimum={min_key}",
            f"--key-maximum={max_key}",
            "--random-data",
            "--threads=5",
            "--clients=4",
            "--server=127.0.0.1",
            "--port=7000",
            "--hide-histogram",
            "--out-file",
            "populate.txt",
        ]

    def __create_command_bench(
        memtier_bench_path: str, min_key: int, max_key: int
    ) -> list[str]:
        # TODO check if arguments are ok
        # TODO adapt
        return [
            os.path.join(memtier_bench_path, "memtier_benchmark"),
            "--ratio=2:8",
            "--data-size=1024",
            f"--test-time={ITERATION_TEST_TIME_S}",
            "--key-pattern=P:P",
            f"--key-minimum={min_key}",
            f"--key-maximum={max_key}",
            "--random-data",
            "--threads=5",
            "--clients=4",
            "--server=127.0.0.1",
            "--port=7000",
            "--hide-histogram",
        ]

    def __create_commands(
        memtier_bench_path: str, key_spans: list[tuple[int, int]]
    ) -> list[list[str]]:
        return [
            MemtierBenchRunner.__create_command_bench(
                memtier_bench_path, min_key, max_key
            )
            for min_key, max_key in key_spans
        ]

    def __calculate_total_iops(outputs: list[str]) -> (float, float, float):
        total_iops = 0
        get_iops = 0
        set_iops = 0
        for output in outputs:
            output = output.decode()
            gets_mo = re.findall("Gets *([0-9]*)", output)
            gets = int(gets_mo[0])
            sets_mo = re.findall("Sets *([0-9]*)", output)
            sets = int(sets_mo[0])
            total_iops += sets + gets
            get_iops += gets
            set_iops += sets
        return (get_iops, set_iops, total_iops)

    def __init__(
        self,
        memtier_bench_path: str,
        min_key: int,
        max_key: int,
        subareas: int,
        used_ratio: float,
    ):
        key_spans = MemtierBenchRunner.__split_keys(
            min_key, max_key, subareas, used_ratio
        )
        self.commands = MemtierBenchRunner.__create_commands(
            memtier_bench_path, key_spans
        )

    def run_iteration(self) -> float:
        handles = []
        outputs = []
        for command in self.commands:
            handles.append(subprocess.Popen(command, stdout=subprocess.PIPE))
        for handle in handles:
            output, _errs = handle.communicate()
            outputs.append(output)  # TODO decode ?
            assert _errs is None
        return MemtierBenchRunner.__calculate_total_iops(outputs)


def fill_database(memtier_bench_path: str, min_key: int, max_key: int):
    # TODO some error check on return values ?
    _ = subprocess.run(
        MemtierBenchRunner.create_command_fill(
            memtier_bench_path, min_key, max_key
        ),
        stdout=subprocess.PIPE,
    )


if __name__ == "__main__":
    # TODO update redis path - take from arguments ?
    parser = argparse.ArgumentParser("Run redis benchmark")
    parser.add_argument("--redis_path")
    parser.add_argument("--memtier_bench_path")
    args = parser.parse_args(sys.argv[1:])
    redis_path = args.redis_path
    memtier_bench_path = args.memtier_bench_path
    iopses_get = []
    iopses_set = []
    iopses_total = []
    with RedisServer(redis_path) as redis_server:
        bench_runner = MemtierBenchRunner(
            memtier_bench_path, MIN_KEY, MAX_KEY, SUBAREAS, USED_RATIO
        )
        fill_database(memtier_bench_path, MIN_KEY, MAX_KEY)
        print(f"Running {ITERATIONS} access iterations, iopsees:")
        for _ in range(ITERATIONS):
            gets, sets, total_iops = bench_runner.run_iteration()
            iopses_get.append(gets)
            iopses_set.append(sets)
            iopses_total.append(total_iops)
            print(f"gets: {gets}, sets: {sets}, total: {total_iops}")
        print("Shutting down redis-server...")
    print("run finished, iopses:")
    print(f"gets: {iopses_get}")
    print(f"sets: {iopses_set}")
    print(f"total: {iopses_total}")
    plt.xlabel("iteration")
    plt.ylabel("iopses")
    plt.grid(True)
    plt.plot(iopses_get)
    plt.plot(iopses_set)
    plt.plot(iopses_total)
    plt.legend(["gets", "sets", "total"])
    plt.savefig("iopses.png")
