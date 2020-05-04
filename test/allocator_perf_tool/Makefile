# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2015 - 2020 Intel Corporation.

perf_tool: main.cpp ScenarioWorkload.cpp FunctionCallsPerformanceTask.cpp StressIncreaseToMax.cpp Allocation_info.cpp PmemMockup.cpp
	g++ -o perf_tool $^ -O0 -lmemkind -std=c++11 -lpthread -lnuma -g
