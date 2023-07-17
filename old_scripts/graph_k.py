#!/usr/bin/python3
import sys, re
import matplotlib as mpl
import numpy as np
mpl.use('Agg')
import matplotlib.pyplot as plt

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

def filter_top_pc(sorted_pc_freq):
    filtered_sorted_pc_freq = []
    print(sorted_pc_freq)
    filtered_sorted_pc_freq.append(sorted_pc_freq[len(sorted_pc_freq)-1])
    return filtered_sorted_pc_freq

def filter_k_list_map(filtered_sorted_pc_freq, k_list_map):
    filtered_k_list_map = {}
    for element in filtered_sorted_pc_freq:
        filtered_k_list_map[element[0]] = k_list_map[element[0]]
    return filtered_k_list_map


def plot_filtered_k_lists(sorted_pc_freq, k_distr, k_list_map, filter_func, trace):
    filtered_sorted_pc_freq = filter_func(sorted_pc_freq)
    filtered_k_list_map = filter_k_list_map(filtered_sorted_pc_freq, k_list_map)
    for pc in filtered_k_list_map.keys():
        print(pc)
        fig, ax = plt.subplots()
        for k_list in filtered_k_list_map[pc][0:7]:
            print(k_list)
            ax.plot(k_list)
        plt_name = 'astar_filtered_k_list.png'
        plt.savefig(plt_name)
        
    
        
def plot_k_distr(k_distr, trace):
    mpl.use('Agg')
    fig, ax = plt.subplots()
    ax.hist(k_distr, rwidth=0.8, bins = 100)
    print(f'k dist mean: {np.mean(k_distr)}')
    print(f'k dist median: {np.median(k_distr)}')
    plt.savefig('astar_k_distr.png', format='png')

def graph_k_all(comp_trace_files):
    fig, axs = plt.subplots()
    i = 0
    k_distr = []
    for trace in comp_trace_files:
        print(trace)
        with open(trace, 'r') as f:
            filter_one = False
            lines = [line.rstrip() for line in f]
            for line in lines:
                split = re.split(',| ', line)
                if (split[0] == 'PC:'):
                    k_start = 5
                    str_k_list = [k for k in split[k_start:] if k != '']
                    k_list = []
                    try:
                        k_list = list(map(int, str_k_list))
                    except:
                        print(str_k_list)
                        print('k_list contains weird characters!')
                    # axs[i].plot(k_list)
                    k_list_non_zero = [k for k in k_list if k != 0]
                    # if (not filter_one and float(split[3]) > 50.0 and len(k_list_non_zero) > 1):
                    #     filter_one = True
                    #     print(k_list_non_zero)
                    #     print(k_list)
                    if (len(k_list_non_zero) > 0):
                        k_distr.extend(k_list)
            i += 1
    axs.hist(k_distr, rwidth=0.8, bins = 100)
    print(f'k dist mean: {np.mean(k_distr)}')
    print(f'k dist median: {np.median(k_distr)}')
    plt.savefig('k.png')

def main():
    if len(sys.argv) < 3:
        print('Need to specify a baseline!')
        print('Example usage: ./print_results.py <baseline-experiment> <comparison-experiment>')
        sys.exit(0)

    baseline = sys.argv[1]
    comparison = sys.argv[2]

    trace_list = open('sim_list/traces.txt', 'r')
    base_trace_files = []
    comp_trace_files = []
    traces = []
    for trace in trace_list:
        trace = trace[:-1]
        traces.append(trace)
        base_trace = 'output/'+baseline+'/'+trace+'.txt'
        base_trace_files.append(base_trace)
        comp_trace = 'output/'+comparison+'/'+trace+'.txt'
        comp_trace_files.append(comp_trace)
    
    for trace in comp_trace_files[0:1]:
        (sorted_pc_freq, k_distr, k_list_map) = parse_stats(trace)
        plot_filtered_k_lists(sorted_pc_freq, k_distr, k_list_map, filter_top_pc, trace)
        plot_k_distr(k_distr, trace)
    graph_k_all(comp_trace_files)


if __name__ == "__main__":
    main()




