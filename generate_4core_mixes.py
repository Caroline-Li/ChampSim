import random

def get_experiment_traces(trace_list):
    traces = []
    experiment_trace_files = []
    for trace in trace_list:
        trace = trace[:-1]
        traces.append(trace)
    return (traces)

def main():
    trace_list = open('sim_list/traces_subset.txt', 'r')
    traces = get_experiment_traces(trace_list)
    print(traces)

    multicore_mixes = open('sim_list/4core_spec2006_mixes_subset.txt', 'w')

    for i in range(0, 10):
        for j in range(0, 4):
            trace_num = random.randint(0, 11)
            multicore_mixes.write(f'{traces[trace_num]} ')
        multicore_mixes.write('\n')

            


if __name__ == "__main__":
    main()
