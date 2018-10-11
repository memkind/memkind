#
#  Copyright (C) 2016 - 2018 Intel Corporation.
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

from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
import numpy as np
import numpy.ma as ma
import os
from shutil import rmtree

files = ('alloctest_hbw.txt', 'alloctest_glibc.txt', 'alloctest_tbb.txt', 'alloctest_pmem.txt')
legend = ('avg hbw', 'avg glibc', 'avg tbb', 'avg pmem', 'first operation')
colors = ('red', 'green', 'blue', 'black')
first_operation_color = 'yellow'

threads_values = ('1', '2', '4', '8', '16', '18', '36')

small_sizes_values = ('1', '4', '16')
medium_sizes_values = ('64', '256')
big_sizes_values = ('1024', '4096', '16384')
sizes_values = (small_sizes_values, medium_sizes_values, big_sizes_values)
sizes_display = ('small', 'medium', 'big')

operations = ('Allocation', 'Free', 'Total')

iterations = 1000
output_directory = './plots/'

threads_index      = np.arange(len(threads_values))
small_sizes_index  = np.arange(len(small_sizes_values))
medium_sizes_index = np.arange(len(medium_sizes_values))
big_sizes_index    = np.arange(len(big_sizes_values))
sizes_index = (small_sizes_index, medium_sizes_index, big_sizes_index)

# 3D and 2D plots needs different width, so that bars do not cover each other in 3D
bar_width_2D = 0.2
bar_width_3D = 0.1

# return times (total, allocation, free, first allocation, first free) as columns 2 - 6
def return_times(entry):
    return entry[:,2], entry[:,3], entry[:,4], entry[:,5], entry[:,6]

# initialize axis with labels, ticks and values
def init_axis(fig, suptitle, subplot_projection, x_label, x_ticks, x_values, y_label, y_ticks, y_values, z_label):
    assert fig is not None
    fig.clf()
    fig.suptitle(suptitle)
    if subplot_projection:
        ax = fig.add_subplot(111, projection=subplot_projection)
    else:
        ax = fig.add_subplot(111)
    assert ax is not None
    if x_label:
        ax.set_xlabel(x_label)
        if x_ticks is not None and x_values is not None:
            ax.set_xticks(x_ticks)
            ax.set_xticklabels(x_values)
    if y_label:
        ax.set_ylabel(y_label)
        if y_ticks is not None and y_values is not None:
            ax.set_yticks(y_ticks)
            ax.set_yticklabels(y_values)
    if z_label:
        ax.set_zlabel(z_label)
    return ax

def draw_bar(ax, show_first_operation, x, y, time, init_time, draw_color):
    assert ax is not None
    if y is None:
        if show_first_operation is True:
            mask_time = ma.where(time>=init_time)
            mask_init_time = ma.where(init_time>=time)
            ax.bar(x[mask_time], time[mask_time], width=bar_width, color=draw_color)
            ax.bar(x, init_time, width=bar_width, color=first_operation_color)
            ax.bar(x[mask_init_time], time[mask_init_time], width=bar_width, color=draw_color)
        else:
            ax.bar(x, time, width=bar_width, color=draw_color)
    else:
        if show_first_operation is True:
            mask_time = ma.where(time>=init_time)
            mask_init_time = ma.where(init_time>=time)
            ax.bar(x[mask_time], (time - init_time)[mask_time], y[mask_time], bottom=init_time[mask_time], zdir='y', width=bar_width, color=draw_color)
            ax.bar(x[mask_init_time], (init_time - time)[mask_init_time], y[mask_init_time], bottom=time[mask_init_time], zdir='y', width=bar_width, color=first_operation_color)
            ax.bar(x[mask_init_time], time[mask_init_time], y[mask_init_time], zdir='y', width=bar_width, color=draw_color)
            ax.bar(x[mask_time], init_time[mask_time], y[mask_time], zdir='y', width=bar_width, color=first_operation_color)
        else:
            ax.bar(x, time, y, zdir='y', width=bar_width, color=draw_color)

def save_plot(show_first_operation, subfolder, filename):
    if show_first_operation:
        path = os.path.join(output_directory, "with_first_operation")
    else:
        path = os.path.join(output_directory, "without_first_operation")
    if subfolder:
        path = os.path.join(path, subfolder)
    if not os.path.exists(path):
        os.mkdir(path)
    plt.savefig(os.path.join(path, filename))
    print "Saved file %s" % filename

def load_data_from_files():
    data = []
    # load all files into data
    for f in files:
        assert os.path.isfile(f) is True
        data.append(np.loadtxt(f, comments='#'))
    return data

def set_bar_width_and_offsets(requested_width):
    bar_width = requested_width
    offsets = (0, bar_width, 2*bar_width, 3*bar_width)
    return bar_width, offsets

if os.path.exists(output_directory):
    rmtree(output_directory)
os.mkdir(output_directory)

data = load_data_from_files()

fig = plt.figure()

########################################
# Draw 3D plots (time, sizes, threads) #
########################################

bar_width, offsets = set_bar_width_and_offsets(bar_width_3D)

# for each size range (small, medium, big)
for size_values, size_index, size_display in zip(sizes_values, sizes_index, sizes_display):
    # show plots with and without first operation
    for show_first_operation in (True, False):
        # for each operation (allocation, free, total)
        for operation in operations:
            # add bar_width to each element of size_index
            ax = init_axis(fig, "%s time of %s sizes (%s iterations)" % (operation, size_display, iterations), '3d',
                           'size [kB]', size_index + (bar_width,) * len(size_index), size_values,
                           'threads', threads_index, threads_values,
                           'time [ms]')
            legend_data = []
            # for each allocator (hbw, glibc, tbb, pmem)
            for entry, offset, draw_color in zip(data, offsets, colors):
                # remove all rows where column 1 (size) is not in size_values (current size range)
                entry = entry[np.in1d(entry[:,1], np.array(size_values).astype(np.int))]
                # convert column 0 (threads values) to thread index
                threads_col = np.array([threads_values.index(str(n)) for n in map(int, entry[:,0])])
                # convert column 1 (sizes values) to size index
                size_col = [size_values.index(str(n)) for n in map(int, entry[:,1])]
                # add offset to size index so that bars display near each other
                size_col = np.array(size_col) + offset
                total_time_col, alloc_time_col, free_time_col, first_alloc_time_col, first_free_time_col = return_times(entry)
                if operation == 'Allocation':
                    draw_bar(ax, show_first_operation, size_col, threads_col, alloc_time_col, first_alloc_time_col, draw_color)
                elif operation == 'Free':
                    draw_bar(ax, show_first_operation, size_col, threads_col, free_time_col, first_free_time_col, draw_color)
                elif operation == 'Total':
                    draw_bar(ax, show_first_operation, size_col, threads_col, total_time_col, first_alloc_time_col+first_free_time_col, draw_color)
                legend_data.append(plt.Rectangle((0, 0), 1, 1, fc=draw_color))
            if show_first_operation is True:
                legend_data.append(plt.Rectangle((0, 0), 1, 1, fc=first_operation_color))
            ax.legend(legend_data,legend,loc='best')
            plt.grid()
            save_plot(show_first_operation, None, "%s_time_of_%s_sizes_%s_iterations.png" % (operation, size_display, iterations))

################################################
# Draw 2D plots (time, sizes, constant thread) #
################################################

bar_width, offsets = set_bar_width_and_offsets(bar_width_2D)

# for each size range (small, medium, big)
for size_values, size_index, size_display in zip(sizes_values, sizes_index, sizes_display):
    # show plots with and without first operation
    for show_first_operation in (True, False):
        for thread in threads_values:
            # for each operation (allocation, free, total)
            for operation in operations:
                # add bar_width to each element of size_index
                ax = init_axis(fig, "%s time of %s sizes with %s threads (%s operations)" % (operation, size_display, thread, iterations), None,
                               'size [kB]', size_index + (bar_width,) * len(size_index), size_values,
                               'time [ms]', None, None,
                               None)
                legend_data = []
                # for each allocator (hbw, glibc, tbb, pmem)
                for entry, offset, draw_color in zip(data, offsets, colors):
                    # remove all rows where column 1 (size) is not in size_values (current size range)
                    entry = entry[np.in1d(entry[:,1], np.array(size_values).astype(np.int))]
                    # remove all rows where column 0 (threads) is not equal to currently analyzed thread value
                    entry = entry[entry[:,0] == int(thread)]
                    # convert column 0 (threads values) to thread index
                    threads_col = np.array([threads_values.index(str(n)) for n in map(int, entry[:,0])])
                    # convert column 1 (size values) to size index
                    size_col = [size_values.index(str(n)) for n in map(int, entry[:,1])]
                    # add offset to size index so that bars display near each other
                    size_col = np.array(size_col) + offset
                    total_time_col, alloc_time_col, free_time_col, first_alloc_time_col, first_free_time_col = return_times(entry)
                    if operation == 'Allocation':
                        draw_bar(ax, show_first_operation, size_col, None, alloc_time_col, first_alloc_time_col, draw_color)
                    elif operation == 'Free':
                        draw_bar(ax, show_first_operation, size_col, None, free_time_col, first_free_time_col, draw_color)
                    elif operation == 'Total':
                        draw_bar(ax, show_first_operation, size_col, None, total_time_col, first_alloc_time_col+first_free_time_col, draw_color)
                    legend_data.append(plt.Rectangle((0, 0), 1, 1, fc=draw_color))
                if show_first_operation is True:
                    legend_data.append(plt.Rectangle((0, 0), 1, 1, fc=first_operation_color))
                ax.legend(legend_data,legend,loc='best')
                plt.grid()
                save_plot(show_first_operation, "constant_thread", "%s_time_of_%s_sizes_with_%s_threads_%s_iterations.png" % (operation, size_display, thread, iterations))

################################################
# Draw 2D plots (time, threads, constant size) #
################################################

# for each size range (small, medium, big)
for size_values, size_index, size_display in zip(sizes_values, sizes_index, sizes_display):
    # show plots with and without first operation
    for show_first_operation in (True, False):
        for size in size_values:
            # for each operation (allocation, free, total)
            for operation in operations:
                # add bar_width to each element of threads_index
                ax = init_axis(fig, "%s time of %s kB (%s operations)" % (operation, size, iterations), None,
                               'threads', threads_index + (bar_width,) * len(threads_index), threads_values,
                               'time [ms]', None, None,
                               None)
                legend_data = []
                # for each allocator (hbw, glibc, tbb, pmem)
                for entry, offset, draw_color in zip(data, offsets, colors):
                    # remove all rows where column 1 (size) is not in size_values (current size range)
                    entry = entry[np.in1d(entry[:,1], np.array(size_values).astype(np.int))]
                    # remove all rows where column 1 (size) is not equal to currently analyzed size value
                    entry = entry[entry[:,1] == int(size)]
                    # convert column 0 (threads values) to thread index
                    threads_col = np.array([threads_values.index(str(n)) for n in map(int, entry[:,0])])
                    # add offset to thread index so that bars display near each other
                    threads_col = np.array(threads_col) + offset
                    # convert column 1 (size values) to size index
                    size_col = [size_values.index(str(n)) for n in map(int, entry[:,1])]
                    total_time_col, alloc_time_col, free_time_col, first_alloc_time_col, first_free_time_col = return_times(entry)
                    if operation == 'Allocation':
                        draw_bar(ax, show_first_operation, threads_col, None, alloc_time_col, first_alloc_time_col, draw_color)
                    elif operation == 'Free':
                        draw_bar(ax, show_first_operation, threads_col, None, free_time_col, first_free_time_col, draw_color)
                    elif operation == 'Total':
                        draw_bar(ax, show_first_operation, threads_col, None, total_time_col, first_alloc_time_col+first_free_time_col, draw_color)
                    legend_data.append(plt.Rectangle((0, 0), 1, 1, fc=draw_color))
                if show_first_operation is True:
                    legend_data.append(plt.Rectangle((0, 0), 1, 1, fc=first_operation_color))
                ax.legend(legend_data,legend,loc='best')
                plt.grid()
                save_plot(show_first_operation, "constant_size", "%s_time_of_%s_kB_%s_operations.png" % (operation, size, iterations))
