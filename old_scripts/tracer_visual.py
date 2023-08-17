#!/usr/bin/python3
import sys, re
import matplotlib as mpl
import numpy as np
import os
import subprocess
import copy
mpl.use('Agg')
# mpl.rcParams['lines.linewidth'] = 2
# mpl.rcParams['axes.labelsize'] = 'medium'
# mpl.rcParams['axes.titlesize'] = 'large'
# mpl.rcParams['xtick.labelsize'] = 'small'
mpl.rcParams['font.size'] = 14
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import argparse

NROWS=5
NCOLS=5

def parse_stats(trace):
    print(trace)
    sorted_pc_freq = []
    k_distr = []
    k_list_map = {}
    k_start = 5
    with open(trace, 'r') as f:
        lines = [line.rstrip() for line in f]
        for line in lines:
            split = re.split(',| ', line)
            if (split[0] == 'PC:' and split[2] == 'Freq:'):
                sorted_pc_freq.append((int(split[1]), float(split[3])))
            elif (split[0] == 'PC:' and split[2] == 'full_addr:'):
                pc = int(split[1])
                str_k_list = [k for k in split[k_start:] if k != '']
                k_list = []
                try:
                    k_list = list(map(int, str_k_list))
                except:
                    print('k_list contains weird characters!')
                # axs[i].plot(k_list)
                k_list_non_zero = [k for k in k_list if k != 0]
                if (len(k_list_non_zero) > 0):
                    k_distr.extend(k_list)
                    if (pc not in k_list_map.keys()):
                        k_list_map[pc] = list()
                    k_list_map[pc].append(k_list)
    return (sorted_pc_freq, k_distr, k_list_map)

def parse_k_lists(trace):
    k_distr = []
    for line in open(trace, 'r'):
        if (re.match("cacheline:", line)):
            split = re.split(',| ', line.rstrip())
            split = [i.strip() for i in split if i.strip()]
            if (len(split) > 3):
                k_list = list(map(int, split[3:]))
                k_distr.extend(k_list)
                
    return k_distr

def get_k_list_compression(trace):
    k_compression = []
    for line in open(trace, 'r'):
        if (re.match("cacheline:", line)):
            split = re.split(',| ', line.rstrip())
            split = [i.strip() for i in split if i.strip()]
            if (len(split) > 9):
                k_list = split[3:]
                k_str = ""
                for k in k_list:
                    k_str += str(k)
                output = subprocess.check_output(f'echo -n {k_str} | sequitur/c++/sequitur -p', shell=True, stderr=subprocess.STDOUT)
                print(k_str)
                output_str = str(output)
                output_split = re.split('->', output_str)
                k_compression.append(len(output_split) - 1)
                if (len(k_compression) > 200):
                    break
    return k_compression

def get_k_list_unique(trace):
    k_percent_unique_distr = []
    for line in open(trace, 'r'):
        if (re.match("cacheline:", line)):
            split = re.split(',| ', line.rstrip())
            split = [i.strip() for i in split if i.strip()]
            if (len(split) > 9):
                k_list = k_list = list(map(int, split[3:]))
                unique_k_list = set(k_list)
                k_percent_unique_distr.append(float(len(unique_k_list) / float(len(k_list)))*100.0)
    return k_percent_unique_distr


def search(regex, pos, trace_files):
    result = []
    for trace_file in trace_files:
        found = False
        for line in open(trace_file, 'r'):
            if not found and re.match(regex, line):
                result.append(float(line.split()[pos]))
                found = True
    return result

def parse_timeliness(trace):
    time_distr = []
    for line in open(trace, 'r'):
        if (re.match("cacheline:", line)):
            split = re.split(',| ', line.rstrip())
            split = [i.strip() for i in split if i.strip()]
            if (len(split) > 3):
                time_list = list(map(int, split[3:]))
                time_distr.extend(time_list)
                
    return time_distr

def search_prefetch(trace_files):
    percent_prefetch_traffic_reduction = []
    percent_prefetch_traffic_reduction_all = []
    percent_prefetch_traffic_reduction_in_demand = []
    num_total_prefetches = []
    num_total_miss = []
    num_total_prefetch_fills_with_demand = []
    percent_prefetch_fills_with_demand = []

    percent_prefetch_miss_evict_interval = []
    percent_prefetch_hit_evict_interval = []
    percent_prefetch_miss_prefetch_hit = []
    percent_prefetch_hit_prefetch_hit = []
    percent_prefetch_miss_demand_hit = []
    percent_prefetch_hit_demand_hit = []
    percent_prefetch_miss_end_interval = []
    percent_prefetch_hit_end_interval = []
    num_total_prefetch_intervals = []
    num_total_prefetch_fills = []
    champsim_useful_prefetch = []
    for trace_file in trace_files:
        found = False
        count = [False] * 17
        for line in open(trace_file, 'r'):
            if (not count[0] and re.match('LLC TOTAL', line)):
                num_total_miss.append(float(line.split()[7]))
                count[0] = True
            elif (not count[1] and re.match('LLC PREFETCH  ACCESS:', line)):
                num_total_prefetches.append(float(line.split()[7]))
                count[1] = True
            elif not count[2] and re.match("num total prefetch fills with demand: ", line):
                num_total_prefetch_fills_with_demand.append(float(line.split()[6]))
                count[2] = True
            elif not count[3] and re.match("percent prefetch fills with demand: ", line):
                percent_prefetch_fills_with_demand.append(float(line.split()[5]))
                count[3] = True
            elif not count[4] and re.match("percent prefetch reduction traffic: ", line):
                percent_prefetch_traffic_reduction.append(float(line.split()[4]))
                count[4] = True
            elif not count[5] and re.match("percent prefetch reduction demand: ", line):
                percent_prefetch_traffic_reduction_in_demand.append(float(line.split()[4]))
                count[5] = True
            elif not count[6] and re.match("percent prefetch_miss_evict_interval: ", line):
                percent_prefetch_miss_evict_interval.append(float(line.split()[2]))
                count[6] = True
            elif not count[7] and re.match("percent prefetch_hit_evict_interval: ", line):
                percent_prefetch_hit_evict_interval.append(float(line.split()[2]))
                count[7] = True
            elif not count[8] and re.match("percent prefetch_miss_prefetch_hit: ", line):
                percent_prefetch_miss_prefetch_hit.append(float(line.split()[2]))
                count[8] = True
            elif not count[9] and re.match("percent prefetch_hit_prefetch_hit: ", line):
                percent_prefetch_hit_prefetch_hit.append(float(line.split()[2]))
                count[9] = True
            elif not count[10] and re.match("percent prefetch_miss_demand_hit: ", line):
                percent_prefetch_miss_demand_hit.append(float(line.split()[2]))
                count[10] = True
            elif not count[11] and re.match("percent prefetch_hit_demand_hit: ", line):
                percent_prefetch_hit_demand_hit.append(float(line.split()[2]))
                count[11] = True
            elif not count[12] and re.match("percent prefetch_miss_end_interval: ", line):
                percent_prefetch_miss_end_interval.append(float(line.split()[2]))
                count[12] = True
            elif not count[13] and re.match("percent prefetch_hit_end_interval: ", line):
                percent_prefetch_hit_end_interval.append(float(line.split()[2]))
                count[13] = True
            elif not count[14] and re.match('percent prefetch reduction traffic all: ', line):
                percent_prefetch_traffic_reduction_all.append(float(line.split()[5]))
                count[14] = True
            elif not count[15] and re.match('num total prefetch intervals: ', line):
                num_total_prefetch_intervals.append(float(line.split()[4]))
                count[15] = True
            elif not count[16] and re.match('num total prefetch fills: ', line):
                num_total_prefetch_fills.append(float(line.split()[4]))
                count[16] = True
    return (num_total_miss, num_total_prefetches, num_total_prefetch_fills_with_demand,
        percent_prefetch_fills_with_demand, percent_prefetch_traffic_reduction, 
        percent_prefetch_traffic_reduction_all, percent_prefetch_traffic_reduction_in_demand,
        percent_prefetch_miss_evict_interval, percent_prefetch_hit_evict_interval,
        percent_prefetch_miss_prefetch_hit, percent_prefetch_hit_prefetch_hit,
        percent_prefetch_miss_demand_hit, percent_prefetch_hit_demand_hit,
        percent_prefetch_miss_end_interval, percent_prefetch_hit_end_interval, num_total_prefetch_intervals,
        num_total_prefetch_fills)

def add_data_labels(bar, ax, labels):
    rects = bar.patches
    for rect, label in zip(rects, labels):
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height, label, ha="center", va="bottom")

def add_geometric_mean(data):
    a = np.log(data)
    data.append(np.exp(a.mean()))
    return data
def mean(data):
    total = 0
    count = 0
    for x in data:
        if (not np.isnan(x)):
            total += x
        else:
            total += 0
        count += 1
    return  round(float(total/count), 2)
def add_mean(data):
    data.append(mean(data))
    return data

def parse_k_predictor_accuracy(trace_files):
    accuracy = []
    for trace_file in trace_files:
        found = False
        for line in open(trace_file, 'r'):
            if (not found and re.match('accuracy with 0s: ', line)):
                accuracy.append(float(line.split()[3]))
    return accuracy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', '-i', help='input experiment directory')
    parser.add_argument('--prefetch-use', '-p', action='store_true', default=False, help='graph prefetch use stats')
    parser.add_argument('--k_distr', action='store_true', default=False, help='enable k distribution graphing')
    parser.add_argument('--k_compression', action='store_true', default=False, help='enable k compression analysis')
    parser.add_argument('--k_percent_unique', action='store_true', default=False, help='enable k percent unique analysis')
    parser.add_argument('--k_list_filtering', action='store_true', default=False, help='enable k list analysis')
    parser.add_argument('--compare-prefetch-use', help = 'second input experiment directory')
    parser.add_argument('--time_distr', action='store_true', default=False, help='enable time distr analysis')
    parser.add_argument('--k-predictor-accuracy', action='store_true', default=False, help='enable k predictor accuracy analysis')
    args = parser.parse_args()
    if args.input == None:
        print('Need to specify an experiment!')
        print('Example usage: ./print_results.py <experiment>')
        sys.exit(0)

    experiment = args.input
    trace_list = open('sim_list/traces.txt', 'r')
    experiment_trace_files = []
    traces = []

    for trace in trace_list:
        trace = trace[:-1]
        traces.append(trace)
        experiment_trace = 'current_output/'+experiment+'/'+trace+'.txt'
        experiment_trace_files.append(experiment_trace)
    print(len(traces))
    
    if (args.prefetch_use):
        (num_total_miss, num_total_prefetches, num_total_prefetch_fills_with_demand,
        percent_prefetch_fills_with_demand, percent_prefetch_traffic_reduction, 
        percent_prefetch_traffic_reduction_all, percent_prefetch_traffic_reduction_in_demand,
        percent_prefetch_miss_evict_interval, percent_prefetch_hit_evict_interval,
        percent_prefetch_miss_prefetch_hit, percent_prefetch_hit_prefetch_hit,
        percent_prefetch_miss_demand_hit, percent_prefetch_hit_demand_hit,
        percent_prefetch_miss_end_interval, percent_prefetch_hit_end_interval,
        num_total_prefetch_intervals, num_total_prefetch_fills)= search_prefetch(experiment_trace_files)

        percent_prefetches = [round(num_total_prefetches[i]/num_total_miss[i]*100.0, 2) for i in range(len(traces))]
        percent_prefetch_fills_with_demand = [round(i*100.0, 2) for i in percent_prefetch_fills_with_demand]
        percent_prefetch_traffic_reduction = [round(i*100.0, 2) for i in percent_prefetch_traffic_reduction]
        percent_prefetch_traffic_reduction_all = [round(i*100.0, 2) for i in percent_prefetch_traffic_reduction_all]
        percent_prefetch_traffic_reduction_in_demand = [round(i*100.0, 2) for i in percent_prefetch_traffic_reduction_in_demand]
        percent_prefetch_miss_evict_interval = [round(i*100.0, 2) for i in percent_prefetch_miss_evict_interval]
        percent_prefetch_hit_evict_interval = [round(i*100.0, 2) for i in percent_prefetch_hit_evict_interval]
        percent_prefetch_miss_prefetch_hit = [round(i*100, 2) for i in percent_prefetch_miss_prefetch_hit]
        percent_prefetch_hit_prefetch_hit = [round(i*100.0, 2) for i in percent_prefetch_hit_prefetch_hit]
        percent_prefetch_miss_demand_hit = [round(i*100.0, 2) for i in percent_prefetch_miss_demand_hit]
        percent_prefetch_hit_demand_hit = [round(i*100, 2) for i in percent_prefetch_hit_demand_hit]
        percent_prefetch_miss_end_interval = [round(i*100.0, 2) for i in percent_prefetch_miss_end_interval]
        percent_prefetch_hit_end_interval = [round(i*100, 2) for i in percent_prefetch_hit_end_interval]
        percent_prefetch_fills = [round(num_total_prefetch_fills[i]/num_total_prefetch_intervals[i]*100.0, 2) if (num_total_prefetch_fills[i] > 0) else 0 for i in range(len(traces))]
        percent_prefetch_traffic_reduction_all_prefetch_intervals = [round(percent_prefetch_traffic_reduction_all[i]*percent_prefetch_fills[i]/100.0, 2) if (not np.isnan(percent_prefetch_traffic_reduction_all[i])) else 0 for i in range(len(traces))]
        percent_prefetch_traffic_reduction_alt = [round(100 - percent_prefetch_miss_demand_hit[i] - percent_prefetch_hit_demand_hit[i], 2) for i in range(len(traces))]
        percent_useful_prefetches = [round(1 - percent_prefetch_traffic_reduction_all_prefetch_intervals[i], 2) for i in range(len(traces))]

        percent_prefetches = add_mean(percent_prefetches)
        percent_prefetch_fills_with_demand = add_mean(percent_prefetch_fills_with_demand)
        percent_prefetch_traffic_reduction = add_mean(percent_prefetch_traffic_reduction)
        percent_prefetch_traffic_reduction_all = add_mean(percent_prefetch_traffic_reduction_all)
        percent_prefetch_traffic_reduction_in_demand = add_mean(percent_prefetch_traffic_reduction_in_demand)
        percent_prefetch_miss_evict_interval = add_mean(percent_prefetch_miss_evict_interval)
        percent_prefetch_hit_evict_interval = add_mean(percent_prefetch_hit_evict_interval)
        percent_prefetch_miss_prefetch_hit = add_mean(percent_prefetch_miss_prefetch_hit)
        percent_prefetch_hit_prefetch_hit = add_mean(percent_prefetch_hit_prefetch_hit)
        percent_prefetch_miss_demand_hit = add_mean(percent_prefetch_miss_demand_hit)
        percent_prefetch_hit_demand_hit = add_mean(percent_prefetch_hit_demand_hit)
        percent_prefetch_miss_end_interval = add_mean(percent_prefetch_miss_end_interval)
        percent_prefetch_hit_end_interval = add_mean(percent_prefetch_hit_end_interval)
        percent_prefetch_traffic_reduction_alt = add_mean(percent_prefetch_traffic_reduction_alt)
        percent_useful_prefetcher = add_mean(percent_useful_prefetches)


        percent_prefetch_fills = add_mean(percent_prefetch_fills)
        percent_prefetch_traffic_reduction_all_prefetch_intervals = add_mean(percent_prefetch_traffic_reduction_all_prefetch_intervals)

        traces_with_mean = copy.deepcopy(traces)
        traces_with_mean.append('zz_mean')

        print('percent prefetches: ')
        print(percent_prefetches)

        print('percent_prefetch_fills_with_demand: ')
        print(percent_prefetch_fills_with_demand)

        print('percent_prefetch_traffic_reduction: ')
        print(percent_prefetch_traffic_reduction)

        print('percent_prefetch_traffic_reduction_all: ')
        print(percent_prefetch_traffic_reduction_all)

        print('percent_prefetch_traffic_reduction_in_demand: ')
        print(percent_prefetch_traffic_reduction_in_demand)

        print('percent_prefetch_miss_evict_interval: ')
        print(percent_prefetch_miss_evict_interval)
        print('percent_prefetch_hit_evict_interval ')
        print(percent_prefetch_hit_evict_interval)
        print('percent_prefetch_miss_prefetch_hit: ')
        print(percent_prefetch_miss_prefetch_hit)

        print('percent_prefetch_hit_prefetch_hit: ')
        print(percent_prefetch_hit_prefetch_hit)
        print('percent_prefetch_miss_demand_hit ')
        print(percent_prefetch_miss_demand_hit)
        print('percent_prefetch_hit_demand_hit: ')
        print(percent_prefetch_hit_demand_hit)

        print('percent_prefetch_miss_end_interval ')
        print(percent_prefetch_miss_end_interval)
        print('percent_prefetch_hit_end_interval: ')
        print(percent_prefetch_hit_end_interval)

        print('percent prefetch fills')
        print(percent_prefetch_fills)

        print('percent_preftch_traffic_reduction_all_prefetch_intervals')
        print(percent_prefetch_traffic_reduction_all_prefetch_intervals)

        print('percent prefetch traffic reduction alt')
        print(percent_prefetch_traffic_reduction_alt)
        fig, ax = plt.subplots(2)
        bar = ax[0].bar(traces_with_mean, np.array(percent_prefetches), width = 0.5, label="percent prefetches")
        ax[0].set_ylabel("percent of accesses")
        add_data_labels(bar, ax[0], percent_prefetches)

        # prefetch_reduction = np.array(percent_prefetch_traffic_reduction)
        bar1 = ax[1].bar(traces_with_mean, percent_prefetch_miss_evict_interval, width=0.5, label='percent_prefetch_miss_evict_interval')
        bar2 = ax[1].bar(traces_with_mean, percent_prefetch_hit_evict_interval, bottom= np.array(percent_prefetch_miss_evict_interval), width=0.5, label='percent_prefetch_hit_evict_interval')
        bar3 = ax[1].bar(traces_with_mean, percent_prefetch_miss_prefetch_hit, bottom= np.array(percent_prefetch_miss_evict_interval) + np.array(percent_prefetch_hit_evict_interval), width=0.5, label='percent_prefetch_miss_prefetch_hit_interval')
        bar4 = ax[1].bar(traces_with_mean, percent_prefetch_hit_prefetch_hit, bottom= np.array(percent_prefetch_miss_evict_interval) + np.array(percent_prefetch_hit_evict_interval) + np.array(percent_prefetch_miss_prefetch_hit), width=0.5, label='percent_prefetch_hit_prefetch_hit_interval')
        bar5 = ax[1].bar(traces_with_mean, percent_prefetch_miss_demand_hit, bottom= np.array(percent_prefetch_miss_evict_interval) + np.array(percent_prefetch_hit_evict_interval) + np.array(percent_prefetch_miss_prefetch_hit) + np.array(percent_prefetch_hit_prefetch_hit), width=0.5, label='percent_prefetch_miss_demand_hit_interval')
        bar6 = ax[1].bar(traces_with_mean, percent_prefetch_hit_demand_hit, bottom= np.array(percent_prefetch_miss_evict_interval) + np.array(percent_prefetch_hit_evict_interval) + np.array(percent_prefetch_miss_prefetch_hit) + np.array(percent_prefetch_hit_prefetch_hit) + np.array(percent_prefetch_miss_demand_hit), width=0.5, label='percent_prefetch_hit_demand_hit_interval')
        bar7 = ax[1].bar(traces_with_mean, percent_prefetch_miss_end_interval, bottom= np.array(percent_prefetch_miss_evict_interval) + np.array(percent_prefetch_hit_evict_interval) + np.array(percent_prefetch_miss_prefetch_hit) + np.array(percent_prefetch_hit_prefetch_hit) + np.array(percent_prefetch_miss_demand_hit) + np.array(percent_prefetch_hit_demand_hit), width=0.5, label='percent_prefetch_miss_end_interval')
        bar8 = ax[1].bar(traces_with_mean, percent_prefetch_hit_end_interval, bottom= np.array(percent_prefetch_miss_evict_interval) + np.array(percent_prefetch_hit_evict_interval) + np.array(percent_prefetch_miss_prefetch_hit) + np.array(percent_prefetch_hit_prefetch_hit) + np.array(percent_prefetch_miss_demand_hit) + np.array(percent_prefetch_hit_demand_hit) + np.array(percent_prefetch_miss_end_interval), width=0.5, label='percent_prefetch_hit_end_interval')
        ax[1].set_ylabel("percent of prefetches")
        add_data_labels(bar1, ax[1], percent_prefetch_miss_evict_interval)
        fig.set_figwidth(30)
        fig.set_figheight(20)
        ax[1].legend(loc = 'lower left')
        ax[0].set_title('percent of prefetch fills in all cache fills')
        ax[1].set_title('prefetch interval percentages')
        ax[0].set_ylim(bottom=0, top = 100)
        ax[1].set_ylim(bottom=0, top = 100)
        fig.suptitle(f'{args.input} prefetch intervals', fontsize=16)
        plt.savefig(f'{args.input}_prefetch_intervals.png')

        fig, ax = plt.subplots(5)
        bar = ax[0].bar(traces_with_mean, percent_prefetch_fills_with_demand, width = 0.5)
        ax[0].set_ylabel('percent of prefetch fills')
        add_data_labels(bar, ax[0], percent_prefetch_fills_with_demand)
        fig.set_figwidth(40)
        fig.set_figheight(40)
        ax[0].set_ylim(bottom=0, top = 100)
        ax[0].set_title('Accuracy of prefetch fills for demands')

        bar = ax[1].bar(traces_with_mean, percent_prefetch_traffic_reduction, width = 0.5)
        ax[1].set_ylabel('percent of effective prefetch fills')
        ax[1].set_ylim(bottom=0, top = 100)
        ax[1].set_title('percent prefetch traffic reduction with demand')
        add_data_labels(bar, ax[1], percent_prefetch_traffic_reduction)

        bar = ax[2].bar(traces_with_mean, percent_prefetch_traffic_reduction_all, width = 0.5)
        ax[2].set_ylabel('percent of prefetch fills')
        ax[2].set_ylim(bottom=0, top = 100)
        ax[2].set_title('percent prefetch traffic reduction out of all prefetch fills')
        add_data_labels(bar, ax[2], percent_prefetch_traffic_reduction_all)

        bar = ax[3].bar(traces_with_mean, percent_prefetch_fills, width = 0.5)
        ax[3].set_ylabel('percent of prefetch intervals')
        ax[3].set_title('percent prefetch fills in all prefetch intervals')
        ax[3].set_ylim(bottom=0, top = 100)
        add_data_labels(bar, ax[3], percent_prefetch_fills)

        ind = np.arange(0, len(traces) + 1, 1)
        bar = ax[4].bar(ind, percent_prefetch_traffic_reduction_all_prefetch_intervals, width = 0.4, label = 'baseline')
        add_data_labels(bar, ax[4], percent_prefetch_traffic_reduction_all_prefetch_intervals)
        bar = ax[4].bar(ind + 0.4, percent_prefetch_traffic_reduction_alt, width = 0.4, label = 'alt')
        ax[4].set_xticks(np.arange(0, len(traces) + 1, 1))
        ax[4].set_xticklabels(traces_with_mean, rotation = 45, ha='right')
        ax[4].set_ylabel('percent of prefetch intervals')
        ax[4].set_title('percent prefetch traffic reduction out of all prefetch intervals')
        ax[4].set_ylim(bottom=0, top = 100)
        add_data_labels(bar, ax[4], percent_prefetch_traffic_reduction_alt)
        fig.suptitle(f'{args.input} prefetch traffic reduction', fontsize=16)
        plt.savefig(f'{args.input}_prefetch_traffic_reduction.png')

    if (args.compare_prefetch_use != None):
        experiment_cmp = args.compare_prefetch_use
        experiment_trace_files_cmp = []

        for trace in traces:
            experiment_trace = 'current_output/'+experiment_cmp+'/'+trace+'.txt'
            experiment_trace_files_cmp.append(experiment_trace)
        
        (num_total_miss, num_total_prefetches, num_total_prefetch_fills_with_demand,
        percent_prefetch_fills_with_demand, percent_prefetch_traffic_reduction, 
        percent_prefetch_traffic_reduction_all, percent_prefetch_traffic_reduction_in_demand,
        percent_prefetch_miss_evict_interval, percent_prefetch_hit_evict_interval,
        percent_prefetch_miss_prefetch_hit, percent_prefetch_hit_prefetch_hit,
        percent_prefetch_miss_demand_hit, percent_prefetch_hit_demand_hit,
        percent_prefetch_miss_end_interval, percent_prefetch_hit_end_interval,
        num_total_prefetch_intervals, num_total_prefetch_fills)= search_prefetch(experiment_trace_files)

        (num_total_miss_cmp, num_total_prefetches_cmp, num_total_prefetch_fills_with_demand_cmp,
        percent_prefetch_fills_with_demand_cmp, percent_prefetch_traffic_reduction_cmp, 
        percent_prefetch_traffic_reduction_all_cmp, percent_prefetch_traffic_reduction_in_demand_cmp,
        percent_prefetch_miss_evict_interval_cmp, percent_prefetch_hit_evict_interval_cmp,
        percent_prefetch_miss_prefetch_hit_cmp, percent_prefetch_hit_prefetch_hit_cmp,
        percent_prefetch_miss_demand_hit_cmp, percent_prefetch_hit_demand_hit_cmp,
        percent_prefetch_miss_end_interval_cmp, percent_prefetch_hit_end_interval_cmp,
        num_total_prefetch_intervals_cmp, num_total_prefetch_fills_cmp)= search_prefetch(experiment_trace_files_cmp)

        percent_prefetches = [round(num_total_prefetches[i]/num_total_miss[i]*100.0, 2) for i in range(len(traces))]
        percent_prefetch_fills_with_demand = [round(i*100.0, 2) for i in percent_prefetch_fills_with_demand]
        percent_prefetch_traffic_reduction = [round(i*100.0, 2) for i in percent_prefetch_traffic_reduction]
        percent_prefetch_traffic_reduction_all = [round(i*100.0, 2) for i in percent_prefetch_traffic_reduction_all]
        percent_prefetch_traffic_reduction_in_demand = [round(i*100.0, 2) for i in percent_prefetch_traffic_reduction_in_demand]
        percent_prefetch_miss_evict_interval = [round(i*100.0, 2) for i in percent_prefetch_miss_evict_interval]
        percent_prefetch_hit_evict_interval = [round(i*100.0, 2) for i in percent_prefetch_hit_evict_interval]
        percent_prefetch_miss_prefetch_hit = [round(i*100, 2) for i in percent_prefetch_miss_prefetch_hit]
        percent_prefetch_hit_prefetch_hit = [round(i*100.0, 2) for i in percent_prefetch_hit_prefetch_hit]
        percent_prefetch_miss_demand_hit = [round(i*100.0, 2) for i in percent_prefetch_miss_demand_hit]
        percent_prefetch_hit_demand_hit = [round(i*100, 2) for i in percent_prefetch_hit_demand_hit]
        percent_prefetch_miss_end_interval = [round(i*100.0, 2) for i in percent_prefetch_miss_end_interval]
        percent_prefetch_hit_end_interval = [round(i*100, 2) for i in percent_prefetch_hit_end_interval]
        percent_prefetch_fills = [round(num_total_prefetch_fills[i]/num_total_prefetch_intervals[i]*100.0, 2) if (num_total_prefetch_fills[i] > 0) else 0 for i in range(len(traces))]
        percent_prefetch_traffic_reduction_all_prefetch_intervals = [round(percent_prefetch_traffic_reduction_all[i]*percent_prefetch_fills[i]/100.0, 2) if (not np.isnan(percent_prefetch_traffic_reduction_all[i])) else 0 for i in range(len(traces))]

        percent_prefetches = add_mean(percent_prefetches)
        percent_prefetch_fills_with_demand = add_mean(percent_prefetch_fills_with_demand)
        percent_prefetch_traffic_reduction = add_mean(percent_prefetch_traffic_reduction)
        percent_prefetch_traffic_reduction_all = add_mean(percent_prefetch_traffic_reduction_all)
        percent_prefetch_traffic_reduction_in_demand = add_mean(percent_prefetch_traffic_reduction_in_demand)
        percent_prefetch_miss_evict_interval = add_mean(percent_prefetch_miss_evict_interval)
        percent_prefetch_hit_evict_interval = add_mean(percent_prefetch_hit_evict_interval)
        percent_prefetch_miss_prefetch_hit = add_mean(percent_prefetch_miss_prefetch_hit)
        percent_prefetch_hit_prefetch_hit = add_mean(percent_prefetch_hit_prefetch_hit)
        percent_prefetch_miss_demand_hit = add_mean(percent_prefetch_miss_demand_hit)
        percent_prefetch_hit_demand_hit = add_mean(percent_prefetch_hit_demand_hit)
        percent_prefetch_miss_end_interval = add_mean(percent_prefetch_miss_end_interval)
        percent_prefetch_hit_end_interval = add_mean(percent_prefetch_hit_end_interval)

        percent_prefetch_traffic_reduction_all_prefetch_intervals = add_mean(percent_prefetch_traffic_reduction_all_prefetch_intervals)
        print(num_total_prefetches_cmp)
        percent_prefetches_cmp = [round(num_total_prefetches_cmp[i]/num_total_miss_cmp[i]*100.0, 2) for i in range(len(traces))]
        percent_prefetch_fills_with_demand_cmp = [round(i*100.0, 2) for i in percent_prefetch_fills_with_demand_cmp]
        percent_prefetch_traffic_reduction_cmp = [round(i*100.0, 2) for i in percent_prefetch_fills_with_demand_cmp]
        percent_prefetch_traffic_reduction_all_cmp = [round(i*100.0, 2) for i in percent_prefetch_traffic_reduction_all_cmp]
        percent_prefetch_traffic_reduction_in_demand_cmp = [round(i*100.0, 2) for i in percent_prefetch_traffic_reduction_in_demand_cmp]
        percent_prefetch_miss_evict_interval_cmp = [round(i*100.0, 2) for i in percent_prefetch_miss_evict_interval_cmp]
        percent_prefetch_hit_evict_interval_cmp = [round(i*100.0, 2) for i in percent_prefetch_hit_evict_interval_cmp]
        percent_prefetch_miss_prefetch_hit_cmp = [round(i*100, 2) for i in percent_prefetch_miss_prefetch_hit_cmp]
        percent_prefetch_hit_prefetch_hit_cmp = [round(i*100.0, 2) for i in percent_prefetch_hit_prefetch_hit_cmp]
        percent_prefetch_miss_demand_hit_cmp = [round(i*100.0, 2) for i in percent_prefetch_miss_demand_hit_cmp]
        percent_prefetch_hit_demand_hit_cmp = [round(i*100, 2) for i in percent_prefetch_hit_demand_hit_cmp]
        percent_prefetch_miss_end_interval_cmp = [round(i*100.0, 2) for i in percent_prefetch_miss_end_interval_cmp]
        percent_prefetch_hit_end_interval_cmp = [round(i*100, 2) for i in percent_prefetch_hit_end_interval_cmp]
        percent_prefetch_fills_cmp = [round(num_total_prefetch_fills_cmp[i]/num_total_prefetch_intervals_cmp[i]*100.0, 2) if (num_total_prefetch_fills_cmp[i] > 0) else 0 for i in range(len(traces))]
        percent_prefetch_traffic_reduction_all_prefetch_intervals_cmp = [round(percent_prefetch_traffic_reduction_all_cmp[i]*percent_prefetch_fills_cmp[i]/100.0, 2) if (not np.isnan(percent_prefetch_traffic_reduction_all_cmp[i])) else 0 for i in range(len(traces))]

        percent_prefetches_cmp = add_mean(percent_prefetches_cmp)
        percent_prefetch_fills_with_demand_cmp = add_mean(percent_prefetch_fills_with_demand_cmp)
        percent_prefetch_traffic_reduction_cmp = add_mean(percent_prefetch_traffic_reduction_cmp)
        percent_prefetch_traffic_reduction_all_cmp = add_mean(percent_prefetch_traffic_reduction_all_cmp)
        percent_prefetch_traffic_reduction_in_demand_cmp = add_mean(percent_prefetch_traffic_reduction_in_demand_cmp)
        percent_prefetch_miss_evict_interval_cmp = add_mean(percent_prefetch_miss_evict_interval_cmp)
        percent_prefetch_hit_evict_interval_cmp = add_mean(percent_prefetch_hit_evict_interval_cmp)
        percent_prefetch_miss_prefetch_hit_cmp = add_mean(percent_prefetch_miss_prefetch_hit_cmp)
        percent_prefetch_hit_prefetch_hit_cmp = add_mean(percent_prefetch_hit_prefetch_hit_cmp)
        percent_prefetch_miss_demand_hit_cmp = add_mean(percent_prefetch_miss_demand_hit_cmp)
        percent_prefetch_hit_demand_hit_cmp = add_mean(percent_prefetch_hit_demand_hit_cmp)
        percent_prefetch_miss_end_interval_cmp = add_mean(percent_prefetch_miss_end_interval_cmp)
        percent_prefetch_hit_end_interval_cmp = add_mean(percent_prefetch_hit_end_interval_cmp)


        percent_prefetch_traffic_reduction_all_prefetch_intervals_cmp = add_mean(percent_prefetch_traffic_reduction_all_prefetch_intervals_cmp)
        print('harmony num total prefetch fills')
        print(num_total_prefetch_fills_cmp)
        print('hawkeye num total prefetch fills')
        print(num_total_prefetch_fills)
        absolute_change_in_prefetch_fills = [(num_total_prefetch_fills_cmp[i] - num_total_prefetch_fills[i]) for i in range(len(traces))]
        print(absolute_change_in_prefetch_fills)
        absolute_change_in_prefetch_fills = add_mean(absolute_change_in_prefetch_fills)
        percent_change_in_prefetch_fills = [round((num_total_prefetch_fills_cmp[i] - num_total_prefetch_fills[i])/num_total_prefetch_fills[i]*100.0, 2) for i in range(len(traces))]

        percent_change_in_prefetch_fills = add_mean(percent_change_in_prefetch_fills)
        traces_with_mean = copy.deepcopy(traces)
        traces_with_mean.append('zz_mean')

        print('percent change in prefetch fills: ')
        print(percent_change_in_prefetch_fills)

        experiment = 'hawkeye'
        experiment_cmp = 'harmony'

        fig, ax = plt.subplots(4)
        #spines used to move the bottom axis to the center of the plot
        ax[0].spines['bottom'].set_position('center')
        ax[0].set_ylim(bottom=-200, top = 200)
        bar = ax[0].bar(traces_with_mean, percent_change_in_prefetch_fills, width = 0.5)
        ax[0].set_xticklabels(traces_with_mean, rotation=45, ha='right')
        ax[0].set_ylabel('percent change')
        add_data_labels(bar, ax[0], percent_change_in_prefetch_fills)
        fig.set_figwidth(35)
        fig.set_figheight(50)
        ax[0].set_title(f'Percent Change in Prefetch Fills {experiment_cmp} compared to baseline {experiment}')
        fig.suptitle(f'{experiment_cmp} compared to baseline {experiment} prefetch traffic reduction', fontsize=16)

        ax[1].spines['bottom'].set_position('center')
        ax[1].set_ylim(bottom=min(absolute_change_in_prefetch_fills), top = -min(absolute_change_in_prefetch_fills))
        bar = ax[1].bar(traces_with_mean, absolute_change_in_prefetch_fills, width = 0.5)
        ax[1].set_title(f'Absolute Change in Prefetch Fills {experiment_cmp} compared to baseline {experiment}')
        add_data_labels(bar, ax[1], absolute_change_in_prefetch_fills)
        ax[1].set_xticklabels(traces_with_mean, rotation=45, ha='right')
        ax[1].set_ylabel('absolute change in number of prefetch fills')


        ind = np.arange(0, len(traces) + 1, 1)
        ax[2].set_ylim(bottom=0, top = 100)
        bar = ax[2].bar(ind, percent_prefetch_traffic_reduction_all, width = 0.4, label='Hawkeye')
        add_data_labels(bar, ax[2], percent_prefetch_traffic_reduction_all)
        bar = ax[2].bar(ind + 0.4, percent_prefetch_traffic_reduction_all_cmp, width = 0.4, label='Harmony')
        ax[2].set_xticks(np.arange(0, len(traces) + 1, 1))
        ax[2].set_xticklabels(traces_with_mean, rotation = 45, ha='right')
        add_data_labels(bar, ax[2], percent_prefetch_traffic_reduction_all_cmp)
        ax[2].set_title(f'Percent potential prefetch traffic reduction out of total prefetch fills {experiment_cmp} compared to baseline {experiment}')
        ax[2].legend(loc = 'lower left')

        ind = np.arange(0, len(traces) + 1, 1)
        ax[3].set_ylim(bottom=0, top = 100)
        bar = ax[3].bar(ind, percent_prefetch_traffic_reduction_all_prefetch_intervals, width = 0.4, label='Hawkeye')
        add_data_labels(bar, ax[3], percent_prefetch_traffic_reduction_all_prefetch_intervals)
        bar = ax[3].bar(ind + 0.4, percent_prefetch_traffic_reduction_all_prefetch_intervals_cmp, width = 0.4, label='Harmony')
        ax[3].set_xticks(np.arange(0, len(traces) + 1, 1))
        ax[3].set_xticklabels(traces_with_mean, rotation = 45, ha='right')
        add_data_labels(bar, ax[3], percent_prefetch_traffic_reduction_all_prefetch_intervals_cmp)
        ax[3].set_title(f'Percent potential prefetch traffic reduction out of total prefetch intervals {experiment_cmp} compared to baseline {experiment}')
        ax[3].legend(loc = 'lower left')
        fig.savefig(f'{experiment_cmp}_cmp_to_{experiment}_prefetch_traffic_reduction.png')


    if (args.k_predictor_accuracy):
        fig, ax = plt.subplots(1)
        fig.set_figwidth(15)
        fig.set_figheight(10)
        
        accuracy = parse_k_predictor_accuracy(experiment_trace_files)
        accuracy = [round(i*100.0, 2) for i in accuracy]
        accuracy = add_mean(accuracy)

        traces_with_mean = copy.deepcopy(traces)
        traces_with_mean.append('zz_mean')


        bar = ax.bar(traces_with_mean, accuracy, label='ip pht k predictor accuracy')
        ax.set_title('ip pht k predictor accuracy')
        add_data_labels(bar, ax, accuracy)
        ax.set_xticks(np.arange(0, len(traces) + 1, 1))
        ax.set_xticklabels(traces_with_mean, rotation = 45, ha='right')
        ax.set_ylabel('percent accuracy')
        ax.set_ylim(bottom=0, top=100)
        fig.savefig(f'{experiment}_k_predictor_accuracy.png')





    fig = plt.figure(figsize=(30, 30))
    spec = gridspec.GridSpec(nrows = NROWS, ncols = NCOLS, wspace = 0.2, hspace = 0.4)
    axs = []

    if (args.k_distr):
        i = 0
        for r in range(NROWS):
            for c in range(NCOLS):
                ax = fig.add_subplot(spec[r, c])
                ax.set_ylabel('Frequency')
                ax.set_xlabel('k')
                if (i < len(traces)):
                    ax.set_title(f'{traces[i]} k distribution')
                axs.append(ax)
                i += 1

        i = 0
        for trace in experiment_trace_files:
            print(trace)
            k_distr = parse_k_lists(trace)
            bins = 10
            if (len(k_distr) != 0):
                bins = max(k_distr) - min(k_distr)
                if (bins == 0):
                    bins = 10
            print(bins)
            if (i < len(traces)):
                axs[i].hist(k_distr, bins=bins, rwidth=0.8)
            i += 1
        plt.savefig(f'{experiment}_benchmarks_k_distr.png')
    
    if (args.k_compression):
        i = 0
        for r in range(NROWS):
            for c in range(NCOLS):
                ax = fig.add_subplot(spec[r, c])
                ax.set_ylabel('Frequency')
                ax.set_xlabel('compressability factor')
                if (i < len(traces)):
                    ax.set_title(f'{traces[i]} k compressability')
                axs.append(ax)
            i += 1
        i = 0
        for trace in experiment_trace_files:
            print(trace)
            k_compression = get_k_list_compression(trace)
            bins = 10
            if (len(k_compression) != 0):
                bins = max(k_compression) - min(k_compression)
                if (bins == 0):
                    bins = 10
            print(bins)
            if (i < len(traces)):
                axs[i].hist(k_compression, bins=bins, rwidth=0.8)
            i += 1
        plt.savefig(f'{experiment}_compression_k_distr.png')

    if (args.k_percent_unique):
        i = 0
        for r in range(NROWS):
            for c in range(NCOLS):
                ax = fig.add_subplot(spec[r, c])
                ax.set_ylabel('Frequency')
                ax.set_xlabel('Percent Unique')
                if (i < len(traces)):
                    ax.set_title(f'{traces[i]} k percent unique')
                axs.append(ax)
                i += 1
        i = 0
        for trace in experiment_trace_files:
            print(trace)
            k_unique = get_k_list_unique(trace)
            bins = 20
            if (i < len(traces)):
                axs[i].hist(k_unique, bins=bins, rwidth=0.8)
            i += 1
        plt.savefig(f'{experiment}_percent_unique_k_distr.png')
    
    if (args.time_distr):
        i = 0
        for r in range(NROWS):
            for c in range(NCOLS):
                ax = fig.add_subplot(spec[r, c])
                ax.set_ylabel('Frequency')
                ax.set_xlabel('time in clock cycles')
                if (i < len(traces)):
                    ax.set_title(f'{traces[i]} timeliness distribution')
                axs.append(ax)
                i += 1
        i = 0
        for trace in experiment_trace_files:
            print(trace)
            time_distr = parse_timeliness(trace)
            bins = 10
            # if (len(time_distr) != 0):
            #     bins = max(time_distr) - min(time_distr)
            #     if (bins == 0):
            #         bins = 10
            # print(bins)
            # print(time_distr)
            if (i < len(traces) and len(time_distr) > 0):
                bin_arr = np.linspace(min(time_distr), max(time_distr), bins, endpoint=True)
                axs[i].hist(time_distr, bins=bin_arr, rwidth=0.8)
                axs[i].autoscale()
            i += 1
        plt.savefig(f'{experiment}_timeliness_distr.png')
    








    
if __name__ == "__main__":
    main()
