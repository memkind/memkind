// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2015 - 2020 Intel Corporation. */
#pragma once
#include <assert.h>
#include <pthread.h>

#include "Runnable.hpp"

class Thread
{
public:
    Thread(Runnable *runnable) : runnable_task(runnable)
    {}

    void start()
    {
        int err = pthread_create(&thread_handle, NULL, execute_thread,
                                 static_cast<void *>(runnable_task));
        assert(!err);
    };

    void wait()
    {
        pthread_join(thread_handle, NULL);
    };

    Runnable *get_runnable_task()
    {
        return runnable_task;
    }

private:
    static void *execute_thread(void *ptr)
    {
        Runnable *runnable = static_cast<Runnable *>(ptr);
        assert(runnable);
        runnable->run();
        pthread_exit(NULL);
    }

    pthread_t thread_handle;
    Runnable *runnable_task;
};

class ThreadsManager
{
public:
    ThreadsManager(std::vector<Thread *> &threads_vec) : threads(threads_vec)
    {}
    ~ThreadsManager()
    {
        release();
    }

    void start()
    {
        for (int i = 0; i < threads.size(); i++) {
            threads[i]->start();
        }
    }

    void barrier()
    {
        for (int i = 0; i < threads.size(); i++) {
            threads[i]->wait();
        }
    }

    void release()
    {
        for (int i = 0; i < threads.size(); i++) {
            delete threads[i];
        }
        threads.clear();
    }

private:
    std::vector<Thread *> &threads;
};
